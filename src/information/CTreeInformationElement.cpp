/***************************************************************************
                          CTreeInformationElement.cpp  -  description
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

#include "CTreeInformationElement.h"
#include "../global.h"
#include "../utilities/crypt/StringCrypter.h"

// -------------------------------------------------------------------------------
CTreeInformationElement::CTreeInformationElement( CInformationElement* pParent,
                         QString description,
                         QString information,
                         InformationFormat* pFormat,
                         QString sIconFileName,
                         bool bOpen )
 : CInformationElement( pParent, description, information, pFormat, sIconFileName)
// -------------------------------------------------------------------------------
{
   setOpen( bOpen );
}


/**
 * Adds the CTreeInformationElement 'element' to this element.
 */
// -------------------------------------------------------------------------------
void CTreeInformationElement::addChild( CTreeInformationElement* pElement )
// -------------------------------------------------------------------------------
{
   if ( nullptr == pElement )
      return;

   mpChildObjects->append( pElement );
   pElement->setParent( this );

   emit childAdded( pElement );
//  if (!mbBatched) emit(childAdded(element));
}


// -------------------------------------------------------------------------------
bool CTreeInformationElement::isOpen( void )
// -------------------------------------------------------------------------------
{
   return mbOpen;
}


// -------------------------------------------------------------------------------
void CTreeInformationElement::setOpen( bool bOpen )
// -------------------------------------------------------------------------------
{
   mbOpen = bOpen;
   if (!mbBatched) emit propertyChanged();
}


// -------------------------------------------------------------------------------
QString CTreeInformationElement::toStringObsoleted( void )
// -------------------------------------------------------------------------------
{
  QString result="";
  QString h;

  // get own name
  h = mDescription;
  result += QString::number(h.length()) + "*" + h;

  // get own text
  h = mInformation;
  result += QString::number(h.length()) + "*" + h;

  // remember the expanded/collapsed state of the node
  h = ( isOpen() ? "-" : "+" );
  result += h;

  // include iconFilename
  result += mIconFilename+"*";

  int n = childCount();
  result += QString::number(n);

  for (CInformationElement* __ie : *getChildren()) {
    CTreeInformationElement* x = (CTreeInformationElement*)__ie;
    result += x->toString();
  }

  // add terminationString & header
  result= "*" +result+ "***";
  result= "***" + QString::number(result.length()) + result;

  return result;
}


