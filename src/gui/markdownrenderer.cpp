/***************************************************************************
                          markdownrenderer.cpp  -  description
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "markdownrenderer.h"

#include <QTextDocument>

#if defined(TUXCARDS_WITH_MATH) || defined(TUXCARDS_WITH_DIAGRAMS)
#  include <QImage>
#  include <QHash>
#  include <QVector>
#  include <QCryptographicHash>
#  include <QRegularExpression>
#  include <QPainter>
#  include <QTextCursor>
#endif

#ifdef TUXCARDS_WITH_MATH
#  include <jkqtmathtext/jkqtmathtext.h>
#endif

#ifdef TUXCARDS_WITH_DIAGRAMS
#  include <QByteArray>
#  include <QSvgRenderer>
#  include <graphviz/gvc.h>
#  include <graphviz/cgraph.h>
#endif


namespace MarkdownRenderer
{

bool mathEnabled()
{
#ifdef TUXCARDS_WITH_MATH
   return true;
#else
   return false;
#endif
}

bool diagramsEnabled()
{
#ifdef TUXCARDS_WITH_DIAGRAMS
   return true;
#else
   return false;
#endif
}


#if defined(TUXCARDS_WITH_MATH) || defined(TUXCARDS_WITH_DIAGRAMS)

// Process-wide cache of rendered fragments, keyed by a short hash of the
// (kind + source). The debounced preview re-renders the whole entry on
// every keystroke, so without this each unchanged formula/diagram would
// be re-rasterized constantly.
static QHash<QString, QImage>& fragmentCache()
{
   static QHash<QString, QImage> cache;
   return cache;
}

static QString hashKey( const QString& kind, const QString& src )
{
   QByteArray h = QCryptographicHash::hash( src.toUtf8(),
                                            QCryptographicHash::Sha1 ).toHex();
   return kind + QString::fromLatin1(h);
}

// A pending image to splice into the rendered document. We can't embed
// via Markdown `![](resource)` — QTextDocument::setMarkdown() ignores
// image resources (unlike setHtml). Instead each math/diagram fragment
// is replaced in the source with a unique plain-text token; after
// setMarkdown we locate the token and replace it with the image via
// QTextCursor::insertImage(), which works reliably.
struct PendingImage { QString token; QImage img; };

// Pure A–Z/0–9 so Markdown leaves it untouched as literal text; the
// trailing/leading sentinels keep it from colliding with real prose.
static QString makeToken( int i )
{
   return QStringLiteral("TUXIMGEMBED%1ENDTUXIMG").arg(i);
}

static QString stashImage( QVector<PendingImage>& out, const QImage& img )
{
   const QString tok = makeToken( out.size() );
   out.append( { tok, img } );
   return tok;
}

#endif // any feature


#ifdef TUXCARDS_WITH_DIAGRAMS

// Lay out a DOT graph with Graphviz, render it to SVG, then rasterize
// the SVG to a transparent QImage. Returns a null image on failure so
// the caller can fall back to showing the source text.
static QImage renderDot( const QString& src )
{
   QImage out;
   GVC_t* gvc = gvContext();
   if ( !gvc )
      return out;

   Agraph_t* g = agmemread( src.toUtf8().constData() );
   if ( !g ) {
      gvFreeContext( gvc );
      return out;
   }

   if ( gvLayout( gvc, g, "dot" ) == 0 ) {
      char*  svgData = nullptr;
      size_t svgLen  = 0;
      if ( gvRenderData( gvc, g, "svg", &svgData, &svgLen ) == 0 && svgData ) {
         QByteArray svg( svgData, int(svgLen) );
         QSvgRenderer renderer( svg );
         if ( renderer.isValid() ) {
            QSize sz = renderer.defaultSize();
            if ( sz.isEmpty() )
               sz = QSize( 320, 240 );
            // 2x for a crisper raster on hidpi; preview scales it down.
            out = QImage( sz * 2, QImage::Format_ARGB32_Premultiplied );
            out.fill( Qt::transparent );
            QPainter p( &out );
            renderer.render( &p );
         }
      }
      if ( svgData )
         gvFreeRenderData( svgData );
      gvFreeLayout( gvc, g );
   }

   agclose( g );
   gvFreeContext( gvc );
   return out;
}

// Replace fenced ```dot / ```graphviz blocks with a placeholder token
// (and queue the rendered image). Anything that fails to render is left
// untouched so the raw fence stays visible for the user to fix.
static QString processDiagrams( QString md, QVector<PendingImage>& embeds )
{
   // ```dot|graphviz\n <body> \n``` — multiline, non-greedy body.
   static const QRegularExpression re(
      QStringLiteral("```[ \\t]*(dot|graphviz)[ \\t]*\\r?\\n(.*?)\\r?\\n```"),
      QRegularExpression::DotMatchesEverythingOption );

   QString result;
   int last = 0;
   auto it = re.globalMatch( md );
   while ( it.hasNext() ) {
      QRegularExpressionMatch m = it.next();
      result += md.mid( last, m.capturedStart() - last );
      last = m.capturedEnd();

      const QString body = m.captured( 2 );
      const QString key  = hashKey( "D:", body );

      QImage img = fragmentCache().value( key );
      if ( img.isNull() ) {
         img = renderDot( body );
         if ( !img.isNull() )
            fragmentCache().insert( key, img );
      }

      if ( img.isNull() )
         result += m.captured( 0 );          // keep the original fence
      else
         result += QStringLiteral("\n\n") + stashImage( embeds, img ) + QStringLiteral("\n\n");
   }
   result += md.mid( last );
   return result;
}
#endif

#ifdef TUXCARDS_WITH_MATH

// Rasterize a LaTeX fragment to a transparent QImage via JKQTMathText.
// `tex` is the inner expression (without $ delimiters). Returns a null
// image on parse failure so the caller keeps the source visible.
static QImage renderMath( const QString& tex, bool display )
{
   JKQTMathText mt;
   mt.useXITS();                              // bundled math font, no system dep
   mt.setFontSize( display ? 16.0 : 12.0 );
   if ( !mt.parse( QStringLiteral("$") + tex + QStringLiteral("$") ) )
      return QImage();
   // transparent background so it blends into the preview pane
   return mt.drawIntoImage( false, Qt::transparent );
}

// Mask fenced code blocks and inline code spans so we never treat a `$`
// inside code as math. Returns the masked text; originals are pushed
// into `stash` and restored verbatim afterwards.
static QString maskCode( QString md, QStringList& stash )
{
   static const QRegularExpression fence(
      QStringLiteral("```.*?```"),
      QRegularExpression::DotMatchesEverythingOption );
   static const QRegularExpression inlineCode( QStringLiteral("`[^`\\n]*`") );

   auto maskWith = [&]( const QRegularExpression& re, QString in ) -> QString {
      QString out; int last = 0;
      auto it = re.globalMatch( in );
      while ( it.hasNext() ) {
         auto m = it.next();
         out += in.mid( last, m.capturedStart() - last );
         out += QStringLiteral("\x01CODE") + QString::number(stash.size()) + QStringLiteral("\x01");
         stash << m.captured(0);
         last = m.capturedEnd();
      }
      out += in.mid( last );
      return out;
   };

   md = maskWith( fence, md );
   md = maskWith( inlineCode, md );
   return md;
}

static QString unmaskCode( QString md, const QStringList& stash )
{
   for ( int i = 0; i < stash.size(); ++i )
      md.replace( QStringLiteral("\x01CODE") + QString::number(i) + QStringLiteral("\x01"),
                  stash.at(i) );
   return md;
}

// Replace $$display$$ and $inline$ math with embedded image references.
// Known limitation: a literal '$' outside code can be misread as a math
// delimiter — escape it as \$ or build without TUXCARDS_WITH_MATH.
static QString processMath( QString md, QVector<PendingImage>& embeds )
{
   QStringList stash;
   md = maskCode( md, stash );

   auto replaceAll = [&]( const QRegularExpression& re, bool display ) {
      QString out; int last = 0;
      auto it = re.globalMatch( md );
      while ( it.hasNext() ) {
         auto m = it.next();
         out += md.mid( last, m.capturedStart() - last );
         last = m.capturedEnd();

         const QString tex = m.captured(1);
         const QString key = hashKey( display ? "MD:" : "MI:", tex );
         QImage img = fragmentCache().value( key );
         if ( img.isNull() ) {
            img = renderMath( tex, display );
            if ( !img.isNull() )
               fragmentCache().insert( key, img );
         }
         if ( img.isNull() )
            out += m.captured(0);                 // keep raw on failure
         else if ( display )
            out += QStringLiteral("\n\n") + stashImage(embeds, img) + QStringLiteral("\n\n");
         else
            out += stashImage( embeds, img );
      }
      out += md.mid( last );
      md = out;
   };

   // $$ ... $$ first (display), then single $ ... $ (inline). Inline
   // requires non-space just inside the delimiters to dodge prose dollars.
   static const QRegularExpression display(
      QStringLiteral("\\$\\$(.+?)\\$\\$"),
      QRegularExpression::DotMatchesEverythingOption );
   static const QRegularExpression inlineM(
      QStringLiteral("\\$(\\S(?:[^$\\n]*\\S)?)\\$") );
   replaceAll( display, true );
   replaceAll( inlineM, false );

   return unmaskCode( md, stash );
}
#endif


void renderInto( QTextDocument* doc, const QString& mdSource )
{
   if ( !doc )
      return;

   QString md = mdSource;

#if defined(TUXCARDS_WITH_MATH) || defined(TUXCARDS_WITH_DIAGRAMS)
   QVector<PendingImage> embeds;
#  ifdef TUXCARDS_WITH_DIAGRAMS
   md = processDiagrams( md, embeds );
#  endif
#  ifdef TUXCARDS_WITH_MATH
   md = processMath( md, embeds );
#  endif

   doc->setMarkdown( md );

   // setMarkdown ignores image resources, so splice the rendered
   // fragments in now: find each placeholder token and replace it with
   // the actual image via the cursor (which setHtml-style honors).
   for ( const PendingImage& pi : embeds ) {
      QTextCursor c = doc->find( pi.token );
      if ( !c.isNull() )
         c.insertImage( pi.img );      // replaces the selected token text
   }
#else
   doc->setMarkdown( md );
#endif
}

} // namespace MarkdownRenderer
