/***************************************************************************
                          optionsdialog.cpp  -  description
                             -------------------
    begin                : Thu Mar 30 2000
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

#include "optionsdialog.h"
#include "../colorbar/CColorBar.h"
#include <QRadioButton>
#include <QFontDialog>
#include <QFileDialog>
#include <QButtonGroup>
#include <QGroupBox>
#include <QTabWidget>
#include <iostream>

OptionsDialog::OptionsDialog( QWidget* parent, CTuxCardsConfiguration& config )
 : QDialog( parent )
 , mrefConfig( config )
{
   setObjectName("OptionsDialog");
   setModal(true);
   setupUi(this);
   connect( mpOkButton, SIGNAL(clicked()), this, SLOT(changeProperties()) );
   connect( treeFontButton, SIGNAL(clicked()), this, SLOT(changeTreeFont()) );
   connect( editorFontButton, SIGNAL(clicked()), this, SLOT(changeEditFont()) );

   buildSidebarTab();
}


void OptionsDialog::setButtonSwatch(QPushButton* b, const QColor& c)
{
   QPixmap pix(60, 16);
   pix.fill(c);
   b->setIcon(QIcon(pix));
}


void OptionsDialog::buildSidebarTab()
{
   QWidget* tab = new QWidget(TabWidget2);
   QHBoxLayout* outer = new QHBoxLayout(tab);

   // preview bar on the left
   mpSidebarPreview = new CColorBar( tab, QColor(0,0,0), QColor(33,72,170),
                                     "Tux", "Cards", QColor(Qt::white) );
   mpSidebarPreview->setMinimumWidth(40);
   outer->addWidget(mpSidebarPreview);

   // form on the right
   QVBoxLayout* form = new QVBoxLayout();
   outer->addLayout(form, 1);

   // colors
   QGroupBox* colorBox = new QGroupBox("Choose Colors", tab);
   QGridLayout* colorGrid = new QGridLayout(colorBox);
   colorGrid->addWidget(new QLabel("Top Color"), 0, 0);
   mpTopColorBtn = new QPushButton("Top Color");
   colorGrid->addWidget(mpTopColorBtn, 0, 1);
   colorGrid->addWidget(new QLabel("Bottom Color"), 1, 0);
   mpBottomColorBtn = new QPushButton("Bottom Color");
   colorGrid->addWidget(mpBottomColorBtn, 1, 1);
   colorGrid->addWidget(new QLabel("Text Color"), 2, 0);
   mpTextColorBtn = new QPushButton("Text Color");
   colorGrid->addWidget(mpTextColorBtn, 2, 1);
   form->addWidget(colorBox);
   connect(mpTopColorBtn,    SIGNAL(clicked()), this, SLOT(chooseTopColor()));
   connect(mpBottomColorBtn, SIGNAL(clicked()), this, SLOT(chooseBottomColor()));
   connect(mpTextColorBtn,   SIGNAL(clicked()), this, SLOT(chooseTextColor()));

   // horizontal text
   mpShowHText = new QCheckBox("Show Horizontal Text", tab);
   form->addWidget(mpShowHText);
   QGroupBox* hBox = new QGroupBox(tab);
   QGridLayout* hGrid = new QGridLayout(hBox);
   hGrid->addWidget(new QLabel("First Text Line"), 0, 0);
   mpTextOne = new QLineEdit;
   hGrid->addWidget(mpTextOne, 0, 1);
   hGrid->addWidget(new QLabel("Second Text Line"), 1, 0);
   mpTextTwo = new QLineEdit;
   hGrid->addWidget(mpTextTwo, 1, 1);
   form->addWidget(hBox);

   // vertical text
   mpShowVText = new QCheckBox("Show Vertical Text", tab);
   form->addWidget(mpShowVText);
   QGroupBox* vBox = new QGroupBox(tab);
   QVBoxLayout* vBoxLay = new QVBoxLayout(vBox);
   mpVText = new QLineEdit;
   vBoxLay->addWidget(mpVText);
   QHBoxLayout* posLay = new QHBoxLayout();
   posLay->addWidget(new QLabel("Text Position"));
   mpVTextTop    = new QRadioButton("Top");
   mpVTextBottom = new QRadioButton("Bottom");
   QButtonGroup* g = new QButtonGroup(vBox);
   g->addButton(mpVTextTop, 0);
   g->addButton(mpVTextBottom, 1);
   posLay->addWidget(mpVTextTop);
   posLay->addWidget(mpVTextBottom);
   vBoxLay->addLayout(posLay);
   form->addWidget(vBox);

   form->addStretch(1);

   TabWidget2->addTab(tab, "SideBar");

   // live preview when fields change
   connect(mpShowHText, SIGNAL(toggled(bool)), this, SLOT(refreshPreviewSlot()));
   connect(mpShowVText, SIGNAL(toggled(bool)), this, SLOT(refreshPreviewSlot()));
   connect(mpTextOne,   SIGNAL(textChanged(QString)), this, SLOT(refreshPreviewSlot()));
   connect(mpTextTwo,   SIGNAL(textChanged(QString)), this, SLOT(refreshPreviewSlot()));
   connect(mpVText,     SIGNAL(textChanged(QString)), this, SLOT(refreshPreviewSlot()));
   connect(mpVTextTop,  SIGNAL(toggled(bool)), this, SLOT(refreshPreviewSlot()));
}


void OptionsDialog::refreshPreview()
{
   if (!mpSidebarPreview) return;
   QString t1 = mpShowHText->isChecked() ? mpTextOne->text() : "";
   QString t2 = mpShowHText->isChecked() ? mpTextTwo->text() : "";
   mpSidebarPreview->change( mTopColor, mBottomColor, t1, t2, mTextColor );
   QString vt = mpShowVText->isChecked() ? mpVText->text() : "";
   mpSidebarPreview->setVerticalText( vt, mpVTextBottom->isChecked() );
   mpSidebarPreview->update();
}


void OptionsDialog::loadSidebarFromConfig()
{
   mTopColor    = mrefConfig.getTopColor();
   mBottomColor = mrefConfig.getBottomColor();
   mTextColor   = mrefConfig.getFontColor();
   setButtonSwatch(mpTopColorBtn,    mTopColor);
   setButtonSwatch(mpBottomColorBtn, mBottomColor);
   setButtonSwatch(mpTextColorBtn,   mTextColor);

   mpShowHText->setChecked( mrefConfig.getBoolValue( CTuxCardsConfiguration::B_IS_HTEXT_ENABLED ) );
   mpTextOne->setText( mrefConfig.getStringValue( CTuxCardsConfiguration::S_TEXT_ONE ) );
   mpTextTwo->setText( mrefConfig.getStringValue( CTuxCardsConfiguration::S_TEXT_TWO ) );

   mpShowVText->setChecked( mrefConfig.getBoolValue( CTuxCardsConfiguration::B_IS_VTEXT_ENABLED ) );
   mpVText->setText( mrefConfig.getStringValue( CTuxCardsConfiguration::S_VERTICAL_TEXT ) );
   if ( mrefConfig.getBoolValue( CTuxCardsConfiguration::B_ALIGN_VTEXT ) )
      mpVTextBottom->setChecked(true);
   else
      mpVTextTop->setChecked(true);

   refreshPreview();
}


void OptionsDialog::saveSidebarToConfig()
{
   mrefConfig.setTopColor(mTopColor);
   mrefConfig.setBottomColor(mBottomColor);
   mrefConfig.setFontColor(mTextColor);
   mrefConfig.setBoolValue(CTuxCardsConfiguration::B_IS_HTEXT_ENABLED, mpShowHText->isChecked());
   mrefConfig.setStringValue(CTuxCardsConfiguration::S_TEXT_ONE, mpTextOne->text());
   mrefConfig.setStringValue(CTuxCardsConfiguration::S_TEXT_TWO, mpTextTwo->text());
   mrefConfig.setBoolValue(CTuxCardsConfiguration::B_IS_VTEXT_ENABLED, mpShowVText->isChecked());
   mrefConfig.setStringValue(CTuxCardsConfiguration::S_VERTICAL_TEXT, mpVText->text());
   mrefConfig.setBoolValue(CTuxCardsConfiguration::B_ALIGN_VTEXT, mpVTextBottom->isChecked());
}


void OptionsDialog::chooseTopColor()
{
   QColor c = QColorDialog::getColor(mTopColor, this, "Top Color");
   if (!c.isValid()) return;
   mTopColor = c;
   setButtonSwatch(mpTopColorBtn, c);
   refreshPreview();
}


void OptionsDialog::chooseBottomColor()
{
   QColor c = QColorDialog::getColor(mBottomColor, this, "Bottom Color");
   if (!c.isValid()) return;
   mBottomColor = c;
   setButtonSwatch(mpBottomColorBtn, c);
   refreshPreview();
}


void OptionsDialog::chooseTextColor()
{
   QColor c = QColorDialog::getColor(mTextColor, this, "Text Color");
   if (!c.isValid()) return;
   mTextColor = c;
   setButtonSwatch(mpTextColorBtn, c);
   refreshPreview();
}


int OptionsDialog::setUp( void )
{
   autosave       ->setChecked(mrefConfig.getBoolValue( CTuxCardsConfiguration::B_AUTOSAVE ));
   saveMinutes    ->setText(QString::number(mrefConfig.getIntValue( CTuxCardsConfiguration::I_SAVE_ALL_MINUTES )));
   saveWhenLeaving->setChecked(mrefConfig.getBoolValue( CTuxCardsConfiguration::B_SAVE_WHEN_LEAVING ));
   createBackup   ->setChecked(mrefConfig.getBoolValue( CTuxCardsConfiguration::B_CREATE_BACKUP_FILE ));
   pCommandLine   ->setText( mrefConfig.getStringValue( CTuxCardsConfiguration::S_EXECUTE_STATEMENT ));
   mpIconDirecory ->setText( mrefConfig.getStringValue( CTuxCardsConfiguration::S_ICON_DIR ));

   treeFontText  ->setFont(mrefConfig.getTreeFont().toFont());
   treeFontText  ->setText(mrefConfig.getTreeFont().toFont().family());
   editorFontText->setFont(mrefConfig.getASCIIEditorFont().toFont());
   editorFontText->setText(mrefConfig.getASCIIEditorFont().toFont().family());

   tabSize->setText(QString::number(mrefConfig.getIntValue( CTuxCardsConfiguration::I_TAB_SIZE )));

   // 'wordwrap' is a non-negative number; 0 means noWrap, 1 means widgetWrap,
   //   'wordwrap' >1 mean wrap at column 'wordwrap'
   int iWordWrap = mrefConfig.getIntValue( CTuxCardsConfiguration::I_WORD_WRAP );
   switch ( iWordWrap )
   {
   case 0:
      noWrap->setChecked( true );
      break;

   case 1:
      widgetWrap->setChecked( true );
      break;

   default:
      columnWrap->setChecked( true );
      wrapColumn->setText( QString::number(iWordWrap) );
      break;
   };

   loadSidebarFromConfig();

   show();
   return exec();
}

/**
 * This method is called whenever the ok-button is pressed.
 * Then all changes are set with the configuratio-object. The mainwindow
 * is also informed about the clicking of the ok-button and adjusts
 * own things.
 */
