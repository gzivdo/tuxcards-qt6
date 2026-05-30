/***************************************************************************
                          informationformat.cpp  -  mDescription
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

#include "informationformat.h"

#include "format_ascii.xpm"
#include "format_rtf.xpm"
#include <QPixmap>

// We keep the legacy XPM files (format_ascii.xpm, format_rtf.xpm) — the
// icon glyphs themselves still look right ("T" for plain, "H/R" for
// rich), it's only the C++ identifier that was misleading.
/*const*/ InformationFormat InformationFormat::TEXT("TEXT", QImage(format_ascii_xpm));
/*const*/ InformationFormat InformationFormat::HTML("HTML", QImage(format_rtf_xpm));
/*const*/ InformationFormat InformationFormat::MARKDOWN("MARKDOWN", QImage(format_ascii_xpm));


// -------------------------------------------------------------------------------
InformationFormat::InformationFormat(QString description, QImage image)
 : mDescription( description )
 , mImage( image )
// -------------------------------------------------------------------------------
{
}


/**
 * Two InformationFormats are equal if the mDescription is equal.
 */
// -------------------------------------------------------------------------------
bool InformationFormat::equals( InformationFormat anotherFormat )
// -------------------------------------------------------------------------------
{
   return ( toString()==anotherFormat.toString() );
}


// -------------------------------------------------------------------------------
InformationFormat* InformationFormat::canbeConvertedTo( void )
// -------------------------------------------------------------------------------
{
   // needs to be implemented
   return this;
}


// -------------------------------------------------------------------------------
QPixmap InformationFormat::getPixmap( void )
// -------------------------------------------------------------------------------
{
   QPixmap pixmap;
   pixmap.convertFromImage( mImage, Qt::AutoColor );
   return pixmap;
}


// -------------------------------------------------------------------------------
QString InformationFormat::toString( void )
// -------------------------------------------------------------------------------
{
   return mDescription;
}



/**
 * Returns a format that was parsed from the string 'format'. Accepts
 * both the current names and the pre-3.3.4 legacy names so old XML
 * files keep loading. Unknown strings fall back to TEXT (was NONE,
 * which is now gone — TEXT is the safe minimum).
 */
// -------------------------------------------------------------------------------
InformationFormat* InformationFormat::getByString( QString format )
// -------------------------------------------------------------------------------
{
  if (format == "HTML" || format == "RTF")
    return &HTML;
  if (format == "MARKDOWN")
    return &MARKDOWN;
  // "TEXT", "ASCII", "NONE", "", or anything else
  return &TEXT;
}
