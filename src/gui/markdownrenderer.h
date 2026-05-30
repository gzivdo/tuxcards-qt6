/***************************************************************************
                          markdownrenderer.h  -  description
                             -------------------
    Single entry point for turning a Markdown source string into a
    rendered QTextDocument. When the optional compile-time features
    TUXCARDS_WITH_MATH / TUXCARDS_WITH_DIAGRAMS are enabled it also
    rasterizes LaTeX math ($..$ / $$..$$) and Graphviz ```dot fenced
    blocks into inline images before handing the text to Qt's markdown
    parser. With both features off it is a thin wrapper around
    QTextDocument::setMarkdown().
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef MARKDOWN_RENDERER_H
#define MARKDOWN_RENDERER_H

#include <QString>

class QTextDocument;

namespace MarkdownRenderer
{
   // Render mdSource into doc. Equivalent to doc->setMarkdown(mdSource)
   // plus, when compiled in, inline rendering of math and diagrams.
   void renderInto( QTextDocument* doc, const QString& mdSource );

   // Reflect the compile-time feature flags so the UI can decide whether
   // to surface the math / diagram helper buttons.
   bool mathEnabled();
   bool diagramsEnabled();
}

#endif
