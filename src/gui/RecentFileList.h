/***************************************************************************
                          RecentFileList.h  -  description
                             -------------------
    begin                : Sun Feb 09 2003
    copyright            : (C) 2003 by Alexander Theel
                           (Qt6 port adapted from TuxCards 2.2.1)
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

#ifndef RECENTFILELIST_H
#define RECENTFILELIST_H

#include <QObject>
#include <QStringList>
#include <QList>

class QMenu;
class QComboBox;
class QAction;
class QWidget;


/**
 * Recent file list — keeps a small history of recently opened files
 * and exposes them as a "Recently Used Files" sub-menu (and optionally
 * as a QComboBox).
 *
 * Adapted from TuxCards 2.2.1: the menu items are pre-created as hidden
 * QActions and merely made visible when a file is added — same QAction
 * objects are reused, no live churn while the menu is open.
 */
class RecentFileList : public QObject {
   Q_OBJECT
public:
   RecentFileList( QWidget* pParent, QMenu* pParentMenu, const QString& sFiles = "" );

   void    setOnTop( const QString& sAbsPath );

   void    createComboBox( QWidget& parentWidget );

   QString toString() const;

private:
   QMenu*       mpParentMenu;
   QStringList  mFileList;

   QMenu*       mpRecentlyFilesMenu;
   QComboBox*   mpComboBox;

   static const QString SEPARATOR;
   static const int     MAX_ELEMENT_COUNT;

   QList<QAction*> mRecentFileActs;

   void createActions( QWidget* pParent );

   void setList( const QString& sFiles );
   void checkSize();
   void update();
   void updateMenu();
   void updateComboBox();

   void remove( const QString& sAbsPath );

private slots:
   void slotOpenRecentFile();
   void slotComboActivated( int idx );

signals:
   void openFile( QString sFileName );
};

#endif
