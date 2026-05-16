/***************************************************************************
                          CTree.cpp  -  description
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

#include "CTree.h"

#include "../icons/delete.xpm"
#include "../icons/changeProperty.xpm"
#include "../icons/addTreeElement.xpm"
#include "../icons/upArrow.xpm"
#include "../icons/downArrow.xpm"

#include <QCursor>
#include <QHeaderView>
#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QDragLeaveEvent>
#include <QKeyEvent>
#include <QPixmap>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QMimeData>
#include <QDrag>
#include <QScrollBar>

#include "../information/xmlpersister.h"

#include "../utilities/CIconManager.h"
#define  getIcon(x)  CIconManager::getInstance().getIcon(x)


CTree::CTree( QWidget* pParent, CTuxCardsConfiguration& refTuxConfiguration )
  : QTreeWidget( pParent )
  , mpCollection( NULLPTR )
  , mContextMenu( pParent )
  , mPropertyDialog( pParent, refTuxConfiguration )
  , mSearchDialog( pParent )
  , mPressPos()
  , mbMousePressed( false )
  , mAutoOpenTimer( pParent )
  , mpOldCurrent( NULLPTR )
  , mpDropElement( NULLPTR )
  , miAutoOpenTime( 750 )
{
  setColumnCount(1);
  setHeaderLabel("");
  setSortingEnabled(false);
  setRootIsDecorated(true);

  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

  setContextMenuPolicy(Qt::CustomContextMenu);

  mAutoOpenTimer.setSingleShot(true);
  connect( &mAutoOpenTimer, SIGNAL(timeout()), this, SLOT(timeoutEvent()) );

  connect( this, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
           this, SLOT(currentItemChangedSlot(QTreeWidgetItem*, QTreeWidgetItem*)) );
  connect( this, SIGNAL(customContextMenuRequested(const QPoint&)),
           this, SLOT(showContextMenu(const QPoint&)) );
  connect( this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
           this, SLOT(elementOpenedEvent(QTreeWidgetItem*)) );
  connect( this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
           this, SLOT(elementClosedEvent(QTreeWidgetItem*)) );
  connect( this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
           this, SLOT(inPlaceRenaming(QTreeWidgetItem*, int)) );

  connect( &mSearchDialog, SIGNAL(makeVisible(SearchPosition*)),
           this,           SIGNAL(makeVisible(SearchPosition*)) );

  settingUpContextMenu();
}


CTree::~CTree( void )
{
   mpCollection = NULLPTR;
   mpOldCurrent = NULLPTR;
   mpDropElement= NULLPTR;
}

void CTree::timeoutEvent( void )
{
   mAutoOpenTimer.stop();
   if ( NULLPTR == mpDropElement )
      return;

   if ( !mpDropElement->isExpanded() )
   {
      mpDropElement->setExpanded( true );
   }
}


void CTree::aboutToRemoveElement( CInformationElement* pIE )
{
   if ( (NULLPTR != mpOldCurrent) && (mpOldCurrent->getInformationElement() == pIE) )
      mpOldCurrent = NULLPTR;

   if ( (NULLPTR != mpDropElement) && (mpDropElement->getInformationElement() == pIE) )
      mpDropElement = NULLPTR;
}



void CTree::setColumnText(QString text)
{
   setHeaderLabel(text);
}

void CTree::settingUpContextMenu( void )
{
  mContextMenu.addAction( getIcon("addTreeElement"), "&Add Entry...",        this, SLOT(addElement()) );
  mContextMenu.addAction( "&Rename Entry",                                   this, SLOT(renameElement()) );
  mContextMenu.addAction( getIcon("changeProperty"), "Change Properties...", this, SLOT(changeActiveElementProperties()) );
  mContextMenu.addAction( getIcon("delete"),         "Delete Entry",         this, SLOT(askForDeletion()) );
  mContextMenu.addSeparator();
  mContextMenu.addAction( getIcon("editentrycolor"), "Set &Entry Color...",  this, SLOT(setEntryColor()) );
  mContextMenu.addAction( getIcon("editentrysubtreecolor"), "Set Entry &Sub-Tree Color...", this, SLOT(setEntrySubTreeColor()) );
  mContextMenu.addSeparator();
  mContextMenu.addAction( getIcon("upArrow"),        "Move Entry Upwards",   this, SLOT(moveElementUp()) );
  mContextMenu.addAction( getIcon("downArrow"),      "Move Entry Downwards", this, SLOT(moveElementDown()) );
  mContextMenu.addSeparator();
  mContextMenu.addAction( "Add to &Bookmarks",                                this, SLOT(addEntryToBookmarks()) );
}

void CTree::currentItemChangedSlot( QTreeWidgetItem* pItem, QTreeWidgetItem* /*previous*/ )
{
   if ( (NULLPTR == mpCollection) || (NULLPTR == pItem) )
      return;

   CTreeElement* pTreeElem = dynamic_cast<CTreeElement*>(pItem);
   if ( !pTreeElem )
      return;

   mpCollection->setActiveElement(pTreeElem->getInformationElement());
}


