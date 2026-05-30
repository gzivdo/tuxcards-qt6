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
   QString resultingText;
   if (informationElement.getInformationFormat() == &InformationFormat::HTML)
   {
      resultingText = convertHTML2Text( informationElement.getInformation() );
      informationElement.setInformationFormat( &InformationFormat::TEXT );
   }
   else
   {
      resultingText = convertText2HTML( informationElement.getInformation() );
      informationElement.setInformationFormat( &InformationFormat::HTML );
   }
   informationElement.setInformation(resultingText);
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
