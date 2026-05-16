/***************************************************************************
                          RecentFileList.h  -  description
                             -------------------
    begin                : Sun Feb 09 2003
    copyright            : (C) 2003 by Alexander Theel
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

#include <QMenu>
#include <QAction>
//Added by qt3to4:
#include <QActionGroup>

class RecentFileList : public QObject {
  Q_OBJECT
public:
  RecentFileList(QWidget* parent, QMenu* menu, QString files="");
  
  void setOnTop(QString absPath);

  QString toString();

private:
  QWidget* parent;
  QMenu* menu;
  QStringList fileList;
  QActionGroup * recentFileGroup;
  QMenu* recentlyFilesMenu;

  static /*const*/ QString separator;
  static /*const*/ uint MAX_ELEMENT_COUNT;

  void setList(QString files);
  void checkSize();
  void update();
  void updateMenu();
  
  void remove(QString absPath);

private slots:
  void slotRecenlyOpenedFilesActivated( QAction* action );
  
signals:
  void openFile(QString fileName);
};

#endif