void CTree::showContextMenu( const QPoint& pos )
{
   QTreeWidgetItem* pItem = itemAt(pos);
   if ( NULLPTR == pItem )
      return;

   mContextMenu.popup( viewport()->mapToGlobal(pos) );
}

void CTree::elementOpenedEvent( QTreeWidgetItem* pItem )
{
   CTreeElement* pTreeElem = dynamic_cast<CTreeElement*>(pItem);
   if ( !pTreeElem )
      return;

   pTreeElem->getInformationElement()->setOpen(true);
}

void CTree::elementClosedEvent( QTreeWidgetItem* pItem )
{
   CTreeElement* pTreeElem = dynamic_cast<CTreeElement*>(pItem);
   if ( !pTreeElem )
      return;

   pTreeElem->getInformationElement()->setOpen(false);
}


/******************************************************************************/

void CTree::mousePressEvent( QMouseEvent* pE )
{
   if ( NULLPTR == pE )
      return;

   QTreeWidget::mousePressEvent(pE);
   if (Qt::RightButton == pE->button())
      return;

   CTreeElement* pItem = dynamic_cast<CTreeElement*>( itemAt(pE->pos()) );
   if ( NULLPTR == pItem )
      return;

   mPressPos = pE->pos();
   mbMousePressed = true;
}



void CTree::mouseMoveEvent( QMouseEvent* pE )
{
   if ( (NULLPTR == mpCollection) || (NULLPTR == pE) )
      return;

   if ( mbMousePressed &&
        (mPressPos - pE->pos()).manhattanLength() > QApplication::startDragDistance() )
   {
      mbMousePressed = false;
      QTreeWidgetItem* pItem = itemAt(mPressPos);
      if ( NULLPTR != pItem )
      {
         emit dragStarted();

         QDrag* pDrag = new QDrag(this);
         QMimeData* mimeData = new QMimeData;

         mimeData->setText(mpCollection->toXML(getCurrentActive()));
         pDrag->setMimeData(mimeData);

         pDrag->exec(Qt::MoveAction | Qt::CopyAction);

         pE->accept();
      }
   }
}



void CTree::mouseReleaseEvent( QMouseEvent* pE )
{
   mbMousePressed = false;
   QTreeWidget::mouseReleaseEvent(pE);
}

/******************************************************************************/

void CTree::dragEnterEvent( QDragEnterEvent* pE )
{
   if ( NULLPTR == pE )
      return;

   if (!pE->mimeData()->hasText())
   {
      pE->ignore();
      return;
   }

   mpOldCurrent = dynamic_cast<CTreeElement*>( currentItem() );

   CTreeElement* pElement = dynamic_cast<CTreeElement*>( itemAt(pE->position().toPoint()) );
   if ( NULLPTR != pElement )
   {
      mpDropElement = pElement;
      mAutoOpenTimer.start(miAutoOpenTime);
   }

   pE->acceptProposedAction();
}

void CTree::dragMoveEvent( QDragMoveEvent* pE )
{
   if ( NULLPTR == pE )
      return;

   if (!pE->mimeData()->hasText())
   {
      pE->ignore();
      return;
   }

   CTreeElement* pElement = dynamic_cast<CTreeElement*>( itemAt(pE->position().toPoint()) );
   if ( NULLPTR != pElement )
   {
      setCurrentItem(pElement);
      pE->setAccepted(true);
      if ( pElement != mpDropElement )
      {
         mAutoOpenTimer.stop();
         mpDropElement = pElement;
         mAutoOpenTimer.start(miAutoOpenTime);
      }
      switch ( pE->dropAction() )
      {
      case Qt::CopyAction:
         break;
      case Qt::MoveAction:
         pE->acceptProposedAction();
         emit showMessage("Move to '"+ (pElement->text(0))+"'", 1);
         break;
      case Qt::LinkAction:
         pE->acceptProposedAction();
         break;
      default:
         ;
      }
   } else {
      pE->ignore();
      mAutoOpenTimer.stop();
      mpDropElement = NULLPTR;
      emit showMessage("", 1);
   }
}


void CTree::dragLeaveEvent( QDragLeaveEvent* )
{
  mAutoOpenTimer.stop();
  mpDropElement = NULLPTR;

  if ( mpOldCurrent ) {
     setCurrentItem( mpOldCurrent );
  }
}


