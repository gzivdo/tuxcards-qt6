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
#  include <QUrl>
#  include <QHash>
#  include <QCryptographicHash>
#  include <QRegularExpression>
#  include <QPainter>
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

// Register img under a private url scheme on doc and return the markdown
// image reference that points at it.
static QString embed( QTextDocument* doc, const QString& urlKey, const QImage& img )
{
   const QUrl url( urlKey );
   doc->addResource( QTextDocument::ImageResource, url, img );
   return QStringLiteral("![](") + urlKey + QStringLiteral(")");
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

// Replace fenced ```dot / ```graphviz blocks with an embedded image
// reference. Anything that fails to render is left untouched (the raw
// fence stays visible so the user can fix it).
static QString processDiagrams( QTextDocument* doc, QString md )
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
      const QString key  = QStringLiteral("tuxdiag://") + hashKey( "D:", body );

      QImage img = fragmentCache().value( key );
      if ( img.isNull() ) {
         img = renderDot( body );
         if ( !img.isNull() )
            fragmentCache().insert( key, img );
      }

      if ( img.isNull() )
         result += m.captured( 0 );          // keep the original fence
      else
         result += QStringLiteral("\n\n") + embed( doc, key, img ) + QStringLiteral("\n\n");
   }
   result += md.mid( last );
   return result;
}
#endif

#ifdef TUXCARDS_WITH_MATH
// (defined in commit 2)
static QImage renderMath( const QString& tex, bool display );
static QString processMath( QTextDocument* doc, QString md );
#endif


void renderInto( QTextDocument* doc, const QString& mdSource )
{
   if ( !doc )
      return;

   QString md = mdSource;

#ifdef TUXCARDS_WITH_DIAGRAMS
   md = processDiagrams( doc, md );
#endif
#ifdef TUXCARDS_WITH_MATH
   md = processMath( doc, md );
#endif

   doc->setMarkdown( md );
}

} // namespace MarkdownRenderer
