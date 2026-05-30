/***************************************************************************
                          mainwindow.cpp  -  description
                             -------------------
    begin                : Sun Mar 26 2000
    copyright            : (C) 2000 by Alexander Theel, (c) 2006-2007 Amit D. Chaudhary
    email                : alex.theel@gmx.net, amitch@rajgad.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "mainwindow.h"
#include "editorfindbar.h"
#include "markdownrenderer.h"

//#define DEBUGGING

#include <iostream>
#include <cstdlib>
#include <QFontDialog>
#include <QTimerEvent>
#include <QLabel>
#include <QPixmap>
#include <QCloseEvent>
#include <QList>
#include <QTextStream>
#include <QKeyEvent>
#include <QMenu>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollBar>
#include <QFileDialog>
#include <QPagedPaintDevice>
#include <QTimer>
#include <QDateTime>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QTextBlock>
#include <QTextCursor>

#include "../icons/lo16-app-tuxcards.xpm"
#include "../icons/lo32-app-tuxcards.xpm"

#ifdef DEBUGGING
  #include "../icons/debug/showText.xpm"
  #include "../icons/debug/xml.xpm"
#endif

#include "../information/converter.h"
#include "../information/CInformationCollection.h"
#include "../persister.h"
#include "../information/xmlpersister.h"
#include "../information/htmlwriter.h"
#include "../utilities/iniparser/configparser.h"

#include <QPrinter>
#include <QPaintDevice>
#include <QTextDocument>
#include <QPainter>
#include <QFontMetrics>

#include <qfontdatabase.h>
#include <qapplication.h>

#include "../utilities/strings.h"
#include "BookmarkButton.h"

#include "./dialogs/CFileEncryptionPasswordDialog.h"

#include "../utilities/crypt/StringCrypter.h"

#include "../version.h"
#include "../global.h"
#include "../Greetings.h"

#include "../utilities/CIconManager.h"
#include "./colorbar/CColorBar.h"
#define  getIcon(x)  CIconManager::getInstance().getIcon(x)


// -------------------------------------------------------------------------------
MainWindow::MainWindow(QString arg)
 : mpCollection( nullptr )
 , mpMenu( nullptr )
 , mpOptionsDialog( nullptr )
 , mConfiguration( CTuxCardsConfiguration::getInstance() )
 , mpSplit( nullptr )
 , mpColorBar( nullptr )
 , mpTree( nullptr )
 , mpSingleEntryView( nullptr )
 , mpEditor( nullptr )
 , mpQuickLoader( nullptr )
 , mpRecentFiles( nullptr )
 , mpStatusBar( nullptr )
 , mpLeftButton( nullptr )
 , mpRightButton( nullptr )
 , mHistory( )
 , mPasswdDialog( )          // with parent as nullptr
 , mpMainTools( nullptr )
 , mpEntryTools( nullptr )
 , mpEditorTools( nullptr )
 , miMainToolBarID( 0 )
 , miEntryToolBarID( 0 )
 , miEditorToolBarID( 0 )
 , mbStartupDone( false )
 , TIMER_ID( 0 )
// -------------------------------------------------------------------------------
{
   checkFirstTime();

   // build up mainwindow
   setWindowTitle(tr("TuxCards"));
   setWindowIcon(QIcon(QPixmap(lo32_app_tuxcards_xpm)));
   CIconManager::getInstance().setIconDirectory( mConfiguration.getStringValue( CTuxCardsConfiguration::S_ICON_DIR ) );

   QWidget* central = new QWidget(this);
   QHBoxLayout* centralLayout = new QHBoxLayout(central);
   centralLayout->setContentsMargins(0,0,0,0);
   centralLayout->setSpacing(0);

   mpColorBar = new CColorBar( central,
                               QColor(0,0,0), QColor(33,72,170),
                               "Tux", "Cards", QColor(Qt::white) );
   centralLayout->addWidget(mpColorBar);

   mpSplit = new QSplitter( central );
   checkPointer( mpSplit );
   centralLayout->addWidget(mpSplit);
   setCentralWidget( central );

   settingUpEditor( mpSplit );
   settingUpTree( mpSplit );
   mpSplit->insertWidget( 0, mpTree );
   mpSplit->setOpaqueResize(true);
   mpSplit->setStretchFactor(0, 1);
   mpSplit->setStretchFactor(1, 3);

   settingUpActions();
   settingUpMenu();
   settingUpToolBar();
   settingUpStatusBar();
   settingUpQuickLoader();

   // create optionsDialog
   mpOptionsDialog = new OptionsDialog( this, mConfiguration );
   if ( nullptr != mpOptionsDialog )
   {
      connect( mpOptionsDialog, &OptionsDialog::configurationChanged, this, &MainWindow::applyConfigurationMain );
   }

   // create little "showing-"dialog
   showDialog   = new QDialog(this);
   QGroupBox* gb = new QGroupBox(showDialog);
   QVBoxLayout* gbLayout = new QVBoxLayout(gb);
   showLabel    = new QLabel(tr("Saving ..."), gb);
   gbLayout->addWidget(showLabel);
   QVBoxLayout* showDlgLayout = new QVBoxLayout(showDialog);
   showDlgLayout->addWidget(gb);

   applyConfiguration();

   CHANGES=false;

   mHistory.setListener( this );

   // build up tree, if config-file ('.tuxcards') was found
   bool result = false;
   if ( arg != "" )
   {
      result = open(arg);
   }
   else if ( mConfiguration.getStringValue( CTuxCardsConfiguration::S_DATA_FILE_NAME ) != "" )
   {
      result = open(mConfiguration.getStringValue( CTuxCardsConfiguration::S_DATA_FILE_NAME ));
   } else {
   	// For empty datafile entry in the .tuxcards config file
   	clearAll();
   }

  // After this point any setX*ToolbarVisible() call is treated as
  // a user action and persists the change.
  mbStartupDone = true;

  // show the completed window
  show();
}


// -------------------------------------------------------------------------------
MainWindow::~MainWindow()
// -------------------------------------------------------------------------------
{
   deleteCollection( mpCollection );
}


// -------------------------------------------------------------------------------
// This is an emergency exit. The application will be quited if not enough
// memory is available to create all needed objects.
// -------------------------------------------------------------------------------
void MainWindow::lowMemoryExit( void )
// -------------------------------------------------------------------------------
{
   std::cout<<"TuxCards ERROR\nNot enough memory to run application.\nTuxCards"
              " will be stopped!!!"<<std::endl;
   QMessageBox::critical( nullptr, tr("TuxCards"), tr("Not enough memory to run "
                          "application.\nThe program will be quit."),
                          QMessageBox::Abort, QMessageBox::NoButton );
   QApplication::exit( -1 );
}

// -------------------------------------------------------------------------------
// Convenience method for 'lowMemoryExit()'.
// -------------------------------------------------------------------------------
void MainWindow::checkPointer( void* pPointer )
// -------------------------------------------------------------------------------
{
   if ( nullptr == pPointer )
   {
      lowMemoryExit();
   }
}


// -------------------------------------------------------------------------------
void MainWindow::historyStatusChanged( bool bHasPreviousElement,
                                       bool bHasNextElement )
// -------------------------------------------------------------------------------
{
   //std::cout<<"historyStatusChanged("<<bHasPreviousElement<<","<<bHasNextElement<<")"<<std::endl;
   if ( nullptr != mpLeftButton )
      mpLeftButton->setEnabled( bHasPreviousElement );

   if ( nullptr != mpRightButton )
      mpRightButton->setEnabled( bHasNextElement );
}


// -------------------------------------------------------------------------------
// Responses to the change of the active informationelement. Adds the new element
// to the history.
// -------------------------------------------------------------------------------
void MainWindow::activeInformationElementChanged( CInformationElement* pIE )
// -------------------------------------------------------------------------------
{
   if ( nullptr != pIE )
   {
      mHistory.addElement( *pIE );
   }
}


// -------------------------------------------------------------------------------
void MainWindow::activatePreviousHistoryElement( void )
// -------------------------------------------------------------------------------
{
   if ( nullptr != mpCollection )
      mpCollection->setActiveElement( mHistory.getPrevious() );
}

// -------------------------------------------------------------------------------
void MainWindow::activateNextHistoryElement( void )
// -------------------------------------------------------------------------------
{
   if ( nullptr != mpCollection )
      mpCollection->setActiveElement( mHistory.getNext() );
}


// -------------------------------------------------------------------------------
void MainWindow::settingUpEditor( QWidget* pParent )
// -------------------------------------------------------------------------------
{
   mpSingleEntryView = new CSingleEntryView( pParent );
   checkPointer( mpSingleEntryView );

   if ( nullptr == mpSingleEntryView )
      return;

   mpEditor = mpSingleEntryView->getEditor();

   connect(mpEditor, &QTextEdit::textChanged,     this, &MainWindow::recognizeChanges);
   connect(mpEditor, &Editor::formatRecognized,   this, &MainWindow::showRecognizedFormat);
   connect(mpEditor, &Editor::resetFormattingRequested,
                                                  this, &MainWindow::resetFormattingToPlainText);
   connect(mpEditor, &QTextEdit::cursorPositionChanged, this, [this](){
      textAlignmentChanged((int)mpEditor->alignment());
   });
}


// -------------------------------------------------------------------------------
void MainWindow::settingUpTree( QWidget* pParent )
// -------------------------------------------------------------------------------
{
   mpTree  = new CTree( pParent, mConfiguration );
   checkPointer( mpTree );

   connect( mpTree, &CTree::showMessage,             this, &MainWindow::showMessage );
   connect( mpTree, &CTree::makeVisible,             this, &MainWindow::makeVisible );
   connect( mpTree, &CTree::addEntryToBookmarksSignal, this, &MainWindow::addElementToBookmarksEvent );
   // When the tree's right-click "Change format" submenu rewrites an
   // entry's body+format, walk the editor through the normal
   // activeElementChanged path so it adopts the new mode.
   connect( mpTree, &CTree::formatChanged, this, [this](CInformationElement* elem) {
      if ( elem && mpEditor )
         mpEditor->activeInformationElementChanged( elem );
      recognizeChanges();
   });

	// On a drag start, move editor contents to InformationElement.
   connect(mpTree, &CTree::dragStarted, mpEditor, &Editor::writeCurrentTextToActiveInformationElement);
}



// -------------------------------------------------------------------------------
void MainWindow::settingUpStatusBar( void )
// -------------------------------------------------------------------------------
{
   mpStatusBar = statusBar();
   checkPointer( mpStatusBar );

   mstatusBar_NumElements=new QLabel( " ", mpStatusBar );
   mstatusBar_NumElements->setFixedWidth(120);
   mpStatusBar->addPermanentWidget(mstatusBar_NumElements, 0);

   statusBar_ChangeLabel=new QLabel( " ", mpStatusBar );
   statusBar_ChangeLabel->setFixedWidth(10);
   mpStatusBar->addPermanentWidget(statusBar_ChangeLabel, 0);

   mstatusBar_EncryptedLabel = new QLabel(" ", mpStatusBar );
   mstatusBar_EncryptedLabel->setFixedWidth(30);
   mpStatusBar->addPermanentWidget(mstatusBar_EncryptedLabel, 0);

   mstatusBar_SaveIndicator = new QLabel( tr("Not saved"), mpStatusBar );
   mstatusBar_SaveIndicator->setMinimumWidth(140);
   mpStatusBar->addPermanentWidget(mstatusBar_SaveIndicator, 0);

   mLastSaveEpochMs = 0;
   mpSaveIndicatorTimer = new QTimer(this);
   connect( mpSaveIndicatorTimer, &QTimer::timeout, this, &MainWindow::refreshSaveIndicator );
   mpSaveIndicatorTimer->start( 1000 );
}


void MainWindow::refreshSaveIndicator( void )
{
   if ( !mstatusBar_SaveIndicator )
      return;

   if ( CHANGES )
   {
      mstatusBar_SaveIndicator->setText( tr("Unsaved changes") );
      return;
   }
   if ( mLastSaveEpochMs == 0 )
   {
      mstatusBar_SaveIndicator->setText( tr("Not saved") );
      return;
   }

   const qint64 dt = (QDateTime::currentMSecsSinceEpoch() - mLastSaveEpochMs) / 1000;
   QString human;
   if ( dt < 5 )       human = tr("Saved just now");
   else if ( dt < 60 ) human = tr("Saved %1s ago").arg(dt);
   else if ( dt < 3600 ) human = tr("Saved %1m ago").arg(dt/60);
   else                human = tr("Saved %1h ago").arg(dt/3600);
   mstatusBar_SaveIndicator->setText(human);
}


void MainWindow::settingUpActions( void )
{
   mfileEncryptFile = new QAction( getIcon("fileunlock"), tr("Toggle file &encryption"), this);
   mfileEncryptFile->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
   mfileEncryptFile->setCheckable(true);
   connect( mfileEncryptFile, &QAction::triggered, this, &MainWindow::toggleFileEncryption );

   editUndoAction = new QAction( getIcon("undo"), tr("&Undo"), this);
   editUndoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
   connect( mpEditor,       &QTextEdit::undoAvailable, editUndoAction, &QAction::setEnabled );
   connect( editUndoAction, &QAction::triggered,       mpEditor,       &QTextEdit::undo );

   editRedoAction = new QAction( getIcon("redo"), tr("&Redo"), this);
   editRedoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
   connect( mpEditor,       &QTextEdit::redoAvailable, editRedoAction, &QAction::setEnabled );
   connect( editRedoAction, &QAction::triggered,       mpEditor,       &QTextEdit::redo );

   editCopyAction = new QAction( getIcon("editcopy"), tr("&Copy"), this);
   editCopyAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
   connect( mpEditor,       &QTextEdit::copyAvailable, editCopyAction, &QAction::setEnabled );
   connect( editCopyAction, &QAction::triggered,       mpEditor,       &QTextEdit::copy );

   editSetEntryColor = new QAction( getIcon("editentrycolor"), tr("Set &Entry Color..."), this);
   editSetEntryColor->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
   connect( editSetEntryColor, &QAction::triggered, mpTree, &CTree::setEntryColor );

   editSetEntrySubTreeColor = new QAction( getIcon("editentrysubtreecolor"), tr("Set Entry &Sub-Tree Color..."), this);
   editSetEntrySubTreeColor->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
   connect( editSetEntrySubTreeColor, &QAction::triggered, mpTree, &CTree::setEntrySubTreeColor );
}


// -------------------------------------------------------------------------------
void MainWindow::settingUpMenu( void )
{
   QMenu* file = new QMenu( tr("&File"), this );
   file->addAction( getIcon("filenew" ), tr("&New File"),       this, &MainWindow::newFile, QKeySequence(Qt::CTRL | Qt::Key_N));
   file->addAction( getIcon("fileopen"), tr("&Open File..."),   this, qOverload<>(&MainWindow::open),    QKeySequence(Qt::CTRL | Qt::Key_O));
   file->addAction( getIcon("filesave"), tr("&Save"),           this, qOverload<>(&MainWindow::save),    QKeySequence(Qt::CTRL | Qt::Key_S));
   file->addAction(                      tr("Save &As..."),     this, &MainWindow::saveAs );
   file->addAction( getIcon("fileprint"),tr("&Print current entry..."),  this, &MainWindow::print,        QKeySequence(Qt::CTRL | Qt::Key_P) );
   file->addAction( getIcon("fileprint"),tr("Print pre&view..."),         this, &MainWindow::printPreview, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P) );

   file->addSeparator();
   file->addAction(mfileEncryptFile);

   file->addSeparator();
   mpRecentFiles = new RecentFileList(this, file, mConfiguration.getStringValue( CTuxCardsConfiguration::S_RECENT_FILES ));
   checkPointer( mpRecentFiles );

   connect( mpRecentFiles, &RecentFileList::openFile, this, &MainWindow::slotSaveAndLoadNewFile );

   file->addSeparator();
   file->addAction(                      tr("Export to &HTML..."),                       this, &MainWindow::exportHTML );
   file->addAction(                      tr("Export current entry to &Markdown..."),     this, &MainWindow::exportEntryMarkdown );
   file->addAction(                      tr("Import &Markdown into current entry..."),   this, &MainWindow::importEntryMarkdown );
   file->addSeparator();
   file->addAction( getIcon("exit"),     tr("&Exit"), this, &MainWindow::exit, QKeySequence(Qt::CTRL | Qt::Key_Q) );


   QMenu* edit = new QMenu( tr("&Edit"), this );
   edit->addAction(editUndoAction);
   edit->addAction(editRedoAction);
   edit->addSeparator();

   if ( nullptr != mpEditor )
   {
      edit->addAction( getIcon("editcut"),   tr("Cu&t"),   mpEditor, &QTextEdit::cut,   QKeySequence(Qt::CTRL | Qt::Key_X) );
      edit->addAction(editCopyAction);
      edit->addAction( getIcon("editpaste"), tr("&Paste"), mpEditor, &QTextEdit::paste, QKeySequence(Qt::CTRL | Qt::Key_V) );
      edit->addSeparator();
      edit->addAction(                       tr("Select &All"), mpEditor, &QTextEdit::selectAll, QKeySequence(Qt::CTRL | Qt::Key_A) );

      edit->addSeparator();
      {
         QAction* a = edit->addAction( tr("&Find in entry..."), this, &MainWindow::editorFind,
                                       QKeySequence(Qt::Key_F3) );
         a->setShortcutContext( Qt::WindowShortcut );

         a = edit->addAction( tr("R&eplace..."), this, &MainWindow::editorReplace,
                              QKeySequence(Qt::CTRL | Qt::Key_H) );
         a->setShortcutContext( Qt::WindowShortcut );

         a = edit->addAction( getIcon("find"), tr("Search in &Tree..."),
                              this, &MainWindow::search,
                              QKeySequence(Qt::CTRL | Qt::Key_F) );
         a->setShortcutContext( Qt::WindowShortcut );
      }

      edit->addSeparator();
      edit->addAction( tr("&Bold"),      this, &MainWindow::textBold,   QKeySequence(Qt::CTRL | Qt::Key_B) );
      edit->addAction( tr("&Italic"),    this, &MainWindow::textItalic, QKeySequence(Qt::CTRL | Qt::Key_I) );
      edit->addAction( tr("&Underline"), this, &MainWindow::textUnder,  QKeySequence(Qt::CTRL | Qt::Key_U) );
      edit->addAction( tr("&Color..."),  this, &MainWindow::textColor,  QKeySequence(Qt::CTRL | Qt::Key_M) );

      edit->addSeparator();
      edit->addAction(editSetEntryColor);
      edit->addAction(editSetEntrySubTreeColor);

      edit->addSeparator();
      edit->addAction( tr("Insert &Image..."),      mpEditor, &Editor::insertImage,             QKeySequence(Qt::CTRL | Qt::Key_P) );
      edit->addAction( tr("Insert Current &Date"),  this,     &MainWindow::insertCurrentDate,   QKeySequence(Qt::CTRL | Qt::Key_D) );
      edit->addAction( tr("Insert Current T&ime"),  this,     &MainWindow::insertCurrentTime,   QKeySequence(Qt::CTRL | Qt::Key_T) );
      edit->addSeparator();
      edit->addAction( tr("Reset &formatting (convert to plain text)"),
                       this, &MainWindow::resetFormattingToPlainText );
      edit->addSeparator();
      edit->addAction( tr("&Options..."), this, &MainWindow::editConfiguration );
   }

   QMenu* toolbars = new QMenu( tr("Toolbars"), this );
   miMainToolBarID   = toolbars->addAction( tr("Show Main Toolbar") );
   miEntryToolBarID  = toolbars->addAction( tr("Show Entry Manipulation Toolbar") );
   miEditorToolBarID = toolbars->addAction( tr("Show Editor Toolbar") );
   miMainToolBarID->setCheckable(true);
   miEntryToolBarID->setCheckable(true);
   miEditorToolBarID->setCheckable(true);
   miMainToolBarID->setChecked(   mConfiguration.getBoolValue( CTuxCardsConfiguration::B_SHOW_MAIN_TOOLBAR ) );
   miEntryToolBarID->setChecked(  mConfiguration.getBoolValue( CTuxCardsConfiguration::B_SHOW_ENTRY_TOOLBAR ) );
   miEditorToolBarID->setChecked( mConfiguration.getBoolValue( CTuxCardsConfiguration::B_SHOW_EDITOR_TOOLBAR ) );
   connect( miMainToolBarID,   &QAction::toggled, this, &MainWindow::setMainToolbarVisible );
   connect( miEntryToolBarID,  &QAction::toggled, this, &MainWindow::setEntryToolbarVisible );
   connect( miEditorToolBarID, &QAction::toggled, this, &MainWindow::setEditorToolbarVisible );

   QMenu* view = new QMenu( tr("&View"), this );
   view->addMenu( toolbars );
   view->addAction( tr("&Word Count"), this, &MainWindow::wordCount );

   QMenu* about = new QMenu( tr("&About"), this );
   about->addAction( tr("&Keyboard Shortcuts"), this, &MainWindow::showKBShortcuts );
   about->addSeparator();
   about->addAction( QIcon(QPixmap(lo16_app_tuxcards_xpm)), tr("About TuxCards"), this, &MainWindow::showAbout );

   mpMenu = menuBar();
   if ( nullptr != mpMenu )
   {
      mpMenu->addMenu( file );
      mpMenu->addMenu( edit );
      mpMenu->addMenu( view );
      mpMenu->addMenu( about );
   }
}


// -------------------------------------------------------------------------------
void MainWindow::settingUpToolBar( void )
{
  mpMainTools = addToolBar(tr("Main"));
  mpMainTools->setIconSize(QSize(18,18));
  mpMainTools->setToolButtonStyle(Qt::ToolButtonIconOnly);

  QAction* clearTool = mpMainTools->addAction( getIcon("filenew"), tr("Create a new file"), this, &MainWindow::newFile);
  mpMainTools->addSeparator();
  QAction* openTool  = mpMainTools->addAction( getIcon("fileopen"), tr("Open a new file"), this, qOverload<>(&MainWindow::open));
  QAction* saveTool  = mpMainTools->addAction( getIcon("filesave"), tr("Save current file (Ctrl+S)"), this, qOverload<>(&MainWindow::save));
  QAction* printTool        = mpMainTools->addAction( getIcon("fileprint"), tr("Print current entry"), this, &MainWindow::print);
  QAction* printPreviewTool = mpMainTools->addAction( getIcon("fileprint"), tr("Print preview (Ctrl+Shift+P)"), this, &MainWindow::printPreview);

  mpMainTools->addAction(mfileEncryptFile);

  mpMainTools->addSeparator();
  mpMainTools->addAction(editUndoAction);
  mpMainTools->addAction(editRedoAction);

  QAction* editCutTool   = mpMainTools->addAction( getIcon("editcut"), tr("Cut (Ctrl+X)"), mpEditor, &QTextEdit::cut);
  mpMainTools->addAction(editCopyAction);
  QAction* editPasteTool = mpMainTools->addAction( getIcon("editpaste"), tr("Paste (Ctrl+V)"), mpEditor, &QTextEdit::paste);

  mpMainTools->addSeparator();
  mpMainTools->addAction(editSetEntryColor);
  mpMainTools->addAction(editSetEntrySubTreeColor);

  mpMainTools->addSeparator();
  QAction* findTool = mpMainTools->addAction( getIcon("find"), tr("Search across tree (Ctrl+F)"), this, &MainWindow::search);
  mpMainTools->addSeparator();

  clearTool->setWhatsThis(tr("<b>Clear whole Tree</b>"));
  openTool->setWhatsThis(tr("<b>Open a new File</b>"));
  saveTool->setWhatsThis(tr("<b>Save Data to File</b> (Ctrl+S)"));
  printTool->setWhatsThis(tr("<b>Print current Entry</b>"));
  printPreviewTool->setWhatsThis(tr("<b>Print preview</b> (Ctrl+Shift+P)"));
  editUndoAction->setWhatsThis(tr("<b>Undo</b> (Ctrl+Z)"));
  editRedoAction->setWhatsThis(tr("<b>Redo</b> (Ctrl+Y)"));
  editCutTool->setWhatsThis(tr("<b>Cut</b> (Ctrl+X)"));
  editCopyAction->setWhatsThis(tr("<b>Copy</b> (Ctrl+C)"));
  editPasteTool->setWhatsThis(tr("<b>Paste</b> (Ctrl+V)"));
  findTool->setWhatsThis(tr("<b>Search across the entire tree</b> (Ctrl+F).<br/>"
                            "For an in-entry find/replace use F3 / Ctrl+H."));


  mpEntryTools = addToolBar(tr("Entry"));
  mpEntryTools->setIconSize(QSize(18,18));
  mpEntryTools->setToolButtonStyle(Qt::ToolButtonIconOnly);

  textFormatTool = mpEntryTools->addAction( getIcon("filenew"), tr("Converts the Text Format"), this, &MainWindow::changeInformationFormat );

  mpEntryTools->addSeparator();
  QAction* addTreeElementTool = mpEntryTools->addAction( getIcon("addTreeElement"), tr("Add Entry (INSERT)"), mpTree, &CTree::addElement );
  QAction* changePropertyTool = mpEntryTools->addAction( getIcon("changeProperty"), tr("Change Properties"), mpTree, &CTree::changeActiveElementProperties );
  QAction* removeEntryTool   = mpEntryTools->addAction( getIcon("delete"), tr("Remove active Entry (DELETE)"), mpTree, &CTree::askForDeletion );

  mpEntryTools->addSeparator();
  QAction* ieUpTool   = mpEntryTools->addAction( getIcon("upArrow"), tr("Move Current Entry Upwards"), this, &MainWindow::moveElementUp );
  QAction* ieDownTool = mpEntryTools->addAction( getIcon("downArrow"), tr("Move Current Entry Downwards"), this, &MainWindow::moveElementDown );

  mpEntryTools->addSeparator();
  mpLeftButton  = mpEntryTools->addAction( getIcon("back"), tr("Last Entry accessed in History (Alt+Left)"), this, &MainWindow::activatePreviousHistoryElement );
  mpLeftButton->setEnabled(false);
  mpRightButton = mpEntryTools->addAction( getIcon("forward"), tr("Next Entry in History (Alt+Right)"), this, &MainWindow::activateNextHistoryElement );
  mpRightButton->setEnabled(false);

  textFormatTool->setWhatsThis(tr("Text format toggle"));
  addTreeElementTool->setWhatsThis(tr("<b>Add Entry</b> (INSERT)"));
  changePropertyTool->setWhatsThis(tr("<b>Change Property</b>"));
  removeEntryTool->setWhatsThis(tr("<b>Remove active Entry</b> (DELETE)"));
  ieUpTool->setWhatsThis(tr("<b>Move Up</b>"));
  ieDownTool->setWhatsThis(tr("<b>Move Down</b>"));
  mpLeftButton->setWhatsThis(tr("<b>History, Back</b> (Alt+Left)"));
  mpRightButton->setWhatsThis(tr("<b>History, Forward</b> (Alt+Right)"));


  addToolBarBreak();
  mpEditorTools = addToolBar(tr("Editor"));
  mpEditorTools->setIconSize(QSize(18,18));
  mpEditorTools->setToolButtonStyle(Qt::ToolButtonIconOnly);

  pComboListStyle = new QComboBox( mpEditorTools );
  pComboListStyle->addItem( tr("Standard") );
  pComboListStyle->addItem( tr("Bullet List (Disc)") );
  pComboListStyle->addItem( tr("Bullet List (Circle)") );
  pComboListStyle->addItem( tr("Bullet List (Square)") );
  pComboListStyle->addItem( tr("Ordered List (Decimal)") );
  pComboListStyle->addItem( tr("Ordered List (Alpha lower)") );
  pComboListStyle->addItem( tr("Ordered List (Alpha upper)") );
  mpEditorTools->addWidget(pComboListStyle);
  connect( pComboListStyle, &QComboBox::activated, this, &MainWindow::textListStyle );

  pComboFont = new QComboBox( mpEditorTools );
  pComboFont->setEditable(true);
  pComboFont->addItems( QFontDatabase::families() );
  mpEditorTools->addWidget(pComboFont);
  connect( pComboFont, &QComboBox::textActivated, this, &MainWindow::textFontFamily );
  pComboFont->lineEdit()->setText( QApplication::font().family() );

  pComboSize = new QComboBox( mpEditorTools );
  pComboSize->setEditable(true);
  for ( int sz : QFontDatabase::standardSizes() )
     pComboSize->addItem( QString::number( sz ) );
  mpEditorTools->addWidget(pComboSize);
  connect( pComboSize, &QComboBox::textActivated, this, &MainWindow::textFontSize );
  pComboSize->lineEdit()->setText( QString::number( QApplication::font().pointSize() ) );


  textBoldTool = mpEditorTools->addAction( getIcon("text_bold"), tr("Bold (Ctrl+B)"), this, &MainWindow::textBold );
  textBoldTool->setCheckable(true);

  textItalicTool = mpEditorTools->addAction( getIcon("text_italic"), tr("Italic (Ctrl+I)"), this, &MainWindow::textItalic );
  textItalicTool->setCheckable(true);

  textUnderTool = mpEditorTools->addAction( getIcon("text_under"), tr("Underline (Ctrl+U)"), this, &MainWindow::textUnder );
  textUnderTool->setCheckable(true);

  QPixmap dummy(1,1); dummy.fill(Qt::black);
  textColorTool = mpEditorTools->addAction( QIcon(dummy), tr("Color"), this, &MainWindow::textColor );
  textColorChanged(Qt::black);

  mpEditorTools->addSeparator();
  textLeftTool = mpEditorTools->addAction( getIcon("text_left"), tr("Align Left"), this, &MainWindow::textLeft );
  textLeftTool->setCheckable(true);

  textCenterTool = mpEditorTools->addAction( getIcon("text_center"), tr("Center"), this, &MainWindow::textHCenter );
  textCenterTool->setCheckable(true);

  textRightTool = mpEditorTools->addAction( getIcon("text_right"), tr("Align Right"), this, &MainWindow::textRight );
  textRightTool->setCheckable(true);

  textBlockTool = mpEditorTools->addAction( getIcon("text_block"), tr("Text Block"), this, &MainWindow::textBlock );
  textBlockTool->setCheckable(true);

  // ---- Markdown-mode toolbar additions ------------------------------
  // All hidden by default; showRecognizedFormat() flips visibility when
  // the active element's format is MARKDOWN.
  mpEditorTools->addSeparator();
  mdPreviewToggleAction = mpEditorTools->addAction(
        getIcon("find"), tr("Preview Markdown"), this,
        [this](bool checked){ toggleMarkdownPreview(checked); });
  mdPreviewToggleAction->setCheckable(true);
  mdPreviewToggleAction->setToolTip(tr("Toggle rendered Markdown preview"));
  mdPreviewToggleAction->setVisible(false);

  mdHelperActions.clear();
  auto addMdHelper = [&](const QString& label, void (MainWindow::*slot)()) {
     QAction* a = mpEditorTools->addAction(label, this, slot);
     a->setVisible(false);
     mdHelperActions.append(a);
     return a;
  };
  addMdHelper(tr("B"),    &MainWindow::mdInsertBold)         ->setToolTip(tr("Bold (**...**)"));
  addMdHelper(tr("I"),    &MainWindow::mdInsertItalic)       ->setToolTip(tr("Italic (_..._)"));
  addMdHelper(tr("<>"),   &MainWindow::mdInsertCode)         ->setToolTip(tr("Inline code (`...`)"));
  addMdHelper(tr("H"),    &MainWindow::mdInsertHeading)      ->setToolTip(tr("Heading (#)"));
  addMdHelper(tr("•"),    &MainWindow::mdInsertBulletList)   ->setToolTip(tr("Bullet list (-)"));
  addMdHelper(tr("1."),   &MainWindow::mdInsertNumberedList) ->setToolTip(tr("Numbered list (1.)"));
  addMdHelper(tr("[ ]"),  &MainWindow::mdInsertLink)         ->setToolTip(tr("Link ([text](url))"));
  addMdHelper(tr("```"),  &MainWindow::mdInsertCodeFence)    ->setToolTip(tr("Code fence (```)"));
  addMdHelper(tr("⊞"),    &MainWindow::mdInsertTable)        ->setToolTip(tr("Table skeleton"));
  // These two only do something useful when the matching optional
  // feature was compiled in; hide them entirely otherwise.
  if ( MarkdownRenderer::mathEnabled() )
     addMdHelper(tr("∑"),  &MainWindow::mdInsertMath)        ->setToolTip(tr("Inline math ($...$)"));
  if ( MarkdownRenderer::diagramsEnabled() )
     addMdHelper(tr("◈"),  &MainWindow::mdInsertDiagram)     ->setToolTip(tr("Graphviz diagram (```dot)"));

  textBoldTool->setWhatsThis(tr("<b>Bold</b>"));
  textItalicTool->setWhatsThis(tr("<b>Italic</b>"));
  textUnderTool->setWhatsThis(tr("<b>Underline</b>"));
  textColorTool->setWhatsThis(tr("<b>Text Color</b>"));
  textLeftTool->setWhatsThis(tr("<b>Align Left</b>"));
  textCenterTool->setWhatsThis(tr("<b>Center</b>"));
  textRightTool->setWhatsThis(tr("<b>Align Right</b>"));
  textBlockTool->setWhatsThis(tr("<b>Text Block</b>"));


  connect( mpEditor, &QTextEdit::currentCharFormatChanged, this, [this](const QTextCharFormat& fmt){
     textFontChanged(fmt.font());
     textColorChanged(fmt.foreground().color());
  });


  setMainToolbarVisible(   mConfiguration.getBoolValue( CTuxCardsConfiguration::B_SHOW_MAIN_TOOLBAR )   );
  setEntryToolbarVisible(  mConfiguration.getBoolValue( CTuxCardsConfiguration::B_SHOW_ENTRY_TOOLBAR )  );
  setEditorToolbarVisible( mConfiguration.getBoolValue( CTuxCardsConfiguration::B_SHOW_EDITOR_TOOLBAR ) );


#ifdef DEBUGGING
  std::cout<<"!!! still having debug turned on"<<std::endl;
  QToolBar* debugTools = addToolBar("Debug");
  QPixmap debugShowRTFSource = QPixmap(showText_xpm);
  debugTools->addAction(QIcon(debugShowRTFSource), "Debug: Shows the RTF-TextSource", this, &MainWindow::debugShowRTFTextSource);

  QPixmap debugShowXMLCode = QPixmap(xml_xpm);
  debugTools->addAction(QIcon(debugShowXMLCode), "Debug: Shows the XML-Representation", this, &MainWindow::debugShowXMLCode);
#endif
}


// -------------------------------------------------------------------------------
void MainWindow::settingUpQuickLoader( void )
{
   mpQuickLoader = addToolBar(tr("Bookmarks"));
   mpQuickLoader->setIconSize(QSize(16,16));
   mpQuickLoader->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
   addToolBar(Qt::BottomToolBarArea, mpQuickLoader);
   checkPointer( mpQuickLoader );

   // Reload persisted bookmarks (comma-separated string of paths)
   QString sStored = mConfiguration.getStringValue( CTuxCardsConfiguration::S_BOOKMARKS );
   if ( !sStored.isEmpty() ) {
      for ( const QString& sPath : sStored.split('\n', Qt::SkipEmptyParts) ) {
         Path p( sPath );
         QString label = p.getPathList().isEmpty() ? sPath : p.getPathList().last();
         BookmarkButton* b = new BookmarkButton(
                  QPixmap(), label,
                  mpQuickLoader, p );
         mpQuickLoader->addWidget( b );
         connect( b, &BookmarkButton::activatedSignal, this, &MainWindow::quicklyLoad );
      }
   }
}


// Persist the current bookmark bar buttons to config.
static QString collectBookmarkPaths( QToolBar* tb )
{
   QStringList paths;
   if ( !tb ) return QString();
   const auto children = tb->findChildren<BookmarkButton*>();
   for ( BookmarkButton* b : children ) {
      Path p = b->getPath();
      paths << p.toString();
   }
   return paths.join('\n');
}


/**
 * Adds the currently active element to the bookmark list.
 */
