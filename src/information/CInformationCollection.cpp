/***************************************************************************
                          CInformationCollection.cpp  -  description
                             -------------------
    begin                : Fri Jul 19 2002
    copyright            : (C) 2002 by alex, (c) 2006-2009 Amit Chaudhary
    email                : alex.theel@gmx.net, amitch@rajgad.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CInformationCollection.h"

#include "CTreeInformationElement.h"
#include <QDomDocument>
#include <QDomElement>


// -------------------------------------------------------------------------------
CInformationCollection::CInformationCollection( CInformationElement* pRoot )
 : mpRootElement( nullptr )
 , mpActiveElement( nullptr ), mbEncrypted(false), mstrFilePassword(""),
	mNumElements(0)
// -------------------------------------------------------------------------------
{
  setRootElement( pRoot );
  setActiveElement( pRoot );
}


// -------------------------------------------------------------------------------
CInformationCollection::~CInformationCollection( void )
// -------------------------------------------------------------------------------
{
   DELETE( mpRootElement );
   mpActiveElement = nullptr;

   mViews.clear();
}


// -------------------------------------------------------------------------------
CInformationCollection* CInformationCollection::createDefaultCollection( void )
// -------------------------------------------------------------------------------
{
  CTreeInformationElement* pE = new CTreeInformationElement(0, "root", "", &InformationFormat::RTF);
  CInformationCollection* pCollection = new CInformationCollection(pE);
  pCollection->registerAsListenerOf(pE);
  return pCollection;
}



// **************************** IParent ******************************************
// -------------------------------------------------------------------------------
QString CInformationCollection::getDescription( void )
// -------------------------------------------------------------------------------
{
  return INFORMATION_COLLECTION_DESC;
}
// -------------------------------------------------------------------------------
IParent* CInformationCollection::getParent( void )
// -------------------------------------------------------------------------------
{
   return nullptr;
}

// -------------------------------------------------------------------------------
void CInformationCollection::removeChild( CInformationElement* /*pChild*/ )
// -------------------------------------------------------------------------------
{
   // Root element cannot be deleted; do nothing.
}

// -------------------------------------------------------------------------------
void CInformationCollection::aboutToRemoveElement( CInformationElement* pIE )
// -------------------------------------------------------------------------------
{
	notifyViewsToRemoveElement( pIE );
	mNumElements--;
	emit numElementsChanged( mNumElements );
}
// **************************** IParent - End ************************************


// -------------------------------------------------------------------------------
void CInformationCollection::setRootElement( CInformationElement* pRoot )
// -------------------------------------------------------------------------------
{
   mpRootElement = pRoot;

   if ( nullptr != pRoot )
      pRoot->setParent(this);
}


// -------------------------------------------------------------------------------
CInformationElement* CInformationCollection::getRootElement( void )
// -------------------------------------------------------------------------------
{
  return mpRootElement;
}


// -------------------------------------------------------------------------------
void CInformationCollection::setActiveElement( CInformationElement* pElement )
// -------------------------------------------------------------------------------
{
   if (!pElement)
      return;

   mpActiveElement = pElement;
   emit activeInformationElementChanged( pElement );
}


// -------------------------------------------------------------------------------
void CInformationCollection::setActiveElement( Path path )
// -------------------------------------------------------------------------------
{
   CInformationElement* pElement = getInformationElement(path);

   if ( nullptr == pElement )
   {
      setActiveElement( mpRootElement );
      return;
   }

   setActiveElement( pElement );
}


// -------------------------------------------------------------------------------
CInformationElement* CInformationCollection::getActiveElement( void )
// -------------------------------------------------------------------------------
{
  return mpActiveElement;
}


// -------------------------------------------------------------------------------
QString CInformationCollection::toString( void )
// -------------------------------------------------------------------------------
{
  //cout<<"printing collection with rootElement("<<mpRootElement<<")"<<endl;
  if ( nullptr == mpRootElement )
   return "";

  return mpRootElement->getTreeString(0);
}


// -------------------------------------------------------------------------------
QString CInformationCollection::toXML( void )
// -------------------------------------------------------------------------------
{

  QDomDocument xmlDocument("tuxcards_data_file");
  xmlDocument.insertBefore( xmlDocument.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"utf-8\"" ), xmlDocument.documentElement() );


  // CInformationCollection-MetaData
  QDomElement thisElement = xmlDocument.createElement("InformationCollection");
  thisElement.setAttribute("isEncrypted", isEncrypted() ? QString("true") : QString("false") );
  xmlDocument.appendChild( thisElement );

  QDomElement pathOfLastActiveIE = xmlDocument.createElement("LastActiveElement");
  QDomText text = xmlDocument.createTextNode( Path(mpActiveElement).toString() );
  pathOfLastActiveIE.appendChild( text );
  thisElement.appendChild( pathOfLastActiveIE );

  //std::cout<<"LastActiveElement = "<<Path(mpActiveElement).toString()<<std::endl;

  if ( nullptr != mpRootElement )
  {
    //std::cout<<"root != 0"<<std::endl;
    mpRootElement->toXML(xmlDocument, thisElement);
  }

  return xmlDocument.toString();
}