// -------------------------------------------------------------------------------
void CTreeInformationElement::toXML( QDomDocument xmlDocument, QDomNode parent )
// -------------------------------------------------------------------------------
{
   QDomElement thisElement = xmlDocument.createElement("InformationElement");
   thisElement.setAttribute("informationFormat", getInformationFormat()->toString());
   thisElement.setAttribute("iconFileName", getIconFileName());
   thisElement.setAttribute("isOpen", isOpen() ? "true" : "false" );
   thisElement.setAttribute("isEncripted", isEncryptionEnabled() ? "true" : "false" );

	// Set the tree item and subtree text color
	QString strTextColor;
   if(msubtreeTextColor != Qt::black) {
		strTextColor = QString("%1:%2:%3")
								.arg(msubtreeTextColor.red())
								.arg(msubtreeTextColor.green())
								.arg(msubtreeTextColor.blue());

	   thisElement.setAttribute("subtreeTextColor", strTextColor);

   }

	strTextColor = QString("%1:%2:%3")
							.arg(mtextColor.red())
							.arg(mtextColor.green())
							.arg(mtextColor.blue());

   thisElement.setAttribute("textColor", strTextColor);

//   std::cout << "CTreeInformationElement::toXML: Text color: saved " << strTextColor.toStdString() <<
//		   "was " << mtextColor.name().toStdString() << std::endl;

   // add description
   QDomElement description = xmlDocument.createElement("Description");
   QDomText text = xmlDocument.createTextNode(getDescription());
   description.appendChild(text);
   thisElement.appendChild(description);

   // add information
   QDomElement information = xmlDocument.createElement("Information");
   if ( isEncryptionEnabled() )
   {
      // Decide whether we can reuse the on-disk ciphertext (mOriginalBlob)
      // or must run the cipher again:
      //  - dirty element → must re-encrypt
      //  - empty mOriginalBlob → nothing to reuse, must encrypt
      //  - sReencryptOnFormatChange && original blob's format !=
      //    currently configured backend → must re-encrypt to migrate
      //  - otherwise → emit mOriginalBlob unchanged (no cipher call).
      QByteArray blob;
      const QByteArray& orig = getOriginalBlob();
      bool canReuse = !isDirty() && orig.size() > 0;
      if ( canReuse && sReencryptOnFormatChange ) {
         const int origFmt   = StringCrypter::identifyBlobFormat(orig);
         const int wantFmt   = StringCrypter::getWriteBackend();
         if ( origFmt != wantFmt )
            canReuse = false;
      }

      if ( canReuse ) {
         blob = orig;
      } else if ( isCurrentlyEncrypted() ) {
         blob = getEncryptedData();
         setOriginalBlob(blob);
         markClean();
      } else {
         StringCrypter::encryptString( getInformation(), msTmpPasswd, blob );
         setOriginalBlob(blob);
         markClean();
      }

      QString sB64Representation (blob.toBase64().constData());
      text = xmlDocument.createCDATASection( sB64Representation );
   }
   else
   {
      //text = xmlDocument.createTextNode( getInformation() );
      text = xmlDocument.createCDATASection( getInformation() );
   }

   information.appendChild(text);
   thisElement.appendChild(information);


   // add children
   for (CInformationElement* x : *(mpChildObjects)) {
      x->toXML(xmlDocument, thisElement);
   }

   parent.appendChild(thisElement);
}


/**
 * Move this element one position upwards within the sibling
 * list. If moving upwards is not possible nothing is done.
 */
// -------------------------------------------------------------------------------
void CTreeInformationElement::moveOneUp( void )
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpParent )
   {
      std::cout<<"parent == 0 -> moving not possible"<<std::endl;
      return;
   }

   ((CTreeInformationElement*)mpParent)->moveChildOneUp(this);
}


/**
 * Move the specified child element one position upwards within
 * children list. If moving upwards is not possible or the given
 * element is not a child of this element nothing is done.
 */
// -------------------------------------------------------------------------------
void CTreeInformationElement::moveChildOneUp( CTreeInformationElement* pChild )
// -------------------------------------------------------------------------------
{
   int pos = mpChildObjects->indexOf( pChild );
   if ( (pos == -1) || (pos == 0) )
   {
      return;
   }

   mpChildObjects->removeAt( pos );
   mpChildObjects->insert( pos-1, pChild );

   if (!mbBatched) emit childMoved(pos, pos-1);
}

/**
 * Move this element one position upwards within the sibling
 * list. If moving upwards is not possible nothing is done.
 */
// -------------------------------------------------------------------------------
void CTreeInformationElement::moveOneDown( void )
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpParent )
   {
      std::cout<<"parent == 0 -> moving not possible"<<std::endl;
      return;
   }

   ((CTreeInformationElement*)mpParent)->moveChildOneDown(this);
}

/**
 * Move the specified child element one position downwards within
 * children list. If moving downwards is not possible or the given
 * element is not a child of this element nothing is done.
 */
// -------------------------------------------------------------------------------
void CTreeInformationElement::moveChildOneDown( CTreeInformationElement* pChild )
// -------------------------------------------------------------------------------
{
   int pos = mpChildObjects->indexOf( pChild );
   if ( (pos == -1) || (pos == childCount()-1) )
   {
      return;
   }

   mpChildObjects->removeAt( pos );
   mpChildObjects->insert( pos+1, pChild );

   if (!mbBatched) emit childMoved(pos, pos+1);
}