void MainWindow::addElementToBookmarksEvent( void )
{
   if ( nullptr == mpCollection || nullptr == mpQuickLoader )
      return;

   CInformationElement* pElement = mpCollection->getActiveElement();
   if ( nullptr == pElement )
      return;

   BookmarkButton* b = new BookmarkButton(
            QPixmap( pElement->getIconFileName() ),
            pElement->getDescription(), mpQuickLoader,
            Path( pElement ) );
   mpQuickLoader->addWidget( b );
   connect( b, &BookmarkButton::activatedSignal, this, &MainWindow::quicklyLoad );

   mConfiguration.setStringValue( CTuxCardsConfiguration::S_BOOKMARKS,
                                  collectBookmarkPaths(mpQuickLoader) );
   if (mbStartupDone)
      mConfiguration.saveToFile();
}


void MainWindow::quicklyLoad(Path* path)
{
   if ( !path || !mpCollection )
      return;
   mpCollection->setActiveElement( *path );
}


// -------------------------------------------------------------------------------
void MainWindow::changeInformationFormat()
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpCollection) || (nullptr == mpEditor) )
      return;

   CInformationElement* pActiveElement = mpCollection->getActiveElement();
   if ( nullptr == pActiveElement )
   {
      QMessageBox::information( 0, tr("Converter"), tr("There is no active entry."),
                                QMessageBox::Abort );
      return;
   }

   if (QMessageBox::Cancel == QMessageBox::warning(this, tr("Converting Information Format"),
                                                   tr("Are you sure to change the information "
                                                   "format.\nSome of the text layout will be lost."),
                                                   QMessageBox::Yes,
                                                   QMessageBox::Cancel,
                                                   QMessageBox::NoButton))
   {
      return;
   }

   // converting (toolbar shortcut — flips TEXT<->HTML, full
   // cross-conversion lives on the tree's right-click "Change format" submenu)
   mpEditor->writeCurrentTextToActiveInformationElement();
   Converter::convert( *pActiveElement );

   // Re-enter the editor through activeInformationElementChanged so it
   // picks the right acceptRichText/readOnly for the new format.
   mpEditor->activeInformationElementChanged( pActiveElement );
   mpCollection->setActiveElement( pActiveElement );
}



