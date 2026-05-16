/***************************************************************************
                          search.h  -  description
                             -------------------
    begin                : Sun Mar 26 2000
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
#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>

#include <QString>
#include "searchlistitem.h"

#include "../CTreeElement.h"

class SearchDialog : public QDialog{
  Q_OBJECT
public:
  SearchDialog( QWidget* pParent );

  int          setUp( CTreeElement* rootTreeElement, CTreeElement* activeTreeElement );
  QString      getText( void );

public slots:
  virtual void startSearching( void );
  virtual void selectionChange( QTreeWidgetItem* x );
  virtual void close( void );
  void         toggleMore( bool );

signals:
  void         makeVisible(SearchPosition*);

private:
  int          whatMode( void );

  QLineEdit*    edit;
  CTreeElement* rootTreeElement;
  CTreeElement* activeTreeElement;
  QLabel*       status;
  QCheckBox*    caseSensitive;
  QCheckBox*    searchTitles;
  QPushButton*  moreBtn;
  QGroupBox*    moreBox;
  QRadioButton* rbWholeTree;
  QRadioButton* rbActiveAndChildren;
  QRadioButton* rbActiveOnly;

  QTreeWidget*   list;
};

#endif
