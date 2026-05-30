/***************************************************************************
                          informationformat.h  -  description
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
#ifndef INFORMATION_FORMAT_H
#define INFORMATION_FORMAT_H

#include <iostream>
#include <QString>
#include <QPixmap>
#include <qimage.h>

// Three storage formats for an entry's body:
//   TEXT      — UTF-8 plain text (was "ASCII" pre-3.3.4; the name was a
//               lie since Qt6 — QString is UTF-16 in memory, serialized
//               UTF-8 to XML).
//   HTML      — rich text serialized as HTML (was "RTF" pre-3.3.4;
//               again a lie — the bytes are HTML, not RTF).
//   MARKDOWN  — raw Markdown source kept as-is, rendered on demand.
class InformationFormat{
public:
   static /*const*/ InformationFormat TEXT;
   static /*const*/ InformationFormat HTML;
   static /*const*/ InformationFormat MARKDOWN;


   InformationFormat( QString, QImage );
   //~InformationFormat();

   bool equals( InformationFormat );

   InformationFormat* canbeConvertedTo( void );

   QPixmap getPixmap( void );
   QString toString( void );

   // Reads both the modern names (TEXT/HTML/MARKDOWN) and the legacy
   // pre-3.3.4 names (ASCII/RTF/NONE) so on-disk files written by
   // older tuxcards keep loading. Unknown strings fall back to TEXT.
   static InformationFormat* getByString( QString format );

private:
   QString mDescription;
   QImage  mImage;
};

#endif