// -------------------------------------------------------------------------------
void MainWindow::showRecognizedFormat(InformationFormat format)
// -------------------------------------------------------------------------------
{
  textFormatTool->setIcon(QIcon(format.getPixmap()));

  // HTML-only formatting controls — Bold/Italic/Align/etc act on
  // QTextCharFormat which only exists in rich-text mode. The font /
  // size / list-style combos in the toolbar manipulate the same
  // thing, so disable them together with the buttons.
  bool isHtml = format.equals(InformationFormat::HTML);
  textBoldTool->setEnabled(isHtml);
  textItalicTool->setEnabled(isHtml);
  textUnderTool->setEnabled(isHtml);
  textColorTool->setEnabled(isHtml);

  textLeftTool->setEnabled(isHtml);
  textCenterTool->setEnabled(isHtml);
  textRightTool->setEnabled(isHtml);
  textBlockTool->setEnabled(isHtml);

  if (pComboListStyle) pComboListStyle->setEnabled(isHtml);
  if (pComboFont)      pComboFont->setEnabled(isHtml);
  if (pComboSize)      pComboSize->setEnabled(isHtml);

  // MARKDOWN-only controls (preview toggle + helper buttons).
  bool isMd = format.equals(InformationFormat::MARKDOWN);
  mdPreviewToggleAction->setVisible(isMd);
  for (QAction* a : mdHelperActions)
     a->setVisible(isMd);
  // Switching away from a markdown entry while preview is on would
  // strand the toggle in a wrong state — flip it off explicitly.
  if (!isMd && mdPreviewToggleAction->isChecked())
     mdPreviewToggleAction->setChecked(false);
}


