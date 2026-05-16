/***************************************************************************
                          CInformationElement.h  -  description
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
#ifndef CINFORMATIONELEMENT_H
#define CINFORMATIONELEMENT_H

#include "IParent.h"
#include <iostream>
#include <QString>
#include <QPixmap>
#include <QList>
#include "informationformat.h"
#include <QObject>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTextEdit>

#include <qdatetime.h>

#include <QDomDocument>
#include <QDomElement>


class CInformationElement : public QObject,
                            public IParent
{
  Q_OBJECT
public:
  CInformationElement( IParent* pParent,
                     QString sDescription="", QString sInformation="",
                     InformationFormat* pFormat=&InformationFormat::NONE,
                     QString sIconFileName = "none");
  ~CInformationElement( void );

  // ************* IParent *****************************************
  virtual QString  getDescription( void );
  virtual IParent* getParent( void );
  virtual void     removeChild( CInformationElement* pChild );
  virtual void     aboutToRemoveElement( CInformationElement* pIE );
  // ************* IParent - End ***********************************

  void setBatched( bool );
  bool isBatched( void ) const;

  virtual void addChild( CInformationElement* pElement );
  QList<CInformationElement*>* getChildren( void );
  int childCount( void ) const;
  void setParent( IParent* pParent = 0 );

  void deleteSelf( void );

  void    setDescription( QString );
  QString getInformation( void ) const;
  void    setInformation( const QString& );
  QString getInformationText( void ) const;
  void    appendInformation( QString text );

  bool    hasIcon( void ) const;
  void    setIcon(QPixmap);
  void    setIconFileName( const QString& );
  QString getIconFileName( void ) const;

  InformationFormat* getInformationFormat( void ) const;
  void    setInformationFormat( InformationFormat* );

  QString toString( void ) const;
  QString getTreeString(int tab=0) const;
  virtual void toXML( QDomDocument xmlDocument, QDomNode parent );

  // search() walks the (sub)tree and appends matches to `list`. In
  // lazy-decrypt mode, encrypted entries that have not yet been viewed
  // are still ciphertext — we skip them and bump `nSkippedEncrypted`
  // so the caller can show "N entries not scanned" to the user.
  void search( QString pattern, bool recursive, bool caseSensitive, bool SearchOnlyTitles,
  						QTreeWidget& list, int& nSkippedEncrypted );

  CInformationElement* findChildWithDescription( QString desc );


  void  setInformationYPos( int iPos );
  int   getInformationYPos( void ) const;

  void  enableEncryption( bool bIsEncryptionEnabled, const QString& sTmpPasswd  );
  bool  isEncryptionEnabled( void ) const;
  bool  isCurrentlyEncrypted( void ) const;
  void  encrypt( void );
  bool  decrypt( const QString& sPasswd );
  const QByteArray& getEncryptedData() const;
  void  setEncryptedData( const QByteArray& data );

  // Dirty flag — true when the user has edited this element since it
  // was loaded or last saved. The XML serializer uses this to decide
  // whether to re-run the cipher or emit mOriginalBlob unchanged.
  bool  isDirty() const   { return mbDirty; }
  void  markDirty()       { mbDirty = true; }
  void  markClean()       { mbDirty = false; }
  const QByteArray& getOriginalBlob() const { return mOriginalBlob; }
  void  setOriginalBlob( const QByteArray& blob ) { mOriginalBlob = blob; }

  // Save-time hints set by MainWindow around XML serialization. They
  // tell toXML() whether it can reuse an element's mOriginalBlob or
  // must re-encrypt with the current configured backend.
  //   sReencryptOnFormatChange == true  → re-encrypt every encrypted
  //                                       element so the file becomes
  //                                       homogeneously in the new
  //                                       format.
  //   false (default)                    → only re-encrypt dirty
  //                                       elements; others keep their
  //                                       on-disk format. File may
  //                                       contain mixed AES-GCM /
  //                                       XChaCha blobs.
  static bool sReencryptOnFormatChange;

	void enableEncryptionForElementTree(QString& password);
	void disableEncryptionForElementTree();
	bool checkEncryptionForElementTree();
	bool decryptTree(QString password);

	// Walk the subtree and return the first non-empty encrypted blob
	// found. Used to identify the format (AES-GCM / XChaCha / BF10)
	// before prompting for a password — so we can refuse to ask if the
	// backend that would decrypt it isn't linked in this build.
	bool firstEncryptedBlob(QByteArray& out) const;

	void setTextColor(QColor& c);
	QColor getTextColor() const;
	void setSubTreeTextColor(QColor& c);

signals:
  void propertyChanged( void );
  void childAdded( CInformationElement* );
  void childMoved( int oldPos, int newPos );
  void informationHasChanged( void );


protected:
   // Returns a pointer to the parent element. If this informationElement
   // is the root element, then the mpParent ptr contains a pointer to the
   // collection.
  IParent*            mpParent;

  // this should reduce update-behavior;
  // if batched==true -> the signal 'propertyChanged()' is not emitted
  bool                mbBatched;

  QString             mDescription;
  InformationFormat*  mpInformationFormat;
  QString             mInformation;
  QPixmap             mIcon;
  QString             mIconFilename;

  QList<CInformationElement*>* mpChildObjects;

  int                 miInformationYPos;


  bool                mbIsEncryptionEnabled;
  QString             msTmpPasswd;
  QByteArray          mEncryptedData;

  // Last-known on-disk ciphertext for this element. Set when the element
  // is loaded from disk OR right after encrypt(). Lets toXML emit the
  // existing blob byte-for-byte instead of running the cipher again when
  // the user hasn't touched this element since load (mbDirty == false)
  // — this is what makes the "only re-encrypt modified" Options
  // checkbox work.
  QByteArray          mOriginalBlob;
  bool                mbDirty;

  QColor 			  mtextColor;
  QColor 			  msubtreeTextColor;

private:
  void searchLine( QString pattern, bool caseSensitive, QTreeWidget& list, QString oneLine,
                  int lineNumber, int searchLocation );
  void searchDescription( QString pattern, bool caseSensitive, QTreeWidget& list );
  void searchInformation( QString pattern, bool caseSensitive, QTreeWidget& list );
};
#endif

