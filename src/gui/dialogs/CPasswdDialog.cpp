/***************************************************************************
                          CPasswdDialog.cpp  -  description
                             -------------------
    begin                : Wed Jan 14 2004
    copyright            : (C) 2004 by Alexander Theel
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

#include "CPasswdDialog.h"

#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>

#include <iostream>

// -------------------------------------------------------------------------------
CPasswdDialog::CPasswdDialog( QWidget* pParent )
 : QDialog( pParent )
 , msPasswd( "" )
{
   setObjectName("CPasswdDialog");
   setModal(true);
   setupUi(this);
   connect( mpOkButton, SIGNAL(clicked()), this, SLOT(verifyAndAccept()) );
}

// -------------------------------------------------------------------------------
void CPasswdDialog::setUp( const QString& sIEDescription )
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpIEDescription) || (nullptr == mpPasswdLineOne)
        || (nullptr == mpPasswdLineTwo) )
      return;

   mpIEDescription->setText("'"+sIEDescription+"'");
   mpPasswdLineOne->setText("");
   mpPasswdLineTwo->setText("");
   msPasswd = "";

   mpPasswdLineOne->setFocus();

   // Center the dialog on the screen (not on the parent geometry —
   // see CFileEncryptionPasswordDialog::setUp for the rationale).
   adjustSize();
   QScreen* screen = nullptr;
   if ( QWidget* p = parentWidget() )
      screen = p->screen();
   if ( !screen )
      screen = QGuiApplication::primaryScreen();
   if ( screen )
      move( screen->availableGeometry().center() - rect().center() );

   show();
   exec();
}



// -------------------------------------------------------------------------------
void CPasswdDialog::verifyAndAccept()
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpPasswdLineOne) || (nullptr == mpPasswdLineTwo) )
      return;

   if ( mpPasswdLineOne->text().trimmed().isEmpty() )
   {
      (void) QMessageBox::warning( this, tr("TuxCards"),
                                   tr("Password field is empty. Please specify a valid password") );
      return;
   }

   if ( 0 != mpPasswdLineOne->text().trimmed().compare(
                        mpPasswdLineTwo->text().trimmed()) )
   {
      (void) QMessageBox::warning( this, tr("TuxCards"),
                                   tr("Passwords did not match. Please try again") );
      mpPasswdLineOne->setText("");
      mpPasswdLineTwo->setText("");
      mpPasswdLineOne->setFocus();
      return;
   }
   msPasswd = mpPasswdLineOne->text().trimmed();
   accept();
}



// -------------------------------------------------------------------------------
QString CPasswdDialog::getPasswd( void )
// -------------------------------------------------------------------------------
{ return msPasswd; }