// -------------------------------------------------------------------------------
// "Reset formatting" — drop all rich-text markup from the active entry,
// switch its format to TEXT, and load the result back into the editor.
// Reachable from the Edit menu and the editor's context menu.
void MainWindow::resetFormattingToPlainText()
// -------------------------------------------------------------------------------
{
   if ( !mpEditor || !mpCollection ) return;
   CInformationElement* elem = mpCollection->getActiveElement();
   if ( !elem ) return;

   // Use the editor's currently-visible plain text — it already
   // collapses HTML tags / markdown markup to whatever the user sees.
   mpEditor->writeCurrentTextToActiveInformationElement();
   const QString plain = mpEditor->toPlainText();

   elem->setInformationFormat( &InformationFormat::TEXT );
   elem->setInformation( plain );
   // Re-enter the editor so it picks up the new format (acceptRichText
   // off, formatting toolbar disabled via showRecognizedFormat).
   mpEditor->activeInformationElementChanged( elem );
   recognizeChanges();
}


// ---- Markdown preview + helper actions --------------------------------------

// -------------------------------------------------------------------------------
void MainWindow::toggleMarkdownPreview( bool on )
// -------------------------------------------------------------------------------
{
   if ( !mpEditor || !mpCollection || !mpCollection->getActiveElement() )
      return;
   CInformationElement* elem = mpCollection->getActiveElement();
   if ( elem->getInformationFormat() != &InformationFormat::MARKDOWN )
      return;

   if (on) {
      // Capture the in-editor source as the authoritative .md, render it.
      mdSourceStash = mpEditor->toPlainText();
      mpEditor->setAcceptRichText(true);
      mpEditor->setReadOnly(true);
      MarkdownRenderer::renderInto(mpEditor->document(), mdSourceStash);
   } else {
      // Restore raw source for editing.
      mpEditor->setReadOnly(false);
      mpEditor->setAcceptRichText(false);
      mpEditor->setPlainText(mdSourceStash);
      mdSourceStash.clear();
   }
}

// Wrap the current selection (or insert a placeholder) between `lhs`
// and `rhs`. Used by the markdown helper buttons.
static void mdWrapSelection( QTextEdit* ed, const QString& lhs,
                             const QString& rhs, const QString& placeholder )
{
   QTextCursor c = ed->textCursor();
   QString sel = c.selectedText();
   if (sel.isEmpty())
      sel = placeholder;
   c.insertText(lhs + sel + rhs);
}

void MainWindow::mdInsertBold()   { if (mpEditor) mdWrapSelection(mpEditor, "**", "**", tr("bold")); }
void MainWindow::mdInsertItalic() { if (mpEditor) mdWrapSelection(mpEditor, "_",  "_",  tr("italic")); }
void MainWindow::mdInsertCode()   { if (mpEditor) mdWrapSelection(mpEditor, "`",  "`",  tr("code")); }

void MainWindow::mdInsertHeading()
{
   if (!mpEditor) return;
   QTextCursor c = mpEditor->textCursor();
   c.movePosition(QTextCursor::StartOfLine);
   c.insertText("# ");
}

void MainWindow::mdInsertBulletList()
{
   if (!mpEditor) return;
   QTextCursor c = mpEditor->textCursor();
   c.movePosition(QTextCursor::StartOfLine);
   c.insertText("- ");
}

void MainWindow::mdInsertNumberedList()
{
   if (!mpEditor) return;
   QTextCursor c = mpEditor->textCursor();
   c.movePosition(QTextCursor::StartOfLine);
   c.insertText("1. ");
}

void MainWindow::mdInsertLink()
{
   if (!mpEditor) return;
   QTextCursor c = mpEditor->textCursor();
   QString sel = c.selectedText();
   if (sel.isEmpty()) sel = tr("text");
   c.insertText(QStringLiteral("[%1](https://)").arg(sel));
}

void MainWindow::mdInsertCodeFence()
{
   if (!mpEditor) return;
   QTextCursor c = mpEditor->textCursor();
   QString sel = c.selectedText();
   // QTextCursor::selectedText uses U+2029 for paragraph breaks — convert.
   sel.replace(QChar::ParagraphSeparator, QChar('\n'));
   if (sel.isEmpty()) sel = tr("code");
   c.insertText(QStringLiteral("\n```\n%1\n```\n").arg(sel));
}

void MainWindow::mdInsertTable()
{
   if (!mpEditor) return;
   mpEditor->textCursor().insertText(
      "\n| col1 | col2 |\n"
      "|------|------|\n"
      "|      |      |\n");
}

void MainWindow::mdInsertMath()
{
   if (!mpEditor) return;
   QTextCursor c = mpEditor->textCursor();
   QString sel = c.selectedText();
   if (sel.isEmpty()) sel = tr("x^2");
   c.insertText(QStringLiteral("$%1$").arg(sel));
}

void MainWindow::mdInsertDiagram()
{
   if (!mpEditor) return;
   mpEditor->textCursor().insertText(
      "\n```dot\n"
      "digraph {\n"
      "  A -> B;\n"
      "}\n"
      "```\n");
}



// -------------------------------------------------------------------------------
void MainWindow::textListStyle( int i )
// -------------------------------------------------------------------------------
{
   if ( !mpEditor )
      return;

   QTextCursor c = mpEditor->textCursor();
   if ( i == 0 )
   {
      QTextBlockFormat bf;
      bf.setIndent(0);
      c.setBlockFormat(bf);
   }
   else
   {
      QTextListFormat lf;
      QTextListFormat::Style style = QTextListFormat::ListDisc;
      switch (i)
      {
        case 1: style = QTextListFormat::ListDisc; break;
        case 2: style = QTextListFormat::ListCircle; break;
        case 3: style = QTextListFormat::ListSquare; break;
        case 4: style = QTextListFormat::ListDecimal; break;
        case 5: style = QTextListFormat::ListLowerAlpha; break;
        case 6: style = QTextListFormat::ListUpperAlpha; break;
      }
      lf.setStyle(style);
      c.createList(lf);
   }
   mpEditor->setFocus();
}


