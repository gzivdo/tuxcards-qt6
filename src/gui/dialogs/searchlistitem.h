/***************************************************************************
                          searlistitem.h  -  description
                             -------------------
    begin                : Fri Mar 31 2000
    copyright            : (C) 2000 by Alexander Theel
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

#ifndef SEARCHLISTITEM_H
#define SEARCHLISTITEM_H

#include <QTreeWidget>
#include <QHeaderView>
#include <QString>
#include "searchposition.h"

#define MAX_SEARCHLIST_STRLEN	80

class SearchListItem : public QTreeWidgetItem{
public:

  SearchListItem(QTreeWidget *parent, Path* path, int location,
                 int line, int pos, int len, QString s);

  SearchPosition* getSearchPosition();



protected:

  QString toString();

  SearchPosition* searchPosition;

};

#endif