void OptionsDialog::changeProperties(){
 	mrefConfig.setBoolValue(   CTuxCardsConfiguration::B_AUTOSAVE,          getAutosave() );
 	mrefConfig.setIntValue(    CTuxCardsConfiguration::I_SAVE_ALL_MINUTES,  getMinutes());
 	mrefConfig.setBoolValue(   CTuxCardsConfiguration::B_SAVE_WHEN_LEAVING, getSaveWhenLeaving() );
 	mrefConfig.setBoolValue(   CTuxCardsConfiguration::B_CREATE_BACKUP_FILE,getCreateBackup() );
   mrefConfig.setStringValue( CTuxCardsConfiguration::S_EXECUTE_STATEMENT, pCommandLine->text() );
   mrefConfig.setStringValue( CTuxCardsConfiguration::S_ICON_DIR,          mpIconDirecory->text() );

   // editor-font
   QFont f                 = getEditorFont();
   QString FONT_FAMILY     =f.family();
   int     FONT_SIZE       =f.pointSize();
   bool    FONT_BOLD	      =f.bold();
   bool    FONT_ITALIC     =f.italic();
   bool    FONT_UNDERLINE  =f.underline();
   bool    FONT_STRIKEOUT  =f.strikeOut();
   FontSettings fontSettingsEditor( FONT_FAMILY, FONT_SIZE, FONT_BOLD, FONT_ITALIC, FONT_UNDERLINE, FONT_STRIKEOUT );
   mrefConfig.setASCIIEditorFont( fontSettingsEditor );

   // tree-font
   f              = getTreeFont();
   FONT_FAMILY    =f.family();
   FONT_SIZE      =f.pointSize();
   FONT_BOLD	   =f.bold();
   FONT_ITALIC    =f.italic();
   FONT_UNDERLINE =f.underline();
   FONT_STRIKEOUT =f.strikeOut();
   FontSettings fontSettingsTree( FONT_FAMILY, FONT_SIZE, FONT_BOLD, FONT_ITALIC, FONT_UNDERLINE, FONT_STRIKEOUT );
   mrefConfig.setTreeFont( fontSettingsTree );

   mrefConfig.setIntValue(  CTuxCardsConfiguration::I_TAB_SIZE,  getTabSize() );
   mrefConfig.setIntValue(  CTuxCardsConfiguration::I_WORD_WRAP, getWordWrap() );

   saveSidebarToConfig();

   // done
   mrefConfig.saveToFile();
   emit configurationChanged();
}