/**
 * Allows to create an XML-document with an arbitrary informationelement
 * as root.
 */
// -------------------------------------------------------------------------------
QString CInformationCollection::toXML( CInformationElement* pElem )
// -------------------------------------------------------------------------------
{
   QDomDocument xmlDocument("tuxcards_data_file");
   xmlDocument.insertBefore( xmlDocument.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"utf-8\"" ), xmlDocument.documentElement() );
   if ( nullptr != pElem )
      pElem->toXML( xmlDocument, xmlDocument );

   return xmlDocument.toString();
}

void CInformationCollection::setEncrypted(bool val)
{
	mbEncrypted = val;
}

bool CInformationCollection::isEncrypted()
{
	return mbEncrypted;
}


// -------------------------------------------------------------------------------
void CInformationCollection::slotChildAdded( CInformationElement* pElem )
// -------------------------------------------------------------------------------
{
	// If current collection (file) is encrypted, mark this new node as encrypted too.
	if (isEncrypted()) {
		pElem->enableEncryption(true, mstrFilePassword);
		// Needed for dropped items. isCurrentlyEncrypted() is for encrypted files
		if (pElem->isCurrentlyEncrypted()) {
			pElem->decryptTree(mstrFilePassword);
		}
	}

	registerAsListenerOf( pElem );
	emit modelHasChanged();

}


// -------------------------------------------------------------------------------
void CInformationCollection::registerAsListenerOf( CInformationElement* pElem )
// -------------------------------------------------------------------------------
{
   if ( nullptr == pElem )
      return;

	mNumElements++;

   connect( pElem, SIGNAL(propertyChanged()), this, SIGNAL(modelHasChanged()) );
   connect( pElem, SIGNAL(childAdded(CInformationElement*)), this, SLOT(slotChildAdded(CInformationElement*)) );

	emit numElementsChanged( mNumElements );
}


// -------------------------------------------------------------------------------
CInformationElement* CInformationCollection::getInformationElement( Path path )
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpRootElement )
      return nullptr;

   QStringList list = path.getPathList();
   if ( mpRootElement->getDescription() != list[0] )
      return nullptr;

   CInformationElement* pElem = mpRootElement;
   for ( uint i=1; i < list.size(); i++ )
   {
      pElem = pElem->findChildWithDescription( list[i] );
      if ( nullptr == pElem )
      {
         return nullptr;
      }
   }

   return pElem;
}


/**
 * Checks whether the given path is not empty and valid
 * within this information collection.
 */
// -------------------------------------------------------------------------------
bool CInformationCollection::isPathValid( Path path )
// -------------------------------------------------------------------------------
{
  if ( path.isEmpty() )
    return false;

  CInformationElement* pElement = getInformationElement(path);
  if ( pElement )
    return true;

  return false;
}


// -------------------------------------------------------------------------------
void CInformationCollection::addView( IView* pView )
// -------------------------------------------------------------------------------
{
   mViews.append( pView );
}

// -------------------------------------------------------------------------------
void CInformationCollection::removeView( IView* pView )
// -------------------------------------------------------------------------------
{
   mViews.removeAll( pView );
}


// -------------------------------------------------------------------------------
void CInformationCollection::notifyViewsToRemoveElement( CInformationElement* pIE )
// -------------------------------------------------------------------------------
{
   for (IView* pView : mViews)
   {
      pView->aboutToRemoveElement( pIE );
   }
}

// Change the current state of encryption flag.
// -------------------------------------------------------------------------------
void CInformationCollection::toggleEncryption(QString& password)
{
	// Change the current state
	mbEncrypted = !mbEncrypted;

	// Save the password
	mstrFilePassword = password;

	if (mbEncrypted)
		mpRootElement->enableEncryptionForElementTree(password);
	else
		mpRootElement->disableEncryptionForElementTree();
}

// Will return true, if any of the element in the tree is encrypted.
// -------------------------------------------------------------------------------
bool CInformationCollection::checkEncryptionForElementTree()
{
	return mpRootElement->checkEncryptionForElementTree();
}

// Decrypt the complete tree
// -------------------------------------------------------------------------------
bool CInformationCollection::firstEncryptedBlob(QByteArray& out) const
{
   if ( !mpRootElement ) return false;
   return mpRootElement->firstEncryptedBlob(out);
}

// -------------------------------------------------------------------------------
bool  CInformationCollection::decryptTree(QString password)
{
	bool retval = mpRootElement->decryptTree(password);

	// If successful, save the password
	if(retval)
		mstrFilePassword = password;

	return retval;
}
