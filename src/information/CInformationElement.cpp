/***************************************************************************
                          CInformationElement.cpp  -  description
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

#include "CInformationElement.h"
#include "../global.h"
#include "../utilities/strings.h"
#include "../gui/dialogs/searchlistitem.h"

#include <QRegExp>
#include <QPixmap>
#include <QList>
#include "../utilities/crypt/StringCrypter.h"


// -------------------------------------------------------------------------------
bool CInformationElement::sReencryptOnFormatChange = false;

// -------------------------------------------------------------------------------
CInformationElement::CInformationElement( IParent* pParent,
                                        QString sDescription,
                                        QString sInformation,
                                        InformationFormat* pFormat,
                                        QString sIconFileName)
 : mpParent( pParent )
 , mbBatched( false )
 , mDescription( sDescription )
 , mpInformationFormat( pFormat )
 , mInformation( sInformation )
 , mIcon()
 , mIconFilename( sIconFileName )
 , mChildObjects()
 , miInformationYPos( 0 )
 , mbIsEncryptionEnabled( false )
 , msTmpPasswd( "" )
 , mEncryptedData()
 , mOriginalBlob()
 , mbDirty( false )
 , mtextColor (Qt::black)
 , msubtreeTextColor (Qt::black)
// -------------------------------------------------------------------------------
{
   if (!mbBatched) emit propertyChanged();
}

// -------------------------------------------------------------------------------
CInformationElement::~CInformationElement( void )
// -------------------------------------------------------------------------------
{
   //std::cout<<"\t~CIE: "<<getDescription()<<std::endl;

   // The legacy Q3PtrList used setAutoDelete(true) to own its elements;
   // QList<T*> does not. We replicate the same ownership semantics with
   // an explicit qDeleteAll here, plus removeAll() + delete in
   // removeChild() — see below.
   qDeleteAll(mChildObjects);
   mChildObjects.clear();

   mpInformationFormat = nullptr;

   if ( nullptr != mpParent )
   {
      mpParent->aboutToRemoveElement(this);
      mpParent = nullptr;
   }
}




// **************************** IParent ******************************************
// -------------------------------------------------------------------------------
QString CInformationElement::getDescription( void )
// -------------------------------------------------------------------------------
{
  return mDescription;
}
// -------------------------------------------------------------------------------
IParent* CInformationElement::getParent( void )
// -------------------------------------------------------------------------------
{
   return mpParent;
}

// -------------------------------------------------------------------------------
void CInformationElement::removeChild( CInformationElement* pChild )
// -------------------------------------------------------------------------------
{
   mChildObjects.removeAll( pChild );
   delete pChild;
}

// -------------------------------------------------------------------------------
void CInformationElement::aboutToRemoveElement( CInformationElement* pIE )
// -------------------------------------------------------------------------------
{
   if ( nullptr != mpParent )
      mpParent->aboutToRemoveElement(pIE);
}
// **************************** IParent - End ************************************



// -------------------------------------------------------------------------------
void CInformationElement::deleteSelf( void )
// -------------------------------------------------------------------------------
{
  //cout<<"IE::deleteSelf(); parent="<<mpParent<<endl;
   if ( nullptr == mpParent )
      return;

   mpParent->removeChild(this);
}

// -------------------------------------------------------------------------------
void CInformationElement::setParent( IParent* pParent )
// -------------------------------------------------------------------------------
{
  mpParent = pParent;
}


// -------------------------------------------------------------------------------
void CInformationElement::setBatched( bool b )
// -------------------------------------------------------------------------------
{
   mbBatched = b;
   if (!mbBatched) emit propertyChanged();
}
// -------------------------------------------------------------------------------
bool CInformationElement::isBatched( void ) const
// -------------------------------------------------------------------------------
{
  return mbBatched;
}

// -------------------------------------------------------------------------------
void CInformationElement::addChild( CInformationElement* pElement )
// -------------------------------------------------------------------------------
{
   if ( nullptr == pElement )
      return;

   mChildObjects.append( pElement );
   // If subtree color is set, set it for this node too
   if(msubtreeTextColor != Qt::black)
	   pElement->setSubTreeTextColor(msubtreeTextColor);

   if (!mbBatched) emit childAdded( pElement );
}
// -------------------------------------------------------------------------------
QList<CInformationElement*>* CInformationElement::getChildren( void )
// -------------------------------------------------------------------------------
{
  return &mChildObjects;
}
// -------------------------------------------------------------------------------
int CInformationElement::childCount( void ) const
// -------------------------------------------------------------------------------
{
   return mChildObjects.count();
}


// -------------------------------------------------------------------------------
void CInformationElement::setDescription( QString description )
// -------------------------------------------------------------------------------
{
  mDescription = description;
  if (!mbBatched) emit propertyChanged();
}

// -------------------------------------------------------------------------------
QString CInformationElement::getInformation( void ) const
// -------------------------------------------------------------------------------
{
  return mInformation;
}

// -------------------------------------------------------------------------------
void CInformationElement::setInformation( const QString& information )
// -------------------------------------------------------------------------------
{
  if ( mInformation != information )
     mbDirty = true;
  mInformation = information;
  if (!mbBatched) emit propertyChanged();
}


// -------------------------------------------------------------------------------
bool CInformationElement::hasIcon( void ) const
// -------------------------------------------------------------------------------
{
  return !mIcon.isNull();
}

// -------------------------------------------------------------------------------
void CInformationElement::setIcon( QPixmap icon )
// -------------------------------------------------------------------------------
{
  mIcon = icon;
  if (!mbBatched) emit propertyChanged();
}
// -------------------------------------------------------------------------------
void CInformationElement::setIconFileName( const QString& file )
// -------------------------------------------------------------------------------
{
  mIconFilename = file;
  if (!mbBatched) emit propertyChanged();
}
// -------------------------------------------------------------------------------
QString CInformationElement::getIconFileName( void ) const
// -------------------------------------------------------------------------------
{
  return (!mIconFilename.isEmpty() ? mIconFilename : QString("none"));
}

// -------------------------------------------------------------------------------
InformationFormat* CInformationElement::getInformationFormat( void ) const
// -------------------------------------------------------------------------------
{
  return mpInformationFormat;
}

// -------------------------------------------------------------------------------
void CInformationElement::setInformationFormat( InformationFormat* pFormat )
// -------------------------------------------------------------------------------
{
  mpInformationFormat = pFormat;
  if (!mbBatched) emit propertyChanged();
}

// -------------------------------------------------------------------------------
QString CInformationElement::toString( void ) const
// -------------------------------------------------------------------------------
{
  QString result = mDescription+" ["+mpInformationFormat->toString()+"]\n"
                   +mInformation;
  return result;
}

// -------------------------------------------------------------------------------
QString CInformationElement::getTreeString( int tab ) const
// -------------------------------------------------------------------------------
{
  QString result = Strings::spaces(tab)+mDescription+"\n";
  tab++;

  for (CInformationElement* x : mChildObjects) {
    result += x->getTreeString(tab);
  }

  return result;
}

// -------------------------------------------------------------------------------
void CInformationElement::toXML( QDomDocument xmlDocument, QDomNode parent )
// -------------------------------------------------------------------------------
{
  QDomElement thisElement = xmlDocument.createElement("InformationElement");
  thisElement.setAttribute("informationFormat", getInformationFormat()->toString());
  thisElement.setAttribute("iconFileName", getIconFileName());

  // add description
  QDomElement description = xmlDocument.createElement("Description");
  QDomText text = xmlDocument.createTextNode(getDescription());
  description.appendChild(text);
  thisElement.appendChild(description);

  // add information
  QDomElement information = xmlDocument.createElement("Information");
  text = xmlDocument.createTextNode(getInformation());
  information.appendChild(text);
  thisElement.appendChild(information);


  // add children
  for (CInformationElement* x : mChildObjects) {
    x->toXML(xmlDocument, thisElement);
  }

  parent.appendChild(thisElement);
}


// -------------------------------------------------------------------------------
CInformationElement* CInformationElement::findChildWithDescription( QString desc )
// -------------------------------------------------------------------------------
{
  for (CInformationElement* x : mChildObjects) {
    if (x->getDescription() == desc)
      return x;
  }

  return nullptr;
}


/**
 * find the specified 'QString pattern' within the text/information of the
 * appropriate 'CInformationElement' (evtl. recursive) and append the found
 * "places" as 'SearchListItem's at the list's end.
 */
