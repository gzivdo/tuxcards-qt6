/***************************************************************************
                          CTree.h  -  description
                             -------------------
    begin                : Mon Mar 27 2000
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

#ifndef CTREE_H
#define CTREE_H

#include "../information/IView.h"

#include <iostream>
#include <QString>
#include <QTreeWidget>
#include <QHeaderView>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QKeyEvent>
#include <QDragLeaveEvent>

#include "./dialogs/CPropertyDialog.h"
#include "./dialogs/searchdialog.h"
#include <QMenu>
#include <QMessageBox>

#include <QPixmap>

#include <QFrame>
#include <QLineEdit>
#include <QPushButton>

#include "../information/CInformationCollection.h"
#include "../information/CTreeInformationElement.h"
#include "CTreeElement.h"
#include "../fontsettings.h"
#include "../CTuxCardsConfiguration.h"

#include <QTimer>

#include "../information/Path.h"

class CTree : public QTreeWidget,
              public IView
{
  Q_OBJECT
private:
  CInformationCollection* mpCollection;
  QMenu                   mContextMenu;
  CPropertyDialog         mPropertyDialog;
  SearchDialog            mSearchDialog;

  QPoint                  mPressPos;
  bool                    mbMousePressed;
  QTimer                  mAutoOpenTimer;
  CTreeElement*           mpOldCurrent;
  CTreeElement*           mpDropElement;
  const int               miAutoOpenTime;

  CInformationElement*    getCurrentActive( void );
  CTreeElement*           getCurrentActiveTreeElement( void );
  CTreeElement*           getTreeElement(Path path);
  CTreeElement*           findChildWithName(const QString name);
  void                    clearTree( void );
  void                    settingUpContextMenu( void );

  void                    addInformationElementsToTreeItem( CTreeElement&,
                                                            CTreeInformationElement& );

protected:
  void                    mousePressEvent( QMouseEvent* pE ) override;
  void                    mouseMoveEvent( QMouseEvent* pE ) override;
  void                    mouseReleaseEvent( QMouseEvent* pE ) override;

  void                    dragEnterEvent( QDragEnterEvent* pE ) override;
  void                    dragMoveEvent( QDragMoveEvent* pE ) override;
  void                    dragLeaveEvent( QDragLeaveEvent* pE ) override;
  void                    dropEvent( QDropEvent* pE ) override;

  void                    resizeEvent( QResizeEvent* pE ) override;

protected slots:
  void                    elementOpenedEvent( QTreeWidgetItem* pItem );
  void                    elementClosedEvent( QTreeWidgetItem* pItem );
  void                    showContextMenu( const QPoint& pos );
  void                    timeoutEvent( void );
  void                    addEntryToBookmarks( void );

  void                    inPlaceRenaming( QTreeWidgetItem* pItem, int iCol );
  void                    moveElementUp();
  void                    moveElementDown();

  void                    currentItemChangedSlot( QTreeWidgetItem* current, QTreeWidgetItem* previous );

public:
  CTree( QWidget* pParent, CTuxCardsConfiguration& refTuxConfiguration );
  ~CTree( void );
  void                    setColumnText( QString text );
  void                    createTreeFromCollection( CInformationCollection& collection );

  void                    deleteElement( CTreeElement* pElem, bool bChangeSelection = true );

  // ************** IView **********************************
  virtual void aboutToRemoveElement( CInformationElement* pIE ) override;

public slots:
  // "contextmenu"
  void                    addElement( void );
  void                    renameElement( void );
  void                    changeActiveElementProperties( void );
  void                    askForDeletion( void );

  void                    keyPressEvent( QKeyEvent* pK ) override;

  void                    removeAll( void );
  void                    search( void );

  void                    activeInformationElementChanged( CInformationElement* );

  void                    setEntryColor();
  void                    setEntrySubTreeColor();

signals:
  void                    showMessage(QString, int time);
  void                    makeVisible( SearchPosition* );
  void                    addEntryToBookmarksSignal( void );
  void                    dragStarted( void );
};

#endif
