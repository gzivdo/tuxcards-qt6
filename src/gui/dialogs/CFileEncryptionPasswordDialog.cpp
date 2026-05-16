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

#include "CFileEncryptionPasswordDialog.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>

#include <iostream>

// -------------------------------------------------------------------------------
CFileEncryptionPasswordDialog::CFileEncryptionPasswordDialog( QWidget* pParent )
 : QDialog( pParent )
 , msPasswd( "" )
{
	setObjectName("CFileEncryptionPasswordDialog");
	setModal(true);
	setupUi(this);
	connect( buttonOk, SIGNAL(clicked()), this, SLOT(accept()) );
}

// -------------------------------------------------------------------------------
void CFileEncryptionPasswordDialog::setUp(QString strFileName)
{
	if (nullptr == leFilePassword)
    	return;

	tlFileName->setText(strFileName);
	leFilePassword->setText("");
	msPasswd = "";

	leFilePassword->setFocus();
	show();
	exec();
}



// -------------------------------------------------------------------------------
void CFileEncryptionPasswordDialog::accept()
{
   if ( nullptr == leFilePassword)
      return;

   if ( leFilePassword->text().trimmed().isEmpty() )
   {
      (void) QMessageBox::warning( this, "TuxCards", "Password field is empty. Please specify a valid password");
      show();
      return;
   }

   msPasswd = leFilePassword->text().trimmed();
   close();
}


// -------------------------------------------------------------------------------
QString CFileEncryptionPasswordDialog::getPasswd( void )
{
	return msPasswd;
}