// -------------------------------------------------------------------------------
void CInformationElement::search( QString pattern, bool recursive, bool caseSensitive,
                                  bool SearchOnlyTitles, QTreeWidget& list,
                                  int& nSkippedEncrypted )
// -------------------------------------------------------------------------------
{
  // Description (title) is always plaintext on disk, so we can scan it
  // even for an entry that is still encrypted in memory.
  searchDescription(pattern, caseSensitive, list);

  if ( !SearchOnlyTitles )
  {
    if ( isCurrentlyEncrypted() )
      ++nSkippedEncrypted;        // lazy mode — body still ciphertext
    else
      searchInformation(pattern, caseSensitive, list);
  }

  // if recursive -> do so
  if (recursive)
  {
    for (CInformationElement* x : mChildObjects) {
      x->search( pattern, true, caseSensitive, SearchOnlyTitles, list,
                 nSkippedEncrypted );
    }
  }
}


// -------------------------------------------------------------------------------
void CInformationElement::searchLine( QString pattern, bool caseSensitive,
                                      QTreeWidget& list, QString line,
                                      int lineNumber, int searchLocation )
// -------------------------------------------------------------------------------
{
  Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
  int pos = line.indexOf(pattern, 0, cs);
  while (pos >= 0)
  {
    QString listtext = line;
    listtext.remove(0, pos);

    listtext.truncate(MAX_SEARCHLIST_STRLEN);
    (void) new SearchListItem( &list, new Path(this), searchLocation,
                               lineNumber, pos, pattern.length(), listtext);

    pos = line.indexOf( pattern, pos+pattern.length(), cs );
  }
}

