/***************************************************************************
                          searlistitem.cpp  -  description
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

#include "searchlistitem.h"
#include <QFont>
#include <iostream>

SearchListItem::SearchListItem(QTreeWidget *parent, Path* path, int location,
                               int line, int pos, int len, QString s)
    : QTreeWidgetItem(parent)
{
    setText(0, path->getPathList().last());
    QString content = (location == SearchPosition::SP_NAME) ? QString("") : s;
    setText(1, content);

    // Carry match position so SearchHighlightDelegate can render it bold.
    if (location != SearchPosition::SP_NAME) {
        setData(1, Qt::UserRole + 1, pos);
        setData(1, Qt::UserRole + 2, len);
    }

    searchPosition = new SearchPosition(path, location, line, pos, len, s);
}


SearchPosition* SearchListItem::getSearchPosition()
{
  return searchPosition;
}

QString SearchListItem::toString(){
  return text(0)
        +": line="+QString::number(searchPosition->getLine())
        +" pos="+QString::number(searchPosition->getPos());
}