bool OptionsDialog::getAutosave(){      return autosave->isChecked(); }
int  OptionsDialog::getMinutes()
{
   int minutes = saveMinutes->text().toInt();
   if ( 0 > minutes )
      return 0;

   return minutes;
}
bool OptionsDialog::getSaveWhenLeaving(){ return saveWhenLeaving->isChecked(); }
bool OptionsDialog::getCreateBackup(){    return createBackup->isChecked(); }

QFont OptionsDialog::getTreeFont(){   return treeFontText->font();   }
QFont OptionsDialog::getEditorFont(){ return editorFontText->font(); }

int OptionsDialog::getTabSize(){ return tabSize->text().toInt(); }
int OptionsDialog::getWordWrap(){
	if(noWrap->isChecked())
		return 0;
	else if(widgetWrap->isChecked())
		return 1;
	else{
		return wrapColumn->text().toInt();
	}
}

//void OptionsDialog::autosave(){
//	if (autosave->isChecked()){
//		saveMinutes->setEnabled(true);  label->setEnabled(true);
//	}else{
//		saveMinutes->setEnabled(false);  label->setEnabled(false);
//	}
//}


/**
 * calls the fontdialog and changes the option for the font of the
 * tuxcards-tree if necessary
 */
void OptionsDialog::changeTreeFont(){
 	bool ok;
	QFont f=QFontDialog::getFont(&ok, treeFontText->font(), this);

	// a valid font was selected
	if(ok){
		treeFontText->setFont(f);
		treeFontText->setText(f.family());
	}
}

/**
 * calls the fontdialog and changes the option for the font of the
 * tuxcards-editor if necessary
 */

void OptionsDialog::changeEditFont(){
 	bool ok;
	QFont f=QFontDialog::getFont(&ok, editorFontText->font(), this);

	// a valid font was selected
	if(ok){
		editorFontText->setFont(f);
		editorFontText->setText(f.family());
	}
}