// -------------------------------------------------------------------------------
void CInformationElement::searchDescription( QString pattern, bool caseSensitive,
                                             QTreeWidget& list)
// -------------------------------------------------------------------------------
{
  searchLine( pattern, caseSensitive, list, mDescription, -1, SearchPosition::SP_NAME );
}

// -------------------------------------------------------------------------------
void CInformationElement::searchInformation( QString pattern, bool caseSensitive,
                                             QTreeWidget& list )
// -------------------------------------------------------------------------------
{
  QString text = getInformationText() +"\n";    // add "\n" -> so, the last line is also searched
  QString oneLine;
  int line = -1;  //because first line is 0

  for (QString oneLine = Strings::removeAndReturnFirstLine(text); !oneLine.isNull();
	  oneLine = Strings::removeAndReturnFirstLine(text) )
  {
    line++;
    searchLine( pattern, caseSensitive, list, oneLine, line, SearchPosition::SP_INFORMATION);
  }

/*
  SearchRTF-Idee  (somit kann man regexp. auch in rtf-text suchen/finden)

  1) mit converter.cpp in ascii umwandeln
  2) paragraphenweise suchen, dabei gefundene Stellen als SearchListItem
    in die Liste eintragen.

*/
}


// -------------------------------------------------------------------------------
QString CInformationElement::getInformationText( void ) const
// -------------------------------------------------------------------------------
{
  QString text;

  if ( getInformationFormat() == &InformationFormat::ASCII )
  {
    text = getInformation();
  }
  else
  {
    text = Strings::removeHTMLTags(getInformation());
  }

  return text;
}


/**
 * Appends the specified text to the end of the information
 * of this element.
 */
// -------------------------------------------------------------------------------
void CInformationElement::appendInformation( QString text )
// -------------------------------------------------------------------------------
{
  if ( mpInformationFormat == &InformationFormat::RTF )
  {
    text.replace( QChar('\n'), QString("<br>\n") );
  }

  mInformation += text;
  emit informationHasChanged();
}


// -------------------------------------------------------------------------------
void CInformationElement::setInformationYPos( int iPos )
// -------------------------------------------------------------------------------
{
   miInformationYPos = iPos;
}

// -------------------------------------------------------------------------------
int CInformationElement::getInformationYPos( void ) const
// -------------------------------------------------------------------------------
{
   return miInformationYPos;
}


// -------------------------------------------------------------------------------
// Adds information so that the element has the capability to be encrypted.
// To encrypt the element call 'encrypt()'.
// -------------------------------------------------------------------------------
void CInformationElement::enableEncryption( bool bIsEncryptionEnabled,
                                            const QString& sTmpPasswd )
// -------------------------------------------------------------------------------
{
   mbIsEncryptionEnabled = bIsEncryptionEnabled;
   msTmpPasswd = mbIsEncryptionEnabled ? sTmpPasswd : QString("");
   // Toggling encryption state always invalidates whatever ciphertext we
   // had cached — the next save must re-encrypt (or stop encrypting).
   mOriginalBlob.clear();
   mbDirty = true;
}

// -------------------------------------------------------------------------------
// States whether this element has the capability to be encrypted.
// To encrypt the element call 'encrypt()'.
// -------------------------------------------------------------------------------
bool CInformationElement::isEncryptionEnabled( void ) const
// -------------------------------------------------------------------------------
{
   return mbIsEncryptionEnabled;
}

// -------------------------------------------------------------------------------
// States whether this element is currently encrypted.
// -------------------------------------------------------------------------------
bool CInformationElement::isCurrentlyEncrypted( void ) const
// -------------------------------------------------------------------------------
{
   return ( 0 != mEncryptedData.size() );
}


// -------------------------------------------------------------------------------
// This encrypts the element and makes its contents unreadable.
// -------------------------------------------------------------------------------
void CInformationElement::encrypt( void )
// -------------------------------------------------------------------------------
{
   if ( !mbIsEncryptionEnabled )
      return;

   StringCrypter::encryptString( mInformation, msTmpPasswd, mEncryptedData );
   mInformation = "";
}

