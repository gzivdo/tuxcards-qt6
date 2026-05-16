/***************************************************************************
                          optionsdialog.h  -  description
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

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include "ui_IOptionsDialog.h"
#include <qtabbar.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>

#include <QLabel>
#include <QLineEdit>
#include <qvalidator.h>

#include <QPushButton>
#include <QColorDialog>
#include <QString>
#include <QFont>

#include "../../CTuxCardsConfiguration.h"

#include <iostream>
class CColorBar;
class QRadioButton;

class OptionsDialog : public QDialog, public Ui_IOptionsDialog {
   Q_OBJECT
public:
   OptionsDialog(QWidget* parent, CTuxCardsConfiguration& config);
   int setUp( void );

   bool getAutosave();
   int  getMinutes();
   bool getSaveWhenLeaving();
   bool getCreateBackup();

   QFont   getTreeFont();
   QFont   getEditorFont();
   int     getTabSize();
   int     getWordWrap();

protected slots:
   virtual void changeTreeFont();
   virtual void changeEditFont();

   virtual void changeProperties();

   void chooseTopColor();
   void chooseBottomColor();
   void chooseTextColor();
   void refreshPreviewSlot() { refreshPreview(); }

signals:
  void configurationChanged();

private:
  CTuxCardsConfiguration& mrefConfig;

  // SideBar tab widgets
  CColorBar*    mpSidebarPreview;
  QPushButton*  mpTopColorBtn;
  QPushButton*  mpBottomColorBtn;
  QPushButton*  mpTextColorBtn;
  QColor        mTopColor;
  QColor        mBottomColor;
  QColor        mTextColor;
  QCheckBox*    mpShowHText;
  QLineEdit*    mpTextOne;
  QLineEdit*    mpTextTwo;
  QCheckBox*    mpShowVText;
  QLineEdit*    mpVText;
  QRadioButton* mpVTextTop;
  QRadioButton* mpVTextBottom;

  void          buildSidebarTab();
  void          loadSidebarFromConfig();
  void          saveSidebarToConfig();
  void          refreshPreview();
  static void   setButtonSwatch(QPushButton* b, const QColor& c);
};

#endif

