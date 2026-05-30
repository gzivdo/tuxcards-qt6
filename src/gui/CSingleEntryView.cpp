/***************************************************************************
                          CSingleEntryView.cpp  -  description
                             -------------------
    begin                : Fri Jan 09 2004
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


#include "CSingleEntryView.h"
#include "editorfindbar.h"
#include "../information/informationformat.h"
#include "../information/CInformationElement.h"
#include <QVBoxLayout>
#include <QSplitter>
#include <QTextBrowser>
#include <QTimer>
#include <iostream>

// -------------------------------------------------------------------------------
CSingleEntryView::CSingleEntryView( QWidget* pParent )
  : QWidget( pParent )
  , mpActiveElement( nullptr )
  , mpEditor( nullptr )
  , mpFindBar( nullptr )
  , mpSplitter( nullptr )
  , mpMdPreview( nullptr )
  , mpMdPreviewDebounce( nullptr )
  , mbSplitEnabled( false )
  , mbPreviewOnly( false )
// -------------------------------------------------------------------------------
{
   mpEditor = new Editor( this );

   if ( (nullptr == mpEditor) )
   {
      std::cout<<"Constructor 'CSingleEntryView': ERROR not enough memory "
               <<" to create objects!!!"<<std::endl;
      return;
   }

   // A read-only rendered-markdown view that lives next to the editor in
   // a splitter. Its presence/visibility (not the editor's document)
   // implements preview, so the editor's undo history is never disturbed.
   mpMdPreview = new QTextBrowser( this );
   mpMdPreview->setOpenExternalLinks( true );
   mpMdPreview->hide();

   mpSplitter = new QSplitter( Qt::Horizontal, this );
   mpSplitter->addWidget( mpEditor );
   mpSplitter->addWidget( mpMdPreview );
   mpSplitter->setStretchFactor( 0, 1 );
   mpSplitter->setStretchFactor( 1, 1 );

   mpFindBar = new EditorFindBar( mpEditor, this );

   QVBoxLayout* lay = new QVBoxLayout( this );
   lay->setContentsMargins( 0, 0, 0, 0 );
   lay->setSpacing( 0 );
   lay->addWidget( mpSplitter, 1 );
   lay->addWidget( mpFindBar );

   // Live-update the preview (debounced) while it is visible. The editor
   // is hidden in preview-only mode, so this mainly drives split-view.
   mpMdPreviewDebounce = new QTimer( this );
   mpMdPreviewDebounce->setSingleShot( true );
   mpMdPreviewDebounce->setInterval( 250 );
   connect( mpMdPreviewDebounce, &QTimer::timeout,
            this, &CSingleEntryView::refreshMdPreview );
   connect( mpEditor, &QTextEdit::textChanged, this, [this]() {
      if ( mpMdPreview->isVisible() && mpMdPreviewDebounce )
         mpMdPreviewDebounce->start();
   });
}


// -------------------------------------------------------------------------------
CSingleEntryView::~CSingleEntryView( void )
// -------------------------------------------------------------------------------
{
   mpActiveElement = nullptr;
}

// ************** IView *********************************************************
// -------------------------------------------------------------------------------
void CSingleEntryView::aboutToRemoveElement( CInformationElement* pIE )
// -------------------------------------------------------------------------------
{
   if ( mpActiveElement == pIE )
   {
      mpActiveElement = nullptr;
   }
   mpEditor->aboutToRemoveElement( pIE );
}
// ************** IView - End ****************************************************


// -------------------------------------------------------------------------------
Editor* CSingleEntryView::getEditor( void )
// -------------------------------------------------------------------------------
{
   return mpEditor;
}


// -------------------------------------------------------------------------------
EditorFindBar* CSingleEntryView::getFindBar( void )
// -------------------------------------------------------------------------------
{
   return mpFindBar;
}


/**
 * This slot is called as soon as the encrypted entry is decrypted.
 */
// -------------------------------------------------------------------------------
void CSingleEntryView::entryDecrypted( void )
// -------------------------------------------------------------------------------
{
   activeInformationElementChanged( mpActiveElement );
}


// -------------------------------------------------------------------------------
QString CSingleEntryView::getText( void )
// -------------------------------------------------------------------------------
{
   return (nullptr != mpEditor) ? mpEditor->getText() : QString("");
}

// -------------------------------------------------------------------------------
void CSingleEntryView::setText( QString text )
// -------------------------------------------------------------------------------
{
   if (nullptr != mpEditor)
      mpEditor->setText( text );
}


// -------------------------------------------------------------------------------
void CSingleEntryView::writeCurrentTextToActiveInformationElement( void )
// -------------------------------------------------------------------------------
{
   if (nullptr != mpEditor)
      mpEditor->writeCurrentTextToActiveInformationElement();
}

// -------------------------------------------------------------------------------
int CSingleEntryView::countBRs( void )
// -------------------------------------------------------------------------------
{
   return (nullptr != mpEditor) ? mpEditor->countBRs() : 0;
}

// -------------------------------------------------------------------------------
void CSingleEntryView::activeInformationElementChanged( CInformationElement* pIE )
// -------------------------------------------------------------------------------
{
   if ( (nullptr == pIE) || (nullptr == mpEditor) )
      return;

   mpActiveElement = pIE;

   // mpEditor->activeInformationElementChanged() loads the content and
   // emits formatRecognized(); MainWindow::showRecognizedFormat() reacts
   // by setting the desired markdown view/edit mode via
   // setSinglePanePreview(), so we must NOT force a mode here (doing so
   // would clobber the default-view-on-navigate behavior).
   mpEditor->activeInformationElementChanged( mpActiveElement );
   updateView();
//   signalEntryDecrypted();
}


// ---- Markdown preview (split pane and single-pane toggle) --------------------

bool CSingleEntryView::currentEntryIsMarkdown() const
{
   return mpActiveElement &&
          mpActiveElement->getInformationFormat() == &InformationFormat::MARKDOWN;
}


// Decide editor/preview visibility from the current mode flags and
// refresh the rendered preview if it is shown.
void CSingleEntryView::updateView()
{
   const bool md          = currentEntryIsMarkdown();
   const bool previewOnly = mbPreviewOnly && md;
   const bool split       = mbSplitEnabled && md && !previewOnly;

   mbPreviewShown = previewOnly || split;
   mpEditor->setVisible( !previewOnly );
   mpMdPreview->setVisible( mbPreviewShown );

   if ( mbPreviewShown )
      refreshMdPreview();
}


void CSingleEntryView::refreshMdPreview()
{
   // Guard on the logical mode, NOT mpMdPreview->isVisible(): at startup
   // the last document auto-opens before the window is shown, so the
   // widget is not yet "visible" and the initial render would be skipped
   // (the entry then appeared blank until navigating away and back).
   if ( !mbPreviewShown ) return;
   // Render the editor's current text (the authoritative .md source —
   // the editor stays in plain mode and is never mutated for preview).
   mpMdPreview->document()->setMarkdown( mpEditor->toPlainText() );
}


void CSingleEntryView::setMarkdownSplitView( bool on )
{
   mbSplitEnabled = on;
   updateView();
}


void CSingleEntryView::setSinglePanePreview( bool on )
{
   mbPreviewOnly = on;
   updateView();
}


bool CSingleEntryView::isPreviewActive() const
{
   return mbPreviewOnly && currentEntryIsMarkdown();
}
