/***************************************************************************
                          CIconManager.cpp  -  description
                             -------------------
    begin                : Tue May 01 2004
    copyright            : (C) 2004 by Alexander Theel
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

#include "CIconManager.h"
#include "../global.h"
#include <QFile>
#include <QPixmap>
#include <iostream>

#include "../icons/fileopen.xpm"
#include "../icons/filenew.xpm"
#include "../icons/filesave.xpm"
#include "../icons/fileprint.xpm"
#include "../icons/exit.xpm"
#include "../icons/addTreeElement.xpm"
#include "../icons/changeProperty.xpm"
#include "../icons/delete.xpm"
#include "../icons/lock.xpm"
#include "../icons/unlock.xpm"
#include "../icons/find.xpm"

#include "../icons/redo.xpm"
#include "../icons/undo.xpm"
#include "../icons/editcut.xpm"
#include "../icons/editcopy.xpm"
#include "../icons/editpaste.xpm"
#include "../icons/editentrycolor.xpm"
#include "../icons/editentrysubtreecolor.xpm"

#include "../icons/text_bold.xpm"
#include "../icons/text_italic.xpm"
#include "../icons/text_under.xpm"
#include "../icons/text_color.xpm"
#include "../icons/text_left.xpm"
#include "../icons/text_center.xpm"
#include "../icons/text_right.xpm"
#include "../icons/text_block.xpm"

#include "../icons/upArrow.xpm"
#include "../icons/downArrow.xpm"
#include "../icons/back.xpm"            // leftArrow
#include "../icons/forward.xpm"         // rightArrow

#include "../icons/locksm.xpm"
#include "../icons/unlocksm.xpm"
#include "../icons/filelock.xpm"
#include "../icons/fileunlock.xpm"

// -------------------------------------------------------------------------------
CIconManager::CIconManager()
// -------------------------------------------------------------------------------
  : mDefaultIconMap()
  , mAlternativeIconMap()
  , msIconDirectory()
  , mFileEndingList()
{
   buildupDefaultIconMap();
   buildupListWithPossibleFileEndings();
}


// -------------------------------------------------------------------------------
CIconManager& CIconManager::getInstance()
// -------------------------------------------------------------------------------
{
   static CIconManager instance;
   return instance;
}


// -------------------------------------------------------------------------------
// Prefer PNG from Qt resource bundle; fall back to inline XPM if missing.
static QPixmap pickIcon( const QString& sQrcPath, const QPixmap& fallback )
{
   QPixmap pix( sQrcPath );
   if ( !pix.isNull() )
      return pix;
   return fallback;
}

void CIconManager::buildupDefaultIconMap()
// -------------------------------------------------------------------------------
{
   mDefaultIconMap["fileopen"]       = pickIcon(":/icons/fileopen.png",  QPixmap(fileopen_xpm));
   mDefaultIconMap["filenew"]        = pickIcon(":/icons/filenew.png",   QPixmap(filenew_xpm));
   mDefaultIconMap["filesave"]       = pickIcon(":/icons/filesave.png",  QPixmap(filesave_xpm));
   mDefaultIconMap["fileprint"]      = pickIcon(":/icons/fileprint.png", QPixmap(fileprint_xpm));
   mDefaultIconMap["exit"]           = pickIcon(":/icons/exit.png",      QPixmap(exit_xpm));
   mDefaultIconMap["addTreeElement"] = QPixmap(addTreeElement_xpm);
   mDefaultIconMap["changeProperty"] = QPixmap(changeProperty_xpm);
   mDefaultIconMap["delete"]         = QPixmap(delete_xpm);
   mDefaultIconMap["lock"]           = QPixmap(lock_xpm);
   mDefaultIconMap["unlock"]         = QPixmap(unlock_xpm);
   mDefaultIconMap["find"]           = pickIcon(":/icons/find.png",      QPixmap(find_xpm));

   mDefaultIconMap["redo"]           = pickIcon(":/icons/editredo.png",  QPixmap(redo_xpm));
   mDefaultIconMap["undo"]           = pickIcon(":/icons/editundo.png",  QPixmap(undo_xpm));
   mDefaultIconMap["editcut"]        = pickIcon(":/icons/editcut.png",   QPixmap(editcut_xpm));
   mDefaultIconMap["editcopy"]       = pickIcon(":/icons/editcopy.png",  QPixmap(editcopy_xpm));
   mDefaultIconMap["editpaste"]      = pickIcon(":/icons/editpaste.png", QPixmap(editpaste_xpm));
   mDefaultIconMap["editentrycolor"]        = QPixmap(editentrycolor_xpm);
   mDefaultIconMap["editentrysubtreecolor"] = QPixmap(editentrysubtreecolor_xpm);

   mDefaultIconMap["text_bold"]      = pickIcon(":/icons/textbold.png",     QPixmap(text_bold_xpm));
   mDefaultIconMap["text_italic"]    = pickIcon(":/icons/textitalic.png",   QPixmap(text_italic_xpm));
   mDefaultIconMap["text_under"]     = pickIcon(":/icons/textunder.png",    QPixmap(text_under_xpm));
   mDefaultIconMap["text_color"]     = QPixmap(text_color_xpm);
   mDefaultIconMap["text_left"]      = pickIcon(":/icons/textleft.png",     QPixmap(text_left_xpm));
   mDefaultIconMap["text_center"]    = pickIcon(":/icons/textcenter.png",   QPixmap(text_center_xpm));
   mDefaultIconMap["text_right"]     = pickIcon(":/icons/textright.png",    QPixmap(text_right_xpm));
   mDefaultIconMap["text_block"]     = pickIcon(":/icons/textjustify.png",  QPixmap(text_block_xpm));

   mDefaultIconMap["upArrow"]        = QPixmap(upArrow_xpm);
   mDefaultIconMap["downArrow"]      = QPixmap(downArrow_xpm);
   mDefaultIconMap["back"]           = pickIcon(":/icons/back.png",     QPixmap(back_xpm));
   mDefaultIconMap["forward"]        = pickIcon(":/icons/forward.png",  QPixmap(forward_xpm));

   mDefaultIconMap["locksm"]         = QPixmap(locksm_xpm);
   mDefaultIconMap["unlocksm"]       = QPixmap(unlocksm_xpm);
   mDefaultIconMap["filelock"]       = QPixmap(filelock_xpm);
   mDefaultIconMap["fileunlock"]     = QPixmap(fileunlock_xpm);
}


// -------------------------------------------------------------------------------
void CIconManager::buildupListWithPossibleFileEndings()
// -------------------------------------------------------------------------------
{
   mFileEndingList.append(".xpm");
   mFileEndingList.append(".png");
   mFileEndingList.append(".gif");
   mFileEndingList.append(".jpg");
   mFileEndingList.append(".bmp");
   mFileEndingList.append(".xbm");
   mFileEndingList.append(".pnm");
   mFileEndingList.append(".mng");
   mFileEndingList.append(".jpeg");
}


// -------------------------------------------------------------------------------
// Sets the directory where to get the icons from.
// -------------------------------------------------------------------------------
void CIconManager::setIconDirectory( const QString& sDir )
// -------------------------------------------------------------------------------
{
   msIconDirectory = sDir;
}


// -------------------------------------------------------------------------------
QString CIconManager::testExistenceOfFile( const QString& sFileName ) const
// -------------------------------------------------------------------------------
{
   for ( uint i = 0; i < mFileEndingList.size(); i++ )
   {
      if( QFile(sFileName + mFileEndingList[i]).exists() )
      {
         return sFileName + mFileEndingList[i];
      }
   }

   return "";
}

// -------------------------------------------------------------------------------
const QPixmap& CIconManager::getIcon( const QString& sFileName )
// -------------------------------------------------------------------------------
{
   // If an icon is already loaded, return that one.
   if ( !mAlternativeIconMap[sFileName].isNull() )
   {
      return mAlternativeIconMap[sFileName];
   }

   // Otherwise, look whether a valid file for the icons does exist.
   QString sResult = testExistenceOfFile( msIconDirectory + "/" + sFileName );
   if ( !sResult.isEmpty() )
   {
      mAlternativeIconMap[sFileName] = QPixmap(sResult);
      return mAlternativeIconMap[sFileName];
   }

   // return default icon
   return mDefaultIconMap[sFileName];
}
