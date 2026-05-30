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
// (defined in commit 3)
static QImage renderDot( const QString& src );
static QString processDiagrams( QTextDocument* doc, QString md );
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