// -------------------------------------------------------------------------------
void MainWindow::textFontFamily( const QString &f )
// -------------------------------------------------------------------------------
{
   if ( !mpEditor )
      return;

   mpEditor->setFontFamily( f );
   mpEditor->viewport()->setFocus();
}

// -------------------------------------------------------------------------------
void MainWindow::textFontSize( const QString &p )
// -------------------------------------------------------------------------------
{
   if ( !mpEditor )
      return;
   mpEditor->setFontPointSize( p.toInt() );
   mpEditor->viewport()->setFocus();
}


/**
 * whenever the font of the currently edited text within the editor
 * is changed -> the toolbuttons are adjusted
 */
// -------------------------------------------------------------------------------
void MainWindow::textFontChanged(const QFont &f)
// -------------------------------------------------------------------------------
{
  pComboFont->lineEdit()->setText( f.family() );
  pComboSize->lineEdit()->setText( QString::number( f.pointSize() ) );
  textBoldTool->setChecked( f.bold() );
  textItalicTool->setChecked( f.italic() );
  textUnderTool->setChecked( f.underline() );
}

// -------------------------------------------------------------------------------
void MainWindow::textColorChanged(const QColor &c)
// -------------------------------------------------------------------------------
{
  QPixmap pix( getIcon("text_color") );

  QPainter p;
  p.begin(&pix);
  p.fillRect(1,13, 16,4, QColor(c));
  p.end();
  textColorTool->setIcon(pix);
}

// -------------------------------------------------------------------------------
void MainWindow::textAlignmentChanged(int a)
// -------------------------------------------------------------------------------
{
  //std::cout<<"alignment changed to "<<a<<std::endl;
  textLeftTool->setChecked(false);
  textCenterTool->setChecked(false);
  textRightTool->setChecked(false);
  textBlockTool->setChecked(false);

  switch (a){
  case Qt::AlignHCenter:
    textCenterTool->setChecked(true);
    //std::cout<<"center"<<std::endl;
    break;
  case Qt::AlignRight:
    textRightTool->setChecked(true);
    //std::cout<<"right"<<std::endl;
    break;
  case Qt::AlignJustify:
    textBlockTool->setChecked(true);
    //std::cout<<"just"<<std::endl;
    break;
  case Qt::AlignLeft:
  default:
    textLeftTool->setChecked(true);
    //std::cout<<"left"<<std::endl;
    break;
  }
}

// -------------------------------------------------------------------------------
void MainWindow::textBold()
{
   if ( nullptr == mpEditor )
      return;
   bool nowBold = (mpEditor->fontWeight() == QFont::Bold);
   mpEditor->setFontWeight(nowBold ? QFont::Normal : QFont::Bold);
}


void MainWindow::textItalic()
{
   if ( nullptr == mpEditor )
      return;
   mpEditor->setFontItalic( !mpEditor->fontItalic() );
}


void MainWindow::textUnder()
{
   if ( nullptr == mpEditor )
      return;
   mpEditor->setFontUnderline( !mpEditor->fontUnderline() );
}


// -------------------------------------------------------------------------------
void MainWindow::textColor()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpEditor )
      return;

   QColor c = QColorDialog::getColor(mpEditor->textColor(), this);
   if ( !c.isValid() )
      return;
   mpEditor->setTextColor( c );
   textColorChanged( c );
}


// -------------------------------------------------------------------------------
void MainWindow::textLeft()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpEditor )
      return;

   mpEditor->setAlignment(Qt::AlignLeft);
}
// -------------------------------------------------------------------------------
void MainWindow::textHCenter()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpEditor )
      return;
   mpEditor->setAlignment(Qt::AlignHCenter);
}
// -------------------------------------------------------------------------------
void MainWindow::textRight()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpEditor )
      return;

   mpEditor->setAlignment(Qt::AlignRight);
}
// -------------------------------------------------------------------------------
void MainWindow::textBlock()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpEditor )
      return;

   mpEditor->setAlignment(Qt::AlignJustify);
}



/**
 * checks whether tuxcards runs for the first time with this version
 * if yes -> write new features in file
 */
 // -------------------------------------------------------------------------------
void MainWindow::checkFirstTime()
// -------------------------------------------------------------------------------
{
  QString configurationFileName = QDir::homePath() + "/.tuxcards";

  ConfigParser parser( configurationFileName, false );
  parser.setGroup("General");
  QString version   = parser.readEntry("Version",   "previousVersion");
  // TODO: Check whether this version is correct and does work
  if( version != "TuxCardsV2.0" )
  {
    // write datafile
    QFile file( QDir::homePath() + "/tuxcards_greeting" );
    QTextStream* pLog = nullptr;

    if( !file.open(QIODevice::WriteOnly) )
    {
      std::cerr<<"TuxCards - cannot write to "<<configurationFileName.toStdString()<<"\n";
    }
    else
    {
      pLog = new QTextStream(&file);
      pLog->setEncoding(QStringConverter::Utf8);
    }

    *pLog<<sGreetingsText;

    file.close();
    DELETE( pLog );
  }
}



// -------------------------------------------------------------------------------
void MainWindow::timerEvent(QTimerEvent*)
// -------------------------------------------------------------------------------
{
  //if (AUTOSAVE)
  save();
}



/**
 * show messages via the statusbar
 */
 // -------------------------------------------------------------------------------
void MainWindow::showMessage(QString s, int seconds)
// -------------------------------------------------------------------------------
{
   if ( nullptr != mpStatusBar )
      mpStatusBar->showMessage(s, seconds*1000);
}



/**
 * to keep track of changes, put a marker in the statusbar,
 * and update the global variable 'CHANGES' to prevent unnecessary savings
 */
// -------------------------------------------------------------------------------
void MainWindow::recognizeChanges()
// -------------------------------------------------------------------------------
{
  CHANGES=true;
  statusBar_ChangeLabel->setText("*");
}


// respond to menu-calls ----------------------------------------------

// -------------------------------------------------------------------------------
void MainWindow::newFile()
{
	if (PromptBeforeNewFile() != QMessageBox::Yes)
    	return;

	// New files are unencrypted by default.
	mfileEncryptFile->setChecked(false);

	clearAll();
}

// -------------------------------------------------------------------------------
void MainWindow::clearAll()
{
	StringCrypter::clearKeyCache();

	if (mpCollection)
		deleteCollection( mpCollection );
	mpCollection = CInformationCollection::createDefaultCollection();
	initializingCollection("");
}


// -------------------------------------------------------------------------------
int MainWindow::PromptBeforeNewFile()
// -------------------------------------------------------------------------------
{
  int result = QMessageBox::No;

  if ( CHANGES ) {
 		result = askForSaving(tr("Would you like to save the current file?"));
  } else {
    result = QMessageBox::warning( this, tr("New File"), tr("Create New File, closing current one?"),
                                   QMessageBox::Yes | QMessageBox::Default,
                                   QMessageBox::No | QMessageBox::Escape);
  }

  return result;
}

// -------------------------------------------------------------------------------
int MainWindow::askForSaving(QString question)
// -------------------------------------------------------------------------------
{
  int result = QMessageBox::Yes;
  if ( CHANGES ){
    result = QMessageBox::warning( this, tr("Save"), question,
                                   QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
                                   QMessageBox::Cancel | QMessageBox::Escape);
    if (result==QMessageBox::Yes)
      save();
    else  // Over-write no as we did not save
        if (result==QMessageBox::No)
            result = QMessageBox::Yes;
  }

  return result;
}



/**
 * Before calling this method the informationCollection 'mpCollection'
 * must be valid !!!
 */
// -------------------------------------------------------------------------------
bool MainWindow::initializingCollection( QString collectionName )
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpCollection) || (nullptr == mpSingleEntryView) ) {
   		std::cout<<"MainWindow::initializingCollection returning due to null"<<std::endl;
      return false;
   }

   checkPointer( mpTree );

	if (mpCollection->isEncrypted()) {
		// Before prompting for the password, make sure this build has
		// the backend that the file was encrypted with. Otherwise we
		// would loop on "wrong password" forever — the password is
		// fine, we just can't run the cipher.
		QByteArray firstBlob;
		if (mpCollection->firstEncryptedBlob(firstBlob)) {
			const int fmt = StringCrypter::identifyBlobFormat(firstBlob);
			if (!StringCrypter::isBackendAvailableFor(fmt)) {
				QString algo;
				switch (fmt) {
				case StringCrypter::BLOB_AESGCM:
					algo = tr("AES-256-GCM (OpenSSL backend)");
					break;
				case StringCrypter::BLOB_XCHACHA:
					algo = tr("XChaCha20-Poly1305 (monocypher backend)");
					break;
				default:
					algo = tr("an unknown crypto backend");
					break;
				}
				QMessageBox::critical(this,
					tr("Unsupported encryption format"),
					tr("This file is encrypted with %1, but this build "
					   "of TuxCards was compiled without that backend.\n\n"
					   "Please download the full build (with both crypto "
					   "backends) from the project releases page, or "
					   "rebuild TuxCards with the matching backend "
					   "enabled.").arg(algo));
				return false;
			}
		}

		bool bCorrectPasswd = false;
        int wrongPassCount=0;
        const int WRONG_PASS_MAX_COUNT=3;

		mfileEncryptFile->setChecked(true);
		// Continue to ask for valid password, on cancel, open an empty collection
		do {
			// Ask for password here.
		   CFileEncryptionPasswordDialog fileEncryptionPasswordDialog( this );
		   fileEncryptionPasswordDialog.setUp(collectionName);

	   		// If cancel are pressed, do not use this collection.
	   		if (fileEncryptionPasswordDialog.getPasswd().isEmpty()) {
	   			// mpTree will be deleted when the empty collection is loaded.
	   			return false;
	   		}

		   {
		      const bool bLazy = CTuxCardsConfiguration::getInstance()
		                            .getBoolValue(CTuxCardsConfiguration::B_LAZY_DECRYPT);
		      bCorrectPasswd = mpCollection->decryptTree(
		            fileEncryptionPasswordDialog.getPasswd(), bLazy);
		   }
           if (!bCorrectPasswd) {
               wrongPassCount++;
               if (wrongPassCount >= WRONG_PASS_MAX_COUNT) {
                   QMessageBox::warning(this, tr("TuxCards"), tr("Too many password attempts"),
                         QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
                   return false;
               }

           }

		} while (!bCorrectPasswd);

		mstatusBar_EncryptedLabel->setPixmap(getIcon("unlocksm"));

	} else {
		mfileEncryptFile->setChecked(false);
		mstatusBar_EncryptedLabel->clear();
	}

   mpTree->createTreeFromCollection( *mpCollection );

   connect( mpCollection, &CInformationCollection::activeInformationElementChanged,
            mpSingleEntryView, &CSingleEntryView::activeInformationElementChanged );
   connect( mpCollection, &CInformationCollection::activeInformationElementChanged,
            this,              &MainWindow::activeInformationElementChanged );
   connect( mpCollection, &CInformationCollection::modelHasChanged,       this, &MainWindow::recognizeChanges );
   connect( mpCollection, &CInformationCollection::numElementsChanged,    this, &MainWindow::updateStatusbarElements );

   if ( nullptr != mpEditor )
      mpEditor->clear();

   // collection successfully created and system set up with it
   // !!! if using a windows-system: this might not work since '/' not in path
   int i=collectionName.lastIndexOf('/');
   if (i>-1)
      mpTree->setColumnText(collectionName.mid(i+1));
   else
      mpTree->setColumnText(collectionName);

	updateStatusbarElements(mpCollection->numElements());

   selectLastActiveElement();

   mpCollection->addView( &mHistory );
   mpCollection->addView( mpTree );
   mpCollection->addView( mpSingleEntryView );

   setWindowTitle(tr("TuxCards (%1)").arg(collectionName));

   mConfiguration.setStringValue( CTuxCardsConfiguration::S_DATA_FILE_NAME, collectionName );
   mConfiguration.saveToFile();                    // because the dataFileName has changed

   statusBar_ChangeLabel->setText(" ");
   CHANGES=false;
   return true;
}

