/***************************************************************************
                          CInformationCollection.h  -  description
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
#ifndef CINFORMATION_COLLECTION_H
#define CINFORMATION_COLLECTION_H

#include "../global.h"
#include "IParent.h"

#include <iostream>
#include "CInformationElement.h"
#include <QList>
#include <QObject>
#include "Path.h"
#include "IView.h"

class CInformationCollection : public QObject,
                               public IParent
{
  Q_OBJECT
public:
  CInformationCollection( CInformationElement* pRoot = nullptr );
  ~CInformationCollection( void);
  static CInformationCollection* createDefaultCollection( void );

  // ************* IParent - End ***********************************
  virtual QString  getDescription( void );
  virtual IParent* getParent( void );
  virtual void     removeChild( CInformationElement* pChild );
  virtual void     aboutToRemoveElement( CInformationElement* pIE );
  // ************* IParent - End ***********************************

  void setRootElement( CInformationElement* pRoot );
  CInformationElement* getRootElement( void );

  void setActiveElement( CInformationElement* pElement );
  void setActiveElement( Path path );
  CInformationElement* getActiveElement( void );
  CInformationElement* getInformationElement( Path path );
  bool isPathValid( Path path );

  void registerAsListenerOf( CInformationElement* pElem );

  QString toString( void );
  QString toXML( void );
  QString toXML( CInformationElement* );


  void setEncrypted(bool val);
  bool isEncrypted();
  void toggleEncryption(QString& password);
  bool checkEncryptionForElementTree();
  bool decryptTree(QString password, bool lazy = false);
  // Decrypts a single element with the cached file password if it is
  // still in ciphertext form. Used in lazy-decrypt mode when the user
  // navigates to an entry that was not eagerly decrypted on open.
  void ensureDecrypted(CInformationElement* pElem);
  bool firstEncryptedBlob(QByteArray& out) const;

  void addView( IView* pView );
  void removeView( IView* pView );

  unsigned int numElements() { return mNumElements; }

signals:
  void activeInformationElementChanged( CInformationElement* );
  void modelHasChanged();
  void numElementsChanged(int);

public slots:
  void slotChildAdded( CInformationElement* );


private:
  CInformationElement* mpRootElement;
  CInformationElement* mpActiveElement;

  QList<IView*> mViews;
  void notifyViewsToRemoveElement( CInformationElement* pIE );

  bool			mbEncrypted;
  QString		mstrFilePassword;
  int	mNumElements;
};
#endif

