/***************************************************************************
                          treeelement.cpp  -  description
                             -------------------
    begin                : Fri Jul 19 2002
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
#include <iostream>

#include "CTreeElement.h"
#include "./dialogs/searchlistitem.h"

#include "../global.h"
#include <QPixmap>
#include <QIcon>
#include <QBrush>
#include <QList>

CTreeElement::CTreeElement( QTreeWidget* pParent, CTreeInformationElement& element )
  : QObject()
  , QTreeWidgetItem( pParent )
  , mpInformationElement( nullptr )
{
   init(element);
}

CTreeElement::CTreeElement( CTreeElement* pParent, CTreeInformationElement& element )
  : QObject()
  , QTreeWidgetItem( pParent )
  , mpInformationElement( nullptr )
{
   init(element);
}

CTreeElement::~CTreeElement( void )
{
   mpInformationElement = nullptr;
}

void CTreeElement::init( CTreeInformationElement& element )
{
  mpInformationElement = &element;
  setFlags(flags() | Qt::ItemIsEditable);
  copyPropertiesFromInformationElement();
  connect( &element, SIGNAL(propertyChanged()), this, SLOT(propertyChangeEvent()) );
  connect( &element, SIGNAL(childAdded(CInformationElement*)), this, SLOT(childAddEvent(CInformationElement*)) );
  connect( &element, SIGNAL(childMoved(int, int)), this, SLOT(childMovedEvent(int, int)) );
}

CTreeElement* CTreeElement::getLastChild( void )
{
   int n = childCount();
   if ( n == 0 )
      return nullptr;
   return dynamic_cast<CTreeElement*>( child(n-1) );
}

CTreeInformationElement* CTreeElement::getInformationElement( void )
{
   return mpInformationElement;
}

void CTreeElement::propertyChangeEvent( void )
{
   copyPropertiesFromInformationElement();
}

void CTreeElement::copyPropertiesFromInformationElement( void )
{
   if ( nullptr == mpInformationElement )
      return;

   QTreeWidget* tw = treeWidget();
   bool oldBlock = tw ? tw->blockSignals(true) : false;

   setText( 0, mpInformationElement->getDescription() );
   QPixmap pix( mpInformationElement->getIconFileName() );
   if (!pix.isNull())
      setIcon( 0, QIcon(pix) );
   setExpanded( mpInformationElement->isOpen() );
   QColor c = mpInformationElement->getTextColor();
   if ( c.isValid() )
      setForeground( 0, QBrush( c ) );

   if ( tw )
      tw->blockSignals(oldBlock);
}

void CTreeElement::childAddEvent( CInformationElement* pChild )
{
   if ( nullptr == pChild )
      return;

   CTreeElement* pNewElement = new CTreeElement(this, *((CTreeInformationElement*)pChild));

   QList<CInformationElement*>* pList = pChild->getChildren();
   for ( CInformationElement* pX : *pList )
   {
      pNewElement->childAddEvent(pX);
   }

   if ( pNewElement->treeWidget() )
      pNewElement->treeWidget()->setCurrentItem( pNewElement );
}

void CTreeElement::childMovedEvent( int oldPos, int newPos )
{
  if ( oldPos == newPos )
    return;

  CTreeElement* pElementToMove = getChildAtPosition(oldPos);
  if ( nullptr == pElementToMove )
    return;

  takeChild( oldPos );
  insertChild( newPos, pElementToMove );
}

CTreeElement* CTreeElement::getChildAtPosition( int pos )
{
  if ( pos < 0 || pos > childCount()-1 )
    return nullptr;

  return dynamic_cast<CTreeElement*>( child(pos) );
}

bool CTreeElement::isChildOrSelfSelected( void )
{
   if (isSelected())
      return true;

   for (int i = 0; i < childCount(); ++i)
   {
      CTreeElement* pElem = dynamic_cast<CTreeElement*>( child(i) );
      if ( pElem && pElem->isChildOrSelfSelected() )
         return true;
   }

   return false;
}

void CTreeElement::search( QString pattern, bool recursive, bool caseSensitive,
                          bool SearchOnlyTitles, QTreeWidget& list )
{
   if ( nullptr == mpInformationElement )
      return;

   mpInformationElement->search(pattern, recursive, caseSensitive,
                                SearchOnlyTitles, list);
}

CTreeElement* CTreeElement::findChildWithName( QString name )
{
  for (int i = 0; i < childCount(); ++i)
  {
    QTreeWidgetItem* pX = child(i);
    if ( pX && pX->text(0) == name )
      return dynamic_cast<CTreeElement*>(pX);
  }

  return nullptr;
}

CTreeElement* CTreeElement::firstChildElem() const
{
   if ( childCount() == 0 )
      return nullptr;
   return dynamic_cast<CTreeElement*>( child(0) );
}

CTreeElement* CTreeElement::nextSiblingElem() const
{
   QTreeWidgetItem* p = QTreeWidgetItem::parent();
   QTreeWidget* tw = treeWidget();
   if ( p ) {
      int idx = p->indexOfChild( const_cast<CTreeElement*>(this) );
      if ( idx < 0 || idx >= p->childCount()-1 )
         return nullptr;
      return dynamic_cast<CTreeElement*>( p->child(idx+1) );
   } else if ( tw ) {
      int idx = tw->indexOfTopLevelItem( const_cast<CTreeElement*>(this) );
      if ( idx < 0 || idx >= tw->topLevelItemCount()-1 )
         return nullptr;
      return dynamic_cast<CTreeElement*>( tw->topLevelItem(idx+1) );
   }
   return nullptr;
}

CTreeElement* CTreeElement::itemAboveElem() const
{
   QTreeWidget* tw = treeWidget();
   if ( !tw )
      return nullptr;
   QTreeWidgetItem* prev = tw->itemAbove( const_cast<CTreeElement*>(this) );
   return dynamic_cast<CTreeElement*>( prev );
}