// -------------------------------------------------------------------------------
void MainWindow::deleteCollection(CInformationCollection* collection)
{
	if (!collection)
		return;

	disconnect( collection, &CInformationCollection::numElementsChanged, this, &MainWindow::updateStatusbarElements );
	DELETE( collection );
}


// -------------------------------------------------------------------------------
CInformationElement* MainWindow::getActiveIE()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpCollection )
      return nullptr;

   return mpCollection->getActiveElement();
}




/**
 * adapter for 'open(QString h)'
 */
// -------------------------------------------------------------------------------
void MainWindow::open()
// -------------------------------------------------------------------------------
{
  // give a chance to save the file before opening another one
  if (askForSaving(tr("Do you want to save the current file before opening another?")) == QMessageBox::Cancel)
    return;

  // getting dataFileName
  QString fileName( QFileDialog::getOpenFileName() );
  if ( fileName.isNull() || fileName=="" )
    showMessage(tr("No Filename specified."), 5);
  else
    open(fileName);
}



/**
 * Returns true, if file was opened successfully; otherwise false.
 */
// -------------------------------------------------------------------------------
bool MainWindow::open( QString fileName )
// -------------------------------------------------------------------------------
{
   // Drop any cached AES-GCM derived key from a previously open file —
   // the new file may use a different password, and we should not keep
   // the previous file's key material in memory across files.
   StringCrypter::clearKeyCache();

   bool retVal = false;

   int format = getDataFileFormat(fileName);

   if ( format == 2 )
      retVal = openXMLDataFile(fileName);
   else if ( format == 1 )
      retVal = openOldDataFile(fileName);
   else
   {
      QMessageBox::critical(	this, tr("Opening a data file"),
            tr("Could not open file '%1'<br> or did not "
            "recognize the dataformat.").arg(fileName),
            tr("Ok"));
   }

   if (retVal)
   {
      if ( nullptr != mpRecentFiles )
         mpRecentFiles->setOnTop(fileName);
   } else {
		// Can fail for encrypted files (incorrect password.) or bad xml data.
		// Open an empty new file in that case.
		clearAll();
   }

   return retVal;
}



/**
 * Opens a file and detects the fileformat.
 *     i.e. '2' == XML-File (the new TuxCards format)
 *          '1' == Old-File (the old TuxCards format, that was used since version 0.5)
 *          '0' == unknown format
 */
// -------------------------------------------------------------------------------
int MainWindow::getDataFileFormat(QString fileName)
// -------------------------------------------------------------------------------
{
  QFile file(fileName);
  if ( !file.open(QIODevice::ReadOnly) ) {
    //showMessage("ERROR could not open '"+fileName+"' for reading.", 5);
    return 0;
  }


  QTextStream t( &file );
  QString line=t.readLine();
  //cout<<"readLine="<<line<<endl;
  file.close();

  if (line.startsWith("TuxCardsV0.5"))
    return 1;
  else if (line.startsWith("<?xml"))
    return 2;

  return 0;
}



/**
 * Opens a file given by a valid name ('fileName'), creates an
 * informationcollection from it & sets latter one to be presented
 * within tuxcards.
 *
 * Returns true, if file was opend successfully; otherwise false.
 */
// -------------------------------------------------------------------------------
bool MainWindow::openOldDataFile(QString fileName)
// -------------------------------------------------------------------------------
{
  QFile file(fileName);
  bool retval;

  if (! file.open(QIODevice::ReadOnly) ) {
    showMessage(tr("ERROR could not open '%1' for reading.").arg(fileName), 5);
    return false;
  }

  // create absolute file name, in case a relative one is given
  fileName = QFileInfo(fileName).absoluteFilePath();


  QString s="";
  QTextStream t( &file );               // use a text stream
  while ( !t.atEnd() ) {
    s += QChar((char)10) + t.readLine();// the first chr(10) is wrong, but doesn't matter
  }
  file.close();
  s=s.mid(1);                           // remove wrong chr(10) from beginning

  deleteCollection( mpCollection);
  mpCollection = Persister::createInformationCollection( s );
  retval = initializingCollection( fileName );
  // On failure, mpCollection will be deleted when the empty collection is loaded.

  return retval;
}



/**
 * Opens a file given by a valid name ('fileName'), creates an
 * informationcollection from it & sets latter one to be presented
 * within tuxcards.
 *
 * Returns true, if file was opend successfully; otherwise false.
 */
// -------------------------------------------------------------------------------
bool MainWindow::openXMLDataFile(QString fileName)
// -------------------------------------------------------------------------------
{
  bool retval;

  // create absolute file name, in case a relative one is given
  fileName = QFileInfo(fileName).absoluteFilePath();

  deleteCollection( mpCollection );

  QFile file(fileName);
  mpCollection = XMLPersister::createInformationCollection( file );

  if ( !mpCollection )
  {
    QMessageBox::warning(this, tr("TuxCards - XML I/O"),
                         tr("ERROR could not open '%1' for reading or parse error.").arg(fileName),
                         QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
    return false;
  }

  retval = initializingCollection( fileName );
  // On failure, mpCollection will be deleted when the empty collection is loaded.

  return retval;
}

// When opening a new file, if current file is dirty, ask for it to be saved.
// -------------------------------------------------------------------------------
void MainWindow::slotSaveAndLoadNewFile(QString newFile)
{
	if (askForSaving(tr("Would you like to save the current file?")) == QMessageBox::Cancel)
    	return;

	open(newFile);

}


/**
 * Reads the last active element from the configuration file and selects it.
 * Also, the position of the vertical scrollbar within the tree is restored.
 */
// -------------------------------------------------------------------------------
void MainWindow::selectLastActiveElement()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpCollection )
      return;


   //Path path(mConfiguration.getStringValue( CTuxCardsConfiguration::S_LAST_ACTIVE_ELEM_PATH ));
   Path path( XMLPersister::getPathOfLastActiveElement() );
   //std::cout<<"Path = "<<path.toString()<<std::endl;

   if ( mpCollection->isPathValid(path) )
   {
      //std::cout<<"Path is valid"<<std::endl;
      mpCollection->setActiveElement( path );
   } else {
      //std::cout<<"Path is invalid"<<std::endl;
      mpCollection->setActiveElement( mpCollection->getRootElement() );
   }

   // Defer until the tree has laid out its items — calling setValue before the
   // scrollbar range is known clamps to 0 and the saved position is lost.
   const int scrollPos =
      mConfiguration.getIntValue( CTuxCardsConfiguration::I_TREE_VSCROLLBAR_VALUE );
   QTimer::singleShot(0, mpTree, [this, scrollPos]() {
      if ( mpTree )
         mpTree->verticalScrollBar()->setValue(scrollPos);
   });
}


// -------------------------------------------------------------------------------
void MainWindow::save()
// -------------------------------------------------------------------------------
{
  if (mConfiguration.getStringValue( CTuxCardsConfiguration::S_DATA_FILE_NAME ) == "")
  {
    saveAs();
  }
  else
  {
    save(mConfiguration.getStringValue( CTuxCardsConfiguration::S_DATA_FILE_NAME ));
  }
}


// -------------------------------------------------------------------------------
void MainWindow::saveAs()
// -------------------------------------------------------------------------------
{
   QString newFileName( QFileDialog::getSaveFileName() );
   if ( newFileName.isNull() || newFileName=="" )
   {
      showMessage(tr("No Filename specified."), 5);
      return;
   }

   save(newFileName);

   if ( nullptr != mpRecentFiles )
   {
      mpRecentFiles->setOnTop(newFileName);
   }

   mConfiguration.setStringValue( CTuxCardsConfiguration::S_DATA_FILE_NAME, newFileName );
   mConfiguration.saveToFile();
}


// -------------------------------------------------------------------------------
void MainWindow::save(QString fileName)
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpCollection) || (nullptr == mpEditor) )
      return;

   if ( (QDir::homePath() + TUX_CONFIG_FILE) == fileName )
   {
      QMessageBox::warning( this, tr("Saving"), tr("File not saved.\n"
                            "Please do not use \"%1\""
                            " as file name.").arg(fileName),
                            QMessageBox::Abort, NULL );
      return;
   }

   // before saving -> move current file i.e. "myfile.data" to "myfile.data~"
   if ( mConfiguration.getBoolValue( CTuxCardsConfiguration::B_CREATE_BACKUP_FILE ) )
   {
      QDir tmp;
      QString sFileName = mConfiguration.getStringValue( CTuxCardsConfiguration::S_DATA_FILE_NAME );
      tmp.rename( sFileName, sFileName + "~" );
   }

   // saving eventual changes
   mpEditor->writeCurrentTextToActiveInformationElement();

   // Push the user's "Encryption format" / "re-encrypt on change" choice
   // into the serializer before it walks the tree. Tree elements decide
   // per-element whether to reuse their on-disk blob or re-encrypt with
   // the current backend.
   const QString fmtStr = mConfiguration.getStringValue(
                             CTuxCardsConfiguration::S_ENCRYPTION_FORMAT );
   if ( fmtStr == "monocypher" )
      StringCrypter::setWriteBackend( StringCrypter::BLOB_XCHACHA );
   else
      StringCrypter::setWriteBackend( StringCrypter::BLOB_AESGCM );
   CInformationElement::sReencryptOnFormatChange = mConfiguration.getBoolValue(
                             CTuxCardsConfiguration::B_REENCRYPT_ON_FORMAT_CHANGE );

   XMLPersister::save( *mpCollection, fileName );


   int i=fileName.lastIndexOf('/');
   if (i>-1)
      mpTree->setColumnText(fileName.mid(i+1));
   else
      mpTree->setColumnText(fileName);

   setWindowTitle(tr("TuxCards (%1)").arg(fileName));

   mConfiguration.setStringValue( CTuxCardsConfiguration::S_DATA_FILE_NAME, fileName );
   mConfiguration.saveToFile();

   statusBar_ChangeLabel->setText(" ");
   CHANGES=false;
   mLastSaveEpochMs = QDateTime::currentMSecsSinceEpoch();
   refreshSaveIndicator();
   showMessage(tr("Saved to '%1'.").arg(fileName), 5);

   callingExecutionStatement();
}


// -------------------------------------------------------------------------------
void MainWindow::callingExecutionStatement()
// -------------------------------------------------------------------------------
{
	QString execStatement = mConfiguration.getStringValue( CTuxCardsConfiguration::S_EXECUTE_STATEMENT );
	if (execStatement.size() > 0)
	{
		const int rc = std::system( execStatement.toLatin1().constData() );
		(void) rc;
	}
}