void CTree::dropEvent( QDropEvent* pE )
{
  mAutoOpenTimer.stop();

  if ( (NULLPTR == pE) || (NULLPTR == mpOldCurrent) )
   return;

  if (!pE->mimeData()->hasText())
  {
     pE->ignore();
     return;
  }

  if (pE->source()==this && mpOldCurrent->isChildOrSelfSelected())
  {
    QMessageBox::information( this, "Dragging", "An Entry cannot be moved onto itself or a child." );
    emit showMessage("Move not possible.", 5);
    pE->ignore();
    return;
  }

   CTreeElement* pItem = dynamic_cast<CTreeElement*>( itemAt(pE->position().toPoint()) );
   if ( NULLPTR == pItem )
   {
      pE->ignore();
      return;
   }

   switch ( pE->dropAction() )
   {
   case Qt::CopyAction:
      break;
   case Qt::MoveAction:
      pE->acceptProposedAction();
      break;
   case Qt::LinkAction:
      pE->acceptProposedAction();
      break;
   default:
      ;
   }
   pE->setAccepted(true);


   QString collectionString = pE->mimeData()->text();

   CInformationCollection* pCollection = XMLPersister::createInformationCollection(collectionString);
   if ( NULLPTR != pCollection )
   {
     CInformationElement* pRoot = pCollection->getRootElement();
     if ( NULLPTR != pRoot )
     {
        pItem->getInformationElement()->addChild(
                                         dynamic_cast<CTreeInformationElement*>(pRoot) );
     }
   }

   this->deleteElement(mpOldCurrent, false);
}

/******************************************************************************/



CInformationElement* CTree::getCurrentActive( void )
{
   if ( NULLPTR == mpCollection )
      return NULLPTR;

   return mpCollection->getActiveElement();
}


void CTree::removeAll( void )
{
  clearTree();
  setColumnText("");
}

void CTree::clearTree( void )
{
   QTreeWidget::clear();
}


void CTree::createTreeFromCollection( CInformationCollection& collection )
{
   removeAll();

   CTreeInformationElement* pCollectionRootElement = dynamic_cast<CTreeInformationElement*>(collection.getRootElement());
   if ( NULLPTR == pCollectionRootElement )
   {
      std::cout<<"CTree::createTreeFromCollection(): NULLPTR == pCollectionRootElement !"<<std::endl;
      return;
   }
   CTreeElement* pTreeElement = new CTreeElement(this, *pCollectionRootElement);
   for (CInformationElement* __ie : *(pCollectionRootElement->getChildren())) {
      CTreeInformationElement* pX = dynamic_cast<CTreeInformationElement*>(__ie);
      if (!pX) continue;
      addInformationElementsToTreeItem( *pTreeElement, *pX );
   }

   mpCollection = &collection;
   connect( mpCollection, SIGNAL(activeInformationElementChanged(CInformationElement*)),
            this, SLOT(activeInformationElementChanged(CInformationElement*)) );
}

void CTree::addInformationElementsToTreeItem( CTreeElement& parent, CTreeInformationElement& element )
{
  CTreeElement* pTreeElement = new CTreeElement(&parent, element);

  for (CInformationElement* __ie : *(element.getChildren())) {
    CTreeInformationElement* pX = dynamic_cast<CTreeInformationElement*>(__ie);
    if (!pX) continue;
    addInformationElementsToTreeItem( *pTreeElement, *pX );
  }
}


void CTree::activeInformationElementChanged( CInformationElement* pElement )
{
   if ( NULLPTR == pElement )
      return;

   Path path(pElement);
   CTreeElement* pX = getTreeElement(path);

   if ( NULLPTR == pX )
      return;

   if ( getCurrentActiveTreeElement() == pX )
      return;

   if ( getCurrentActiveTreeElement() &&
        pX->text(0) == getCurrentActiveTreeElement()->text(0) )
      return;

   setCurrentItem( pX );
   scrollToItem( pX );
}

CTreeElement* CTree::getTreeElement( Path path )
{
   QStringList list = path.getPathList();
   if (list.isEmpty())
      return NULLPTR;

   CTreeElement* pX = findChildWithName(list[0]);
   if ( NULLPTR == pX )
      return NULLPTR;

   for ( int i=1; i < list.size(); i++ )
   {
      pX = pX->findChildWithName(list[i]);
      if ( NULLPTR == pX )
         return NULLPTR;
   }

   return pX;
}

CTreeElement* CTree::findChildWithName( const QString name )
{
  for (int i = 0; i < topLevelItemCount(); ++i)
  {
     QTreeWidgetItem* pX = topLevelItem(i);
     if ( pX && pX->text(0) == name )
        return dynamic_cast<CTreeElement*>(pX);
  }
  return NULLPTR;
}


// responde to "context-menu" -------------------------------------------
void CTree::addElement( void )
{
  mPropertyDialog.setUp( getCurrentActive(),
                         CPropertyDialog::MODE_CREATE_NEW_ELEMENT );
}


