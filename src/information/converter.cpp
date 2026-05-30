/***************************************************************************
                          informationformat.cpp  -  description
                             -------------------
    begin                : Sat Jul 13 2002
    copyright            : (C) 2002 by Alexander Theel
    email                : alex.theel@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "converter.h"

#include <QRegExp>
#include <QTextDocument>
#include <QMessageBox>
#include <QTextEdit>

/**
 * Takes an informationElement as parameter, recognizes whether it is
 * an ascii
 */
// -------------------------------------------------------------------------------
void Converter::convert( CInformationElement& informationElement )
// -------------------------------------------------------------------------------
{
   if (informationElement.getInformationFormat() == &InformationFormat::HTML)
      convertTo(informationElement, &InformationFormat::TEXT);
   else
      convertTo(informationElement, &InformationFormat::HTML);
}


// -------------------------------------------------------------------------------
void Converter::convertTo( CInformationElement& elem, InformationFormat* target )
// -------------------------------------------------------------------------------
{
   InformationFormat* src = elem.getInformationFormat();
   if ( src == target || target == nullptr )
      return;

   const QString in = elem.getInformation();
   QString out;

   if (target == &InformationFormat::TEXT) {
      // Strip whatever markup the source carried and keep visible text.
      if (src == &InformationFormat::HTML) {
         QTextDocument doc;
         doc.setHtml(in);
         out = doc.toPlainText();
      } else if (src == &InformationFormat::MARKDOWN) {
         QTextDocument doc;
         doc.setMarkdown(in);
         out = doc.toPlainText();
      } else {
         out = in;
      }
   } else if (target == &InformationFormat::HTML) {
      if (src == &InformationFormat::TEXT) {
         // Plain text → HTML: escape & preserve line breaks.
         out = in.toHtmlEscaped().replace(QChar('\n'), QStringLiteral("<br/>"));
      } else if (src == &InformationFormat::MARKDOWN) {
         QTextDocument doc;
         doc.setMarkdown(in);
         out = doc.toHtml();
      } else {
         out = in;
      }
   } else if (target == &InformationFormat::MARKDOWN) {
      if (src == &InformationFormat::HTML) {
         QTextDocument doc;
         doc.setHtml(in);
         out = doc.toMarkdown();
      } else {
         // Plain text is already valid (degenerate) Markdown.
         out = in;
      }
   }

   elem.setInformation(out);
   elem.setInformationFormat(target);
}



/**
 * Convert plain text to HTML — preserve line breaks via <br/>; nothing
 * else (the editor will interpret the result as rich text).
 */
// -------------------------------------------------------------------------------
QString Converter::convertText2HTML(QString text)
// -------------------------------------------------------------------------------
{
   return text.replace( QChar('\n'), QString("<br/>") );
}



/**
 * Convert HTML body to plain text. Today this is a no-op pass-through;
 * Strings::removeHTMLTags exists for the cases that actually need it.
 */
// -------------------------------------------------------------------------------
QString Converter::convertHTML2Text(QString htmlText)
// -------------------------------------------------------------------------------
{
   return htmlText;

   /*
      Idee:
      alle Tags entfernen
         <html>
         <head>              |-> ""
         <meta>
         <body ...>

         <p>  |-> ""   </p>  |-> "\n\n"
         <br />  |-> "\n"
         <span ...>  |-> ""   </span>  |->  ""
         <li> |-> "- "   </li>  |-> "\n"
         <ul ...>  |-> ""   </ul>  |-> ""
   */
}
