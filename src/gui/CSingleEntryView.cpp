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
  , mbSplitActive( false )
  , mbSplitEnabled( false )
// -------------------------------------------------------------------------------
{
   mpEditor = new Editor( this );

   if ( (nullptr == mpEditor) )
   {
      std::cout<<"Constructor 'CSingleEntryView': ERROR not enough memory "
               <<" to create objects!!!"<<std::endl;
      return;
   }

   mpFindBar = new EditorFindBar( mpEditor, this );

   QVBoxLayout* lay = new QVBoxLayout( this );
   lay->setContentsMargins( 0, 0, 0, 0 );
   lay->setSpacing( 0 );
   lay->addWidget( mpEditor, 1 );
   lay->addWidget( mpFindBar );
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

   mpEditor->activeInformationElementChanged( mpActiveElement );
   // Splitter visibility depends on (config flag) AND (entry is markdown).
   applySplitVisibility();
   if ( mbSplitActive )
      refreshMdPreview();
//   signalEntryDecrypted();
}


// ---- Split-view (Markdown preview pane) --------------------------------------

bool CSingleEntryView::currentEntryIsMarkdown() const
{
   return mpActiveElement &&
          mpActiveElement->getInformationFormat() == &InformationFormat::MARKDOWN;
}


void CSingleEntryView::buildSplitterLazy()
{
   if ( mpSplitter ) return;

   mpSplitter = new QSplitter( Qt::Horizontal, this );
   mpMdPreview = new QTextBrowser( mpSplitter );
   mpMdPreview->setOpenExternalLinks( true );

   // Debounce textChanged: re-rendering on every keystroke is wasteful
   // for large entries. 250 ms is comfortably below "feels laggy".
   mpMdPreviewDebounce = new QTimer( this );
   mpMdPreviewDebounce->setSingleShot( true );
   mpMdPreviewDebounce->setInterval( 250 );
   connect( mpMdPreviewDebounce, &QTimer::timeout,
            this, &CSingleEntryView::refreshMdPreview );
   connect( mpEditor, &QTextEdit::textChanged, this, [this]() {
      if ( mbSplitActive && mpMdPreviewDebounce )
         mpMdPreviewDebounce->start();
   });
}


void CSingleEntryView::applySplitVisibility()
{
   const bool wantSplit = mbSplitEnabled && currentEntryIsMarkdown();
   if ( wantSplit == mbSplitActive )
      return;

   mbSplitActive = wantSplit;
   QVBoxLayout* lay = qobject_cast<QVBoxLayout*>( layout() );
   if ( !lay ) return;

   if ( wantSplit ) {
      buildSplitterLazy();
      // Re-parent editor into the splitter; remove from outer layout first.
      lay->removeWidget( mpEditor );
      mpSplitter->addWidget( mpEditor );
      mpSplitter->addWidget( mpMdPreview );
      mpSplitter->setStretchFactor( 0, 1 );
      mpSplitter->setStretchFactor( 1, 1 );
      // Insert splitter where the editor used to be (index 0, above find bar).
      lay->insertWidget( 0, mpSplitter, 1 );
      mpSplitter->show();
   } else if ( mpSplitter ) {
      // Reverse the wrap: pull the editor back out, drop the splitter.
      mpSplitter->setParent( nullptr );
      mpEditor->setParent( this );
      lay->insertWidget( 0, mpEditor, 1 );
      mpSplitter->hide();
   }
}


void CSingleEntryView::refreshMdPreview()
{
   if ( !mbSplitActive || !mpMdPreview ) return;
   // Render from the editor's current text (the authoritative .md
   // source while we're in MARKDOWN+plain-edit mode).
   mpMdPreview->document()->setMarkdown( mpEditor->toPlainText() );
}


void CSingleEntryView::setMarkdownSplitView( bool on )
{
   mbSplitEnabled = on;
   applySplitVisibility();
   if ( mbSplitActive )
      refreshMdPreview();
}