// -------------------------------------------------------------------------------
// This decrypts the element and makes its contents readable.
// Returns 'true' if everything is correct and element could be decrypted.
// -------------------------------------------------------------------------------
bool CInformationElement::decrypt( const QString& sPasswd )
// -------------------------------------------------------------------------------
{
	int iError;

	// Ignore empty encrypted data fields.
	// This typically would happen only for manually updates entries or bugs in Tuxcards
	if (mEncryptedData.size() == 0)
		return true;

   iError = StringCrypter::decryptString( mEncryptedData, sPasswd, mInformation );
   if ( StringCrypter::NO_ERROR == iError )
   {
      msTmpPasswd = sPasswd;
      // Stash the on-disk ciphertext so that on save we can emit it
      // byte-for-byte if the user has not touched this element. Before
      // 3.2.0 we just dropped the blob here, which forced a full
      // re-encrypt of every element on every save.
      mOriginalBlob = mEncryptedData;
      mbDirty       = false;
      mEncryptedData.resize(0);
   }
   else if (  StringCrypter::ERROR_INVALID_FILEHEADER == iError )
   {
      // this should never happen (if the user does not touch the data)
      std::cout<<"StringCrypter::ERROR_INVALID_FILEHEADER"<<std::endl;
   }

   return (StringCrypter::NO_ERROR == iError);
}


// -------------------------------------------------------------------------------
const QByteArray& CInformationElement::getEncryptedData() const
// -------------------------------------------------------------------------------
{
   return mEncryptedData;
}

// -------------------------------------------------------------------------------
void CInformationElement::setEncryptedData( const QByteArray& data )
// -------------------------------------------------------------------------------
{
   mEncryptedData = data;
   // Newly-loaded blob from disk doubles as the "original" we'd re-emit
   // unchanged if the user never touches this element.
   mOriginalBlob = data;
   mbDirty       = false;
   mbIsEncryptionEnabled = true;
}


// By enabling encryption, will save this item and it's subtree as encrypted
// -------------------------------------------------------------------------------
void CInformationElement::enableEncryptionForElementTree(QString& password)
{
	// Enable encryption for this entry and leave it unencrypted.
	enableEncryption(true, password);

	// Go through all children and call the same function for them.
	for (CInformationElement* x : mChildObjects) {
    	x->enableEncryptionForElementTree(password);
	}
}

// Will save this item and it's subtree as unencrypted
// -------------------------------------------------------------------------------
void CInformationElement::disableEncryptionForElementTree()
{
	// Disable encryption for this entry and leave it unencrypted.
	enableEncryption(false, QString(""));

	// Go through all children and call the same function for them.
	for (CInformationElement* x : mChildObjects) {
    	x->disableEncryptionForElementTree();
	}
}

// Will return true, if any of the element in the tree is encrypted.
// -------------------------------------------------------------------------------
bool CInformationElement::checkEncryptionForElementTree()
{
	if (isEncryptionEnabled())
		return true;

	// Go through all children and call the same function for them.
	for (CInformationElement* x : mChildObjects) {
    	if (x->checkEncryptionForElementTree())
    		return true;
	}
	return false;
}

// -------------------------------------------------------------------------------
bool CInformationElement::firstEncryptedBlob(QByteArray& out) const
{
   if ( isCurrentlyEncrypted() && mEncryptedData.size() > 0 ) {
      out = mEncryptedData;
      return true;
   }
   for ( const CInformationElement* x : mChildObjects ) {
      if ( x->firstEncryptedBlob(out) )
         return true;
   }
   return false;
}

// Decrypt's this element and it's children in the tree.
// -------------------------------------------------------------------------------
bool CInformationElement::decryptTree(QString password)
{
	bool bCorrectPasswd;

	// Decrypt this entry.
	if (isCurrentlyEncrypted()) {
//        std::cout << "Decrypting: " << mDescription.toStdString() << std::endl;
		bCorrectPasswd = decrypt(password);
		if (!bCorrectPasswd)
			return bCorrectPasswd;
	}

	// Go through all children and call the same function for them.
	for (CInformationElement* x : mChildObjects) {
		if (x->isCurrentlyEncrypted()) {
	    	bCorrectPasswd = x->decryptTree(password);
			if (!bCorrectPasswd)
				return bCorrectPasswd;
		}
	}
	return true;
}

// -------------------------------------------------------------------------------
void CInformationElement::setTextColor(QColor& c)
{
	mtextColor = c;
}

// -------------------------------------------------------------------------------
QColor CInformationElement:: getTextColor() const
{
	return mtextColor;
}

// -------------------------------------------------------------------------------
void CInformationElement::setSubTreeTextColor(QColor& c)
{
	// Set text color for current entry.
	msubtreeTextColor = mtextColor = c;

	// Go through all children and call the same function for them.
	for (CInformationElement* x : mChildObjects) {
    	x->setSubTreeTextColor(c);
	}
}