// Toggles encrption for current file
// -------------------------------------------------------------------------------
void MainWindow::toggleFileEncryption()
{
	QString strPassword("");

	if ( nullptr == mpCollection )
		return;

	// Check current state of encryption before changing it
	if (mpCollection->isEncrypted()) {

		// Confirm decryption
		if (QMessageBox::Cancel == QMessageBox::warning(this, tr("Tuxcards File Encryption"),
													tr("About to remove encryption for current"
                                                   " file.\n Would you like to continue?"),
                                                   QMessageBox::Ok,
                                                   QMessageBox::Cancel,
                                                   QMessageBox::NoButton))
	   {
            // Reset toggle to on.
			mfileEncryptFile->setChecked(true);
    	  	return;
	   }

		mfileEncryptFile->setChecked(false);
		mstatusBar_EncryptedLabel->clear();
	}
	else {
		// Confirm with user before going ahead
	   int iUseEncryption = mConfiguration.askForUsingEncryption();
		// User decided not to go ahead
		if(!iUseEncryption) {
	    	mfileEncryptFile->setChecked(false);
			return;
		}

		// Request user for password to use for encryption
		mPasswdDialog.setUp( tr("Current file"));
		strPassword = mPasswdDialog.getPasswd();
	    if (strPassword.isEmpty() ) {
	    	mfileEncryptFile->setChecked(false);
	    	return;
	    }

		mstatusBar_EncryptedLabel->setPixmap(getIcon("unlocksm"));
	}

	mpCollection->toggleEncryption(strPassword);
	recognizeChanges();

}

void MainWindow::exportEntryMarkdown()
{
   if ( !mpCollection || !mpEditor || !mpCollection->getActiveElement() )
      return;
   QString fn = QFileDialog::getSaveFileName(this, tr("Export entry as Markdown"),
                  QString(),
                  tr("Markdown (*.md);;All files (*)"));
   if ( fn.isEmpty() ) return;

   mpEditor->writeCurrentTextToActiveInformationElement();

   // MARKDOWN — pass through (mInformation is the raw .md source).
   // HTML — let Qt down-convert via toMarkdown().
   // TEXT — write the plain text as-is.
   CInformationElement* elem = mpCollection->getActiveElement();
   QString md;
   if ( elem->getInformationFormat() == &InformationFormat::MARKDOWN )
      md = elem->getInformation();
   else if ( elem->getInformationFormat() == &InformationFormat::HTML )
      md = mpEditor->document()->toMarkdown();
   else
      md = elem->getInformation();

   QFile f(fn);
   if ( !f.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
      QMessageBox::warning(this, tr("Export"), tr("Could not open %1 for writing.").arg(fn));
      return;
   }
   f.write(md.toUtf8());
   f.close();
   showMessage(tr("Exported to '%1'.").arg(fn), 5);
}


void MainWindow::importEntryMarkdown()
{
   if ( !mpCollection || !mpEditor || !mpCollection->getActiveElement() )
      return;
   QString fn = QFileDialog::getOpenFileName(this, tr("Import Markdown into current entry"),
                  QString(),
                  tr("Markdown (*.md *.markdown);;All files (*)"));
   if ( fn.isEmpty() ) return;

   QFile f(fn);
   if ( !f.open(QIODevice::ReadOnly) ) {
      QMessageBox::warning(this, tr("Import"), tr("Could not open %1 for reading.").arg(fn));
      return;
   }
   QString md = QString::fromUtf8(f.readAll());
   f.close();

   // Load the raw .md into the *currently active* entry. We must NOT
   // route through Editor::activeInformationElementChanged here: that
   // path first writes the editor's existing (pre-import) buffer back
   // into the element, which would clobber the markdown we are about to
   // store. Instead set the format, flip the editor into plain-text
   // mode, push the md text in, then persist editor → element.
   CInformationElement* elem = mpCollection->getActiveElement();
   elem->setInformationFormat( &InformationFormat::MARKDOWN );
   mpEditor->setAcceptRichText( false );      // before setText, so it stores plain
   mpEditor->setText( md );                   // editor now shows the raw markdown
   mpEditor->writeCurrentTextToActiveInformationElement();  // md → element
   // Refresh the format toolbar (disable rich-text tools, reveal the
   // markdown helpers / preview toggle for the now-MARKDOWN entry).
   showRecognizedFormat( InformationFormat::MARKDOWN );
   recognizeChanges();
   showMessage(tr("Imported from '%1'.").arg(fn), 5);
}


/**
 * opens the current file from disk and exports it to
 */
// -------------------------------------------------------------------------------
void MainWindow::exportHTML()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpCollection )
      return;

   QString dirPath = QFileDialog::getExistingDirectory(
                               this,
                               tr("Choose a directory"),
                               QDir::homePath() );

   if ( dirPath.isEmpty() )
      return;

   bool bSuccess = HTMLWriter::writeCollectionToHTMLFile( *mpCollection, dirPath );

   // done
   if ( false != bSuccess )
   {
      QMessageBox::information( this, tr("HTML-Export"), tr("HTML<em>Export</em> "
                                "<font size=-1>(%1)</font>"
                                " finished, successfully.\n\n"
                                "The data are stored in\n%2").arg(QString(TUX_VERSION), dirPath), 1);
   }
   else
   {
      QMessageBox::warning( this, tr("HTML-Export"), tr("HTML<em>Export</em> "
                            "<font size=-1>(%1)</font>"
                            " not successfully.\n\n"
                            "Please check write permission and disk space\n"
                            "(%2)").arg(QString(TUX_VERSION), dirPath), tr("Abort"));
   }
}


// -------------------------------------------------------------------------------
void MainWindow::keyPressEvent(QKeyEvent* k)
// -------------------------------------------------------------------------------
{
   if ( !k || !mpTree || !mpSingleEntryView )
      return;

   switch( k->modifiers() )
   {
   case Qt::ControlModifier:
      // Ctrl+F / Ctrl+H / Ctrl+Shift+F are wired as QAction shortcuts in the
      // Edit menu (settingUpMenu) and reach their slots via Qt's shortcut
      // system, so this dispatch only needs the keys that don't have actions.
      if (k->key() == Qt::Key_S)
         save();
      break;

   case Qt::AltModifier:
      if ( k->key() == Qt::Key_Left )
         activatePreviousHistoryElement();
      else if ( k->key() == Qt::Key_Right )
         activateNextHistoryElement();
      break;

   default:
      if ( Qt::Key_F5 == k->key() )
      {
         if ( mpEditor->hasFocus() )
            mpTree->setFocus();
         else
            mpEditor->setFocus();
      }
      else
         k->ignore();
      break;
  }
}


// -------------------------------------------------------------------------------
void MainWindow::exit()
// -------------------------------------------------------------------------------
{
  close();                      // calls 'closeEvent(..)' indirectly
}


// -------------------------------------------------------------------------------
void MainWindow::wordCount( void )
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpCollection) || (nullptr == mpEditor) )
      return;

   mpEditor->writeCurrentTextToActiveInformationElement();
   CInformationElement* pActiveElement = mpCollection->getActiveElement();
   if ( nullptr == pActiveElement )
   {
      QMessageBox::information( 0, tr("WordCount"), tr("There is no active entry."),
                                QMessageBox::Abort );
      return;
   }

   QString text = pActiveElement->getInformationText();
   int numchar  = text.length();
   int words  = Strings::wordCount( text );
   int lines  = mpEditor->document()->blockCount();
   int parags = mpEditor->document()->blockCount();

   QMessageBox::information( this, tr("TuxCards"),
                    tr("<center>Current Entry contains<br><br>"
                       "%1 characters<br>%2 words<br>%3 lines<br>%4 paragraphs.</center>")
                       .arg(numchar).arg(words).arg(lines).arg(parags));
}

// -------------------------------------------------------------------------------
void MainWindow::insertCurrentDate( void )
// -------------------------------------------------------------------------------
{
   if ( nullptr != mpEditor )
      mpEditor->insertPlainText( QDate::currentDate().toString() );
}

// -------------------------------------------------------------------------------
void MainWindow::insertCurrentTime()
// -------------------------------------------------------------------------------
{
   if ( nullptr != mpEditor )
      mpEditor->insertPlainText( QTime::currentTime().toString() );
}

// -------------------------------------------------------------------------------
void MainWindow::showKBShortcuts()
// -------------------------------------------------------------------------------
{
   QMessageBox::about(  this, tr("TuxCards"),
                    tr("TuxCards Keyboard shortcuts\n\n"

					"Common shortcuts:\n"
					"Ctrl+N: New File\n"
					"Ctrl+O: Open File\n"
					"Ctrl+S: Save current file\n"
					"Ctrl+E: Encrypt current file\n"
					"Ctrl+F: Search across the whole tree\n"
					"F3: Find in current entry (press again for next match, Shift+F3 for previous)\n"
					"Ctrl+H: Find and replace in current entry\n"
					"F5: Switch between tree(left pane) and editor window(right pane)\n"
					"Alt+Left or Right arrow: Navigate items accessed earlier(history)\n"
					"MENU (Left of right Ctrl key): Show current context menu\n"
					"Ctrl+W: Word count, Ctrl+D: Date insertion, Ctrl+T: Time insertion\n\n"

					"Tree (Left pane) shortcuts:\n"
					"F2: Edit current entry title\n"
					"INS: Add a new entry\n"
					"DEL: Delete current entry\n\n"

					"Editor (Right pane) shortcuts:\n"
					"Ctrl+X: Cut, Ctrl+C: Copy, Ctrl+V: Paste\n"
					"Ctrl+Z: Undo, Ctrl+Y: Redo\n"
					"Ctrl+K: Delete till end of current line\n"
					"Ctrl+B: Bold, Ctrl+I: Italic, Ctrl+U: Underline, Ctrl+A: Select all\n")
                    );
}

// -------------------------------------------------------------------------------
void MainWindow::showAbout()
// -------------------------------------------------------------------------------
{
   QMessageBox::about(  this, tr("TuxCards"),
                    tr("TuxCards - The Notebook for Linux\n"
                    "%1\n\n"
                    "Copyright (c) 2006-2009 Amit Chaudhary\n"
                    "amitch@rajgad.com\n"
                    "Copyright (c) 2007 Yahoo! Inc.\n"
                    "Copyright (c) 2000-2004 Alexander Theel\n"
                    "alex.theel@gmx.net\n\n"
                    "Qt6 port (2026-05-16) by Claude Opus 4.7\n"
                    "at the initiative of gzivdo (https://github.com/gzivdo)\n"
                    "SideBar and PNG icons backported from TuxCards 2.2.1.\n").arg(QString(TUX_VERSION)));
}

/**
 * saves the data automatically by closing/quitting the program
 */
// -------------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *e)
// -------------------------------------------------------------------------------
{
   // accept signal
   e->accept();                  // default implementation of this method


   // we always save the options; the splitter-size may have changed
   if ( nullptr != mpCollection )
   {
      Path path( mpCollection->getActiveElement() );
      mConfiguration.setStringValue( CTuxCardsConfiguration::S_LAST_ACTIVE_ELEM_PATH, path.toString() );
   }
   mConfiguration.setIntValue( CTuxCardsConfiguration::I_TREE_VSCROLLBAR_VALUE, mpTree->verticalScrollBar()->value());

   mConfiguration.setIntValue( CTuxCardsConfiguration::I_WINDOW_WIDTH, width() );
   mConfiguration.setIntValue( CTuxCardsConfiguration::I_WINDOW_HEIGHT, height() );
   mConfiguration.setIntValue( CTuxCardsConfiguration::I_WINDOW_X, x() );
   mConfiguration.setIntValue( CTuxCardsConfiguration::I_WINDOW_Y, y() );
   mConfiguration.setIntValue( CTuxCardsConfiguration::I_TREE_WIDTH,  mpSplit->sizes().first() );
   mConfiguration.setIntValue( CTuxCardsConfiguration::I_EDITOR_WIDTH, mpSplit->sizes().last() );

   if ( nullptr != mpRecentFiles )
   {
      mConfiguration.setStringValue( CTuxCardsConfiguration::S_RECENT_FILES, mpRecentFiles->toString() );
   }
   mConfiguration.saveToFile();


   if (!CHANGES)
      return;

   if ( mConfiguration.getBoolValue( CTuxCardsConfiguration::B_SAVE_WHEN_LEAVING )
       || (QMessageBox::warning(  this, tr("Save before exiting."),
                                  tr("Do you want to save before leaving TuxCards?"),
                                  tr("Yes"), tr("No")) == 0) )
   {
      showDialog->show();
      save();
      showDialog->hide();
   }

   StringCrypter::clearKeyCache();
}


