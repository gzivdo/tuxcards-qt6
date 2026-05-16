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
#include <QScreen>
#include <QGuiApplication>

#include <iostream>

// -------------------------------------------------------------------------------
CFileEncryptionPasswordDialog::CFileEncryptionPasswordDialog( QWidget* pParent )
 : QDialog( pParent )
 , msPasswd( "" )
{
	setObjectName("CFileEncryptionPasswordDialog");
	setModal(true);
	setupUi(this);
	// buttonOk → accept() is already wired by setupUi() via the .ui's
	// <connections> block; adding a second SIGNAL/SLOT connect here would
	// fire accept() twice on each click.
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

	// Center the dialog on the screen. We deliberately avoid using
	// parentWidget()->geometry() — at app startup MainWindow may not be
	// shown yet and its geometry would be bogus (often (0,0)+default
	// size), which threw the dialog into the top-left corner.
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
void CFileEncryptionPasswordDialog::accept()
{
   if ( nullptr == leFilePassword)
      return;

   if ( leFilePassword->text().trimmed().isEmpty() )
   {
      (void) QMessageBox::warning( this, tr("TuxCards"),
                                   tr("Password field is empty. Please specify a valid password"));
      show();
      return;
   }

   msPasswd = leFilePassword->text().trimmed();
   QDialog::accept();
}


// -------------------------------------------------------------------------------
QString CFileEncryptionPasswordDialog::getPasswd( void )
{
	return msPasswd;
}
