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
#ifndef CONVERTER_H
#define CONVERTER_H

#include <iostream>

#include <QString>
#include "CInformationElement.h"

class Converter{
public:
   static void convert( CInformationElement& );

   // Re-encode the element's content for the requested target format
   // and switch the format flag. No-op if the element is already in
   // `target`. Covers every TEXT/HTML/MARKDOWN cross-conversion;
   // TEXT→{HTML,MARKDOWN} is mechanical, the rich ones route through
   // QTextDocument so we don't reinvent CommonMark/HTML.
   static void convertTo( CInformationElement&, InformationFormat* target );

   static QString convertText2HTML( QString );
   static QString convertHTML2Text( QString );
};

#endif

