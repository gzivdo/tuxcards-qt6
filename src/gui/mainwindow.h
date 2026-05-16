/***************************************************************************
                          mainwindow.h  -  description
                             -------------------
    begin                : Sun Mar 26 2000
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
//Added by qt3to4:
#include <QCloseEvent>
#include <QTimerEvent>
#include <QList>
#include <QLabel>
#include <QKeyEvent>
#include "../information/IHistoryListener.h"
#include "../information/CInformationElementHistory.h"

#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

#include "./dialogs/optionsdialog.h"

#include <QToolBar>
#include <QToolButton>
#include <QPixmap>
#include <QMimeData>
#include <QWhatsThis>
#include <QComboBox>

#include <QHBoxLayout>
//#include "qwidgetstack.h"
#include <QSplitter>
#include "CSingleEntryView.h"
#include "editor.h"
#include "CTree.h"

#include <QObject>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QString>

#include "../information/CInformationCollection.h"
#include "../CTuxCardsConfiguration.h"
#include "./RecentFileList.h"

#include "./dialogs/CPasswdDialog.h"

#include <iostream>

class MainWindow : public QMainWindow,
                   public IHistoryListener
{
  Q_OBJECT
private:
  CInformationCollection* mpCollection;
  QMenuBar*               mpMenu;
  OptionsDialog*          mpOptionsDialog;
  CTuxCardsConfiguration& mConfiguration;

  QSplitter*              mpSplit;
  class CColorBar*        mpColorBar;
  QList<int>* lst;
  CTree*                  mpTree;
  CSingleEntryView*       mpSingleEntryView;
  Editor*                 mpEditor;
  QToolBar*               mpQuickLoader;
  RecentFileList*         mpRecentFiles;

  QStatusBar*             mpStatusBar;
  QLabel*     statusBar_ChangeLabel;
  QLabel*     mstatusBar_EncryptedLabel;
  QLabel*     mstatusBar_NumElements;
  QLabel*     mstatusBar_SaveIndicator;   // "Saved Xs ago" / "Unsaved"
  class QTimer* mpSaveIndicatorTimer;
  qint64      mLastSaveEpochMs;           // 0 means never saved this session
  QDialog*    showDialog;       // a dialog
  QLabel*     showLabel;        // the label of the 'show'-dialog, to change it
                                //     whenever we want to

  bool    CHANGES;              // states whether or not -> changes are done (for saving)
  bool    mbStartupDone;        // true after applyConfiguration() — guards init-time writes to disk

  int     TIMER_ID;

  void lowMemoryExit( void );
  void checkPointer( void* pPointer );

  void settingUpActions( void );
  void settingUpMenu( void );
  void settingUpToolBar( void );
  void settingUpStatusBar( void );
  void settingUpEditor( QWidget* pParent );
  void settingUpTree( QWidget* pParent );
  void setWindowGeometry( int,int, int,int );

  void settingUpQuickLoader( void );

  QComboBox* pComboListStyle;
  QComboBox* pComboFont;
  QComboBox* pComboSize;

  QAction* textFormatTool;
  QAction* textBoldTool;
  QAction* textItalicTool;
  QAction* textUnderTool;
  QAction* textColorTool;

  QAction* textLeftTool;
  QAction* textCenterTool;
  QAction* textRightTool;
  QAction* textBlockTool;

  QAction* mpLeftButton;
  QAction* mpRightButton;

  // actions
  QAction* mfileEncryptFile;
  QAction* editUndoAction;
  QAction* editRedoAction;
  QAction* editCopyAction;
  QAction* editSetEntryColor;
  QAction* editSetEntrySubTreeColor;

	int PromptBeforeNewFile();
  int askForSaving(QString question);
  bool initializingCollection(QString dataFileName);
  void deleteCollection(CInformationCollection* collection);

  void callingExecutionStatement();

  CInformationElementHistory mHistory;
  CPasswdDialog              mPasswdDialog;

  QToolBar*  mpMainTools;
  QToolBar*  mpEntryTools;
  QToolBar*  mpEditorTools;
  QAction*   miMainToolBarID;
  QAction*   miEntryToolBarID;
  QAction*   miEditorToolBarID;


  CInformationElement* getActiveIE();
  void applyConfiguration();
  void startAutosaveTimer();

private slots:
  void textListStyle(int);
  void textFontFamily( const QString &f );
  void textFontSize( const QString &p );
  void textBold();
  void textItalic();
  void textUnder();
  void textColor();
  void textLeft();
  void textHCenter();
  void textRight();
  void textBlock();

  void changeInformationFormat();
  void textFontChanged(const QFont &f);
  void textColorChanged(const QColor &c);
  void textAlignmentChanged(int);
  void showRecognizedFormat(InformationFormat format);

  void editConfiguration();
  void applyConfigurationMain();

  void wordCount();
  void insertCurrentDate();
  void insertCurrentTime();

  void activatePreviousHistoryElement( void );
  void activateNextHistoryElement( void );

  // debug slots
  void debugShowRTFTextSource();
  void debugShowXMLCode();

//  void saveActiveEntry();
  void quicklyLoad(Path*);

  void search();
  void print();

  void moveElementUp();
  void moveElementDown();

  void addElementToBookmarksEvent();

  void slotSaveAndLoadNewFile(QString newFile);

  void activeInformationElementChanged(CInformationElement*);


  void setMainToolbarVisible( bool bVisible );
  void setEntryToolbarVisible( bool bVisible );
  void setEditorToolbarVisible( bool bVisible );

public:
  MainWindow(QString arg);
  ~MainWindow();

  // ************* IHistoryListener ******************
  virtual void historyStatusChanged( bool bHasPreviousElement, bool bHasNextElement );

public slots:
  void showMessage(QString, int seconds);

  // menucalls
  void newFile();
  void open();
  bool open(QString fileName);
  void save();
  void save(QString);
  void saveAs();
  void toggleFileEncryption();
  void exportHTML();
  void exportEntryMarkdown();
  void importEntryMarkdown();
  void exit();

  void showKBShortcuts();
  void showAbout();

  void selectLastActiveElement( void );
  void makeVisible( SearchPosition* pPosition );

  virtual void keyPressEvent( QKeyEvent* );

  void updateStatusbarElements(int);
protected:
  void clearAll();
  int  getDataFileFormat( QString fileName );
  bool openOldDataFile( QString fileName );
  bool openXMLDataFile( QString fileName );

  virtual void closeEvent( QCloseEvent *e );
  void timerEvent( QTimerEvent* );
  void checkFirstTime( void );

protected slots:
  void recognizeChanges( void );                  // to keep track of changes
  void refreshSaveIndicator( void );
};

#endif