void MainWindow::setMainToolbarVisible( bool bVisible )
{
   mConfiguration.setBoolValue( CTuxCardsConfiguration::B_SHOW_MAIN_TOOLBAR, bVisible );

   if ( mpMainTools )
   {
      if (bVisible) mpMainTools->show();
      else mpMainTools->hide();
   }
   if (mbStartupDone)
      mConfiguration.saveToFile();
}


void MainWindow::setEntryToolbarVisible( bool bVisible )
{
   mConfiguration.setBoolValue( CTuxCardsConfiguration::B_SHOW_ENTRY_TOOLBAR, bVisible );

   if ( mpEntryTools )
   {
      if (bVisible) mpEntryTools->show();
      else mpEntryTools->hide();
   }
   if (mbStartupDone)
      mConfiguration.saveToFile();
}


void MainWindow::setEditorToolbarVisible( bool bVisible )
{
   mConfiguration.setBoolValue( CTuxCardsConfiguration::B_SHOW_EDITOR_TOOLBAR, bVisible );

   if ( mpEditorTools )
   {
      if (bVisible) mpEditorTools->show();
      else mpEditorTools->hide();
   }
   if (mbStartupDone)
      mConfiguration.saveToFile();
}

// -------------------------------------------------------------------------------
void MainWindow::editConfiguration( void )
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpOptionsDialog )
      return;

   mpOptionsDialog->setUp();
}


// -------------------------------------------------------------------------------
void MainWindow::applyConfigurationMain()
// -------------------------------------------------------------------------------
{
	applyConfiguration();
	startAutosaveTimer();
}

// -------------------------------------------------------------------------------
void MainWindow::applyConfiguration()
// -------------------------------------------------------------------------------
{
   // editor
   if ( nullptr != mpEditor )
   {
      QFont font = mConfiguration.getASCIIEditorFont().toFont();
      mpEditor->setFont(font);
      mpEditor->setTabStopDistance(qreal( mConfiguration.getIntValue( CTuxCardsConfiguration::I_TAB_SIZE ) * QFontMetrics(font).horizontalAdvance('X') ));
      mpEditor->setWordWrap( mConfiguration.getIntValue( CTuxCardsConfiguration::I_WORD_WRAP ) );
   }

   // tree
   mpTree->setFont( mConfiguration.getTreeFont().toFont() );

   // colorbar / sidebar
   if ( mpColorBar )
   {
      QString t1 = mConfiguration.getBoolValue( CTuxCardsConfiguration::B_IS_HTEXT_ENABLED )
                      ? mConfiguration.getStringValue( CTuxCardsConfiguration::S_TEXT_ONE ) : "";
      QString t2 = mConfiguration.getBoolValue( CTuxCardsConfiguration::B_IS_HTEXT_ENABLED )
                      ? mConfiguration.getStringValue( CTuxCardsConfiguration::S_TEXT_TWO ) : "";
      mpColorBar->change( mConfiguration.getTopColor(),
                          mConfiguration.getBottomColor(),
                          t1, t2,
                          mConfiguration.getFontColor() );
      QString vt = mConfiguration.getBoolValue( CTuxCardsConfiguration::B_IS_VTEXT_ENABLED )
                      ? mConfiguration.getStringValue( CTuxCardsConfiguration::S_VERTICAL_TEXT ) : "";
      mpColorBar->setVerticalText( vt,
                                   mConfiguration.getBoolValue( CTuxCardsConfiguration::B_ALIGN_VTEXT ) );
      mpColorBar->update();
   }

   // windowsize & splitter
   setWindowGeometry( mConfiguration.getIntValue( CTuxCardsConfiguration::I_WINDOW_WIDTH ),
                      mConfiguration.getIntValue( CTuxCardsConfiguration::I_WINDOW_HEIGHT ),
                      mConfiguration.getIntValue( CTuxCardsConfiguration::I_TREE_WIDTH ),
                      mConfiguration.getIntValue( CTuxCardsConfiguration::I_EDITOR_WIDTH )
                    );

   // restore window position (only if we saved one previously: -1 sentinel
   // means "leave it to the window manager")
   const int wx = mConfiguration.getIntValue( CTuxCardsConfiguration::I_WINDOW_X );
   const int wy = mConfiguration.getIntValue( CTuxCardsConfiguration::I_WINDOW_Y );
   if ( wx >= 0 && wy >= 0 )
      move( wx, wy );

   // markdown editor: split-view on/off per user preference
   if ( mpSingleEntryView )
      mpSingleEntryView->setMarkdownSplitView(
         mConfiguration.getBoolValue( CTuxCardsConfiguration::B_MARKDOWN_SPLIT_VIEW ) );
}

// -------------------------------------------------------------------------------
void MainWindow::startAutosaveTimer()
// -------------------------------------------------------------------------------
{
	 // autosave
   killTimer(TIMER_ID);
   if ( mConfiguration.getBoolValue( CTuxCardsConfiguration::B_AUTOSAVE ) )
      TIMER_ID=startTimer( 60000 * mConfiguration.getIntValue( CTuxCardsConfiguration::I_SAVE_ALL_MINUTES ) );
}


// -------------------------------------------------------------------------------
void MainWindow::setWindowGeometry( int windowWidth, int windowHeight,
                                    int treeSize,    int editorSize )
// -------------------------------------------------------------------------------
{
  resize(windowWidth, windowHeight);

  lst = new QList<int>();
  lst->append(treeSize);
  lst->append(editorSize);
  mpSplit->setSizes( *lst );
}


// -------------------------------------------------------------------------------
void MainWindow::search()
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpEditor) || (nullptr == mpTree) )
      return;

   mpEditor->writeCurrentTextToActiveInformationElement();
   mpTree->search();
}

// -------------------------------------------------------------------------------
void MainWindow::editorFind()
// -------------------------------------------------------------------------------
{
   if ( !mpSingleEntryView ) return;
   EditorFindBar* bar = mpSingleEntryView->getFindBar();
   if ( !bar ) return;
   // F3 = "open the bar" the first time, "find next" every subsequent press.
   if ( bar->isVisible() )
      bar->findNext();
   else
      bar->showFind( false );
}

// -------------------------------------------------------------------------------
void MainWindow::editorReplace()
// -------------------------------------------------------------------------------
{
   if ( !mpSingleEntryView ) return;
   if ( EditorFindBar* bar = mpSingleEntryView->getFindBar() )
      bar->showFind( true );
}

// -------------------------------------------------------------------------------
void MainWindow::print()
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpCollection) || (nullptr == mpCollection->getActiveElement()) )
      return;

   if ( mpCollection->getActiveElement()->getInformationFormat() != &InformationFormat::HTML )
   {
      QMessageBox::information( this, "Printing", "Please consider converting this note to "
                               "HTML before printing." );
   }

   if ( nullptr == mpEditor )
      return;


  mpEditor->writeCurrentTextToActiveInformationElement();
#ifndef QT_NO_PRINTER
  QPrinter printer;
  printer.setFullPage(true);
  QPrintDialog dlg(&printer, this);
  if ( dlg.exec() == QDialog::Accepted )
  {
     QTextDocument doc;
     QFont font( mConfiguration.getASCIIEditorFont().toFont() );
     font.setPointSize(10);
     doc.setDefaultFont(font);
     doc.setHtml( mpCollection->getActiveElement()->getInformation() );
     doc.print( &printer );
  }
#endif
}

// -------------------------------------------------------------------------------
void MainWindow::printPreview()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpCollection || nullptr == mpCollection->getActiveElement() )
      return;

   if ( mpCollection->getActiveElement()->getInformationFormat() != &InformationFormat::HTML )
   {
      QMessageBox::information( this, tr("Print preview"),
                                tr("Please consider converting this note to "
                                   "HTML before printing.") );
   }

   if ( nullptr == mpEditor )
      return;

   mpEditor->writeCurrentTextToActiveInformationElement();
#ifndef QT_NO_PRINTER
   QPrinter printer;
   printer.setFullPage(true);
   QPrintPreviewDialog dlg(&printer, this);
   dlg.setWindowTitle(tr("Print preview"));
   QFont font( mConfiguration.getASCIIEditorFont().toFont() );
   font.setPointSize(10);
   const QString html = mpCollection->getActiveElement()->getInformation();
   connect(&dlg, &QPrintPreviewDialog::paintRequested,
           this, [font, html](QPrinter* p) {
              QTextDocument doc;
              doc.setDefaultFont(font);
              doc.setHtml(html);
              doc.print(p);
           });
   dlg.exec();
#endif
}

// -------------------------------------------------------------------------------
void MainWindow::makeVisible( SearchPosition* pPosition )
// -------------------------------------------------------------------------------
{
   if ( (nullptr == mpCollection) || (nullptr == pPosition) || (nullptr == mpEditor) )
      return;

   mpCollection->setActiveElement( *(pPosition->getPath()) );

   int paragraph = pPosition->getLine();
   int pos = pPosition->getPos();
   int len = pPosition->getLen();

   this->activateWindow();
   QTextCursor tc = mpEditor->textCursor();
   QTextBlock block = mpEditor->document()->findBlockByNumber(paragraph);
   if ( block.isValid() ) {
      tc.setPosition(block.position() + pos);
      tc.setPosition(block.position() + pos + len, QTextCursor::KeepAnchor);
      mpEditor->setTextCursor(tc);
   }

   mpEditor->setFocus();
   mpEditor->ensureCursorVisible();
}

// -------------------------------------------------------------------------------
void MainWindow::moveElementUp()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpCollection )
      return;

   if ( nullptr == mpCollection->getActiveElement() )
      return;


   ((CTreeInformationElement*) mpCollection->getActiveElement())->moveOneUp();
}

// -------------------------------------------------------------------------------
void MainWindow::moveElementDown()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpCollection )
      return;

   if ( nullptr == mpCollection->getActiveElement() )
      return;


   ((CTreeInformationElement*) mpCollection->getActiveElement())->moveOneDown();
}



/*********************** debug methods **********************************/
// -------------------------------------------------------------------------------
void MainWindow::debugShowRTFTextSource()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpEditor )
      return;

   QTextEdit* outputWindow=new QTextEdit();
   outputWindow->resize(400,400);
   outputWindow->setAcceptRichText(false);
   outputWindow->setText(mpEditor->getText());
   outputWindow->show();
}


// -------------------------------------------------------------------------------
void MainWindow::debugShowXMLCode()
// -------------------------------------------------------------------------------
{
   if ( nullptr == mpCollection )
      return;

   QTextEdit* outputWindow=new QTextEdit();
   outputWindow->resize(400,400);
   outputWindow->setAcceptRichText(false);
   outputWindow->setText(mpCollection->toXML());
   outputWindow->show();
}

// -------------------------------------------------------------------------------
void MainWindow::updateStatusbarElements(int numElements)
{
	QString strNumElements = QString("Notes: %1").arg(numElements);
	mstatusBar_NumElements->setText(strNumElements);
}
