/***************************************************************************
                          CPropertyDialog.cpp  -  description
                             -------------------
    begin                : Tue Mar 28 2000
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
#include "../../icons/blank.xpm"


#include "CPropertyDialog.h"
#include "../../global.h"

#include "../../information/CTreeInformationElement.h"
#include "../../CTuxCardsConfiguration.h"

#include <QMessageBox>
#include <qradiobutton.h>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPixmap>


// -------------------------------------------------------------------------------
CPropertyDialog::CPropertyDialog( QWidget* pParent,
                                  CTuxCardsConfiguration& refTuxConfiguration )
 : QDialog( pParent )
 , mBlankIcon( blank_xpm )
 , mIconSelector()
 , miMode( MODE_NONE )
 , miChoice( 0 )
 , mpEditingElement( nullptr )
 , mrefTuxConfiguration( refTuxConfiguration )
{
   setObjectName("CPropertyDialog");
   setModal(true);
   setupUi(this);

   connect( mpIconButton, SIGNAL(clicked()), this, SLOT(chooseIcon()) );
   connect( mpButtonApply, SIGNAL(clicked()), this, SLOT(changeProperties()) );
}


// -------------------------------------------------------------------------------
CPropertyDialog::~CPropertyDialog( void )
// -------------------------------------------------------------------------------
{
   mpEditingElement = nullptr;                 // do not kill this pointer
}


// -------------------------------------------------------------------------------
void CPropertyDialog::setUp( CInformationElement* pElement, int iMode )
// -------------------------------------------------------------------------------
{
   if ( nullptr == pElement )
      return;

   if ( iMode == MODE_CHANGE_PROPERTIES )
   {
      setWindowTitle(tr("Change Properties of existing Entry"));
      setAttributes( pElement->getDescription(), pElement->getIconFileName());
      mpTextFormatChoser->setEnabled( false );
   }
   else if ( iMode == MODE_CREATE_NEW_ELEMENT )
   {
      setWindowTitle( tr("Add new Entry") );
      setAttributes( "", "none" );
      mpTextFormatChoser->setEnabled( true );
   }
   else
   {
      return;
   }


   miMode = iMode;
   mpEditingElement = pElement;
   show();

   exec();
}


// -------------------------------------------------------------------------------
void CPropertyDialog::setAttributes( QString sDescription, QString sIconFilename)
// -------------------------------------------------------------------------------
{
   if ( sIconFilename == "none" )
   {
      // entry without icon
      mpNoIconRB->setChecked(true); mpUseIconRB->setChecked(false);
      mpIconButton->setIcon(QIcon( mBlankIcon ));
   }
   else
   {
      mpIconButton->setIcon(QIcon( sIconFilename ));
      mpNoIconRB->setChecked(false); mpUseIconRB->setChecked(true);
   }
   mpLocationLabel->setText( sIconFilename );

   mpNameLine->setText(sDescription);
}



/**
 * open a filedialog to let user select his icon
 */
// -------------------------------------------------------------------------------
void CPropertyDialog::chooseIcon( void )
// -------------------------------------------------------------------------------
{
   // getting iconfileName
   if( ! mIconSelector.exec() ) return;
   QString h = mIconSelector.getIconFileName();

   if( h == "" ) return;
   // if 'h' is a valid fileName, test whether it is a valid Pixmap
   QPixmap pix(h);
   if ( ! pix.isNull() )
   {
      mpIconButton->setIcon(QIcon(h));
      mpLocationLabel->setText(h);
   }
}


// -------------------------------------------------------------------------------
QString CPropertyDialog::getName( void )
// -------------------------------------------------------------------------------
{ return mpNameLine->text(); }


// -------------------------------------------------------------------------------
QString CPropertyDialog::getIconFileName( void )
// -------------------------------------------------------------------------------
{
   if ( mpUseIconRB->isChecked() )
      return mpLocationLabel->text();
   else
      return "none";
}


/**
 *  This slot is called when the apply-Button is pressed.
 *  The set attributes are applied to the currently edited informationElement.
 */
// -------------------------------------------------------------------------------
void CPropertyDialog::changeProperties( void )
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpEditingElement )
      return;

//   if ( (nullptr == mpPasswdLineOne) || (nullptr == mpPasswdLineTwo) )
//      return;

   if ( getName().trimmed().isEmpty() )
   {
      int iAnswer = QMessageBox::warning( this, tr("TuxCards"), tr("The name of your note is empty.\n"
                                          "Do you want to change this?"),
                                          QMessageBox::Yes, QMessageBox::No );
      if ( QMessageBox::Yes == iAnswer )
      {
         show();
         return;
      }
   }



   if ( miMode == MODE_CHANGE_PROPERTIES )
   {
      // change properties: name & icon
      mpEditingElement->setBatched( true );
      mpEditingElement->setDescription( getName() );
      mpEditingElement->setIconFileName( getIconFileName() );
      mpEditingElement->setBatched( false );
   }
   else if ( miMode == MODE_CREATE_NEW_ELEMENT )
   {
      InformationFormat* pInfoFormat
             = InformationFormat::getByString( mpTextFormatChoser->currentText() );

      CTreeInformationElement* pNewElement = new CTreeInformationElement( mpEditingElement,
                                                             getName(),
                                                             "",
                                                             pInfoFormat,
                                                             getIconFileName());

      mpEditingElement->addChild( pNewElement );
      ((CTreeInformationElement*)mpEditingElement)->setOpen(true);
   }

   close();
}