void CTree::changeActiveElementProperties( void )
{
  mPropertyDialog.setUp( getCurrentActive(),
                         CPropertyDialog::MODE_CHANGE_PROPERTIES);
}

void CTree::askForDeletion( void )
{
  if ( (NULLPTR == currentItem()) || (NULLPTR == getCurrentActive()) )
  {
    emit showMessage("Nothing selected.", 5);
    return;
  }

  if (currentItem() == topLevelItem(0))
  {
    QMessageBox::information( this, "Delete the active Entry",
                              "The root entry cannot be deleted." );
    return;
  }

  if (QMessageBox::warning( this, "Delete the active Entry",
                            "Do you really want to delete '"
                            +getCurrentActive()->getDescription()+"'?",
                            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
  {
    return;
  }

  deleteElement( dynamic_cast<CTreeElement*>(currentItem()) );
}

void CTree::deleteElement( CTreeElement* pElem, bool bChangeSelection /*= true*/)
{
   if ( NULLPTR == pElem )
      return;

   if ( pElem == topLevelItem(0) )
      return;

   if (bChangeSelection) {
      QTreeWidgetItem* pAbove = itemAbove(pElem);
      if (pAbove) setCurrentItem(pAbove);
   }

   CInformationElement* pIE = pElem->getInformationElement();
   if ( NULLPTR != pIE )
      pIE->deleteSelf();
   else
      std::cout<<"CTree::deleteElement(): pIE == NULLPTR"<<std::endl;

   DELETE( pElem );
}


CTreeElement* CTree::getCurrentActiveTreeElement( void )
{
  return dynamic_cast<CTreeElement*>( currentItem() );
}

void CTree::search( void )
{
   if (mSearchDialog.setUp( dynamic_cast<CTreeElement*>(topLevelItem(0)),
                            getCurrentActiveTreeElement()) == QDialog::Rejected )
   {
      return;
   }
}


void CTree::resizeEvent( QResizeEvent* pE )
{
   QTreeWidget::resizeEvent(pE);
   if ( NULLPTR == pE )
      return;

   setColumnWidth(0, pE->size().width()-22);
}


void CTree::addEntryToBookmarks( void )
{
  emit addEntryToBookmarksSignal();
}


void CTree::keyPressEvent( QKeyEvent* pK )
{
   if ( NULLPTR == pK )
      return;

   if( ( (pK->modifiers() & Qt::AltModifier)  &&  (pK->key() == Qt::Key_Left) ) ||
       ( (pK->modifiers() & Qt::AltModifier)  &&  (pK->key() == Qt::Key_Right) ) )
   {
      pK->ignore();
      return;
   }

   switch( pK->key() )
   {
   case Qt::Key_Delete:
      askForDeletion();
      break;
   case Qt::Key_Menu:
      mContextMenu.popup( mapToGlobal( QPoint(5,5) ) );
      break;
   case Qt::Key_F2:
      renameElement();
      break;
   default:
      QTreeWidget::keyPressEvent(pK);
  }
}

void CTree::inPlaceRenaming( QTreeWidgetItem* pItem, int /*iCol*/ )
{
   if ( NULLPTR == pItem )
      return;

   CTreeElement* pTreeElement = dynamic_cast<CTreeElement*>(pItem);
   if ( NULLPTR == pTreeElement )
      return;

   CTreeInformationElement* pIE = pTreeElement->getInformationElement();
   if ( NULLPTR == pIE )
      return;

   if ( pIE->getDescription() != pItem->text(0) )
      pIE->setDescription( pItem->text(0) );
}

void CTree::moveElementUp()
{
   CInformationElement* pIE = getCurrentActive();
   CTreeInformationElement* pTreeIE = dynamic_cast<CTreeInformationElement*>(pIE);

   if (pTreeIE)
      pTreeIE->moveOneUp();
}

void CTree::moveElementDown()
{
   CInformationElement* pIE = getCurrentActive();
   CTreeInformationElement* pTreeIE = dynamic_cast<CTreeInformationElement*>(pIE);

   if (pTreeIE)
      pTreeIE->moveOneDown();
}

void CTree::renameElement()
{
   QTreeWidgetItem* pItem = currentItem();
   if ( !pItem )
      return;
   editItem(pItem, 0);
}


void CTree::setEntryColor()
{
   CInformationElement* pElement = getCurrentActive();
   if ( NULLPTR == pElement )
      return;

   QColor c = QColorDialog::getColor(pElement->getTextColor());

   if(!c.isValid()) return;

   pElement->setTextColor(c);
}

void CTree::setEntrySubTreeColor()
{
   CInformationElement* pElement = getCurrentActive();
   if ( NULLPTR == pElement )
      return;

   QColor c = QColorDialog::getColor(pElement->getTextColor());

   if(!c.isValid()) return;

   pElement->setSubTreeTextColor(c);
}
