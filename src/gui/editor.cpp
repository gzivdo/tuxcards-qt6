/***************************************************************************
                          editor.cpp  -  description
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
#include "editor.h"

#include <QTextDocument>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QRegExp>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <QMimeData>
#include <QApplication>
#include <QFileDialog>
#include <QImage>
#include "../CTuxCardsConfiguration.h"
#include "../utilities/strings.h"


Editor::Editor( QWidget *pParent, const char* /*pName*/ )
  : QTextEdit( pParent )
  , mpActiveElement( nullptr )
  , SEMAPHORE_TEXT_WAS_SET( false )
{
  initialize();
}


Editor::~Editor( void )
{
   mpActiveElement = nullptr;
}


void Editor::aboutToRemoveElement( CInformationElement* pIE )
{
   if ( mpActiveElement == pIE )
   {
      mpActiveElement = nullptr;
   }
}


// Append a "Reset formatting" entry to the editor's standard right-click
// menu. We don't act on it here — MainWindow has the format-toolbar /
// format-flag wiring and listens for resetFormattingRequested().
void Editor::contextMenuEvent( QContextMenuEvent* ev )
{
   QMenu* menu = createStandardContextMenu();
   menu->addSeparator();
   QAction* a = menu->addAction(tr("Reset formatting of selection"));
   a->setEnabled( textCursor().hasSelection() );
   connect(a, &QAction::triggered, this, &Editor::resetFormattingRequested);
   menu->exec(ev->globalPos());
   delete menu;
}


QString Editor::getText( void )
{
  // Drive the serialization by the element's FORMAT, not the widget's
  // acceptRichText() state: the latter is flipped to true during a
  // markdown preview, but a MARKDOWN (or TEXT) entry must always be
  // stored as its plain source, never as toHtml().
  if ( mpActiveElement &&
       mpActiveElement->getInformationFormat() == &InformationFormat::HTML )
     return toHtml();
  return toPlainText();
}



void Editor::setText( QString text )
{
  SEMAPHORE_TEXT_WAS_SET = true;
  if ( acceptRichText() )
     QTextEdit::setHtml(text);
  else
     QTextEdit::setPlainText(text);
}




void Editor::rereadInformation( void )
{
   if ( nullptr == mpActiveElement )
      return;

   setText( mpActiveElement->getInformation() );
}



void Editor::clear( void )
{
  initialize();
}



void Editor::initialize( void )
{
  QTextEdit::clear();
  QTextEdit::setAutoFormatting(AutoNone);
  mpActiveElement = nullptr;

  SEMAPHORE_TEXT_WAS_SET = false;
  connect( this, &QTextEdit::textChanged, this, &Editor::sendUndoAvailableSignal );
  connect( this, &QTextEdit::textChanged, this, &Editor::sendRedoAvailableSignal );
}



void Editor::sendUndoAvailableSignal( void )
{
  if ( document()->isUndoAvailable() )
  {
    if ( SEMAPHORE_TEXT_WAS_SET )
      SEMAPHORE_TEXT_WAS_SET = false;
    else
      emit undoAvailable( true );
  }
}



void Editor::sendRedoAvailableSignal( void )
{
  if ( document()->isRedoAvailable() )
    emit redoAvailable(true);
}




void Editor::writeCurrentTextToActiveInformationElement( void )
{
   if ( !mpActiveElement )
      return;

   mpActiveElement->setInformation(getText());
}


void Editor::setWordWrap( int wordWrap )
{
  if( wordWrap == 0 )
    QTextEdit::setLineWrapMode(QTextEdit::NoWrap);
  else if( wordWrap == 1 )
    QTextEdit::setLineWrapMode(QTextEdit::WidgetWidth);
  else
  {
    QTextEdit::setLineWrapMode(QTextEdit::FixedColumnWidth);
    QTextEdit::setLineWrapColumnOrWidth(wordWrap);
  }
}



void Editor::keyPressEvent( QKeyEvent* pKeyEv )
{
   if ( !pKeyEv )
      return;

   Qt::KeyboardModifiers mods = pKeyEv->modifiers();
   int key = pKeyEv->key();

   if( ( (mods & Qt::ControlModifier)  &&  (key == Qt::Key_S) ) ||
       ( (mods & Qt::AltModifier)      &&  (key == Qt::Key_Left) ) ||
       ( (mods & Qt::AltModifier)      &&  (key == Qt::Key_Right) ) ||
       ( (mods & Qt::ControlModifier)  &&  (key == Qt::Key_F) ) ||
       // Ctrl+H is bound to "delete previous char" by QTextEdit on some
       // platforms — intercept so MainWindow can open the Replace bar.
       ( (mods & Qt::ControlModifier)  &&  (key == Qt::Key_H) ) )
   {
      pKeyEv->ignore();
   }

   else if ( (mods & Qt::ControlModifier) && (Qt::Key_A == key) )
   {
      selectAll();
   }

   else if ( (mods & Qt::ControlModifier) && (Qt::Key_B == key) )
   {
      if ( acceptRichText() )
         setFontWeight( fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold );
   }

   else if ( (mods & Qt::ControlModifier) && (Qt::Key_I == key) )
   {
      if ( acceptRichText() )
         setFontItalic( !fontItalic() );
   }

   else if ( (mods & Qt::ControlModifier) && (Qt::Key_U == key) )
   {
      if ( acceptRichText() )
         setFontUnderline( !fontUnderline() );
   }

   else
   {
      QTextEdit::keyPressEvent( pKeyEv );
   }
}


void Editor::activeInformationElementChanged( CInformationElement* pElement )
{
   if ( !pElement )
      return;

   if ( mpActiveElement )
   {
      disconnect( mpActiveElement, &CInformationElement::informationHasChanged, this, &Editor::rereadInformation );

      // Save the outgoing element. getText() serializes by the element's
      // format (markdown/text → plain source, html → toHtml), and the
      // editor is never mutated for preview, so this can't corrupt a
      // markdown source into HTML.
      writeCurrentTextToActiveInformationElement();
      mpActiveElement->setInformationYPos( verticalScrollBar()->value() );
   }

   loadElementContent( pElement );
}


// Configure the editor for pElement's format and load its content. Does
// NOT save the previously-shown content — callers that switch elements
// use activeInformationElementChanged(); callers that changed the
// current element's content/format in place use reloadActiveElement().
void Editor::loadElementContent( CInformationElement* pElement )
{
   if ( pElement->getInformationFormat() == &InformationFormat::HTML )
   {
      setAcceptRichText( true );
   }
   else
   {
      // TEXT or MARKDOWN — plain-text editing of the raw bytes. Clear any
      // char formatting inherited from a previously-viewed rich-text
      // entry so the raw text always shows in the default editor font at
      // one size, not e.g. bold/large left over from an HTML heading.
      setAcceptRichText( false );
      QTextCharFormat fmt;
      fmt.setFont( font() );        // the configured default editor font
      setCurrentCharFormat( fmt );
   }

   setText( pElement->getInformation() );
   verticalScrollBar()->setValue( pElement->getInformationYPos() );

   if ( mpActiveElement != pElement )
   {
      if ( mpActiveElement )
         disconnect( mpActiveElement, &CInformationElement::informationHasChanged,
                     this, &Editor::rereadInformation );
      mpActiveElement = pElement;
      connect( mpActiveElement, &CInformationElement::informationHasChanged,
               this, &Editor::rereadInformation );
   }

   // Notify listeners AFTER the content is in place, so a preview driven
   // by showRecognizedFormat() renders the new text, not the old buffer.
   emit formatRecognized( *pElement->getInformationFormat() );
}


// Re-display the current element after its content/format was changed
// programmatically (format conversion, markdown import), WITHOUT first
// writing the stale editor buffer back over the new content.
void Editor::reloadActiveElement( void )
{
   if ( mpActiveElement )
      loadElementContent( mpActiveElement );
}


void Editor::printBRs( void )
{
  std::cout<<countBRs()<<std::endl;
}


int Editor::countBRs( void )
{
   if ( !mpActiveElement )
      return 0;

   return mpActiveElement->getInformation().count("<br />");
}


void Editor::paste()
{
   if (QApplication::clipboard()->supportsSelection())
     adaptClipboardText( QClipboard::Selection );

   adaptClipboardText( QClipboard::Clipboard );

   QTextEdit::paste();
}


void Editor::insertImage()
{
   if ( !acceptRichText() )
      return;
   QString fn = QFileDialog::getOpenFileName( this, tr("Insert Image"),
                  QString(),
                  tr("Images (*.png *.jpg *.jpeg *.gif *.bmp *.xpm);;All files (*)") );
   if ( fn.isEmpty() )
      return;
   QImage img( fn );
   if ( img.isNull() )
      return;
   textCursor().insertImage( img );
}


void Editor::adaptClipboardText( QClipboard::Mode mode )
{
    QClipboard* pCb = QApplication::clipboard();
    if ( nullptr == pCb )
      return;

    const QMimeData* mimeData = pCb->mimeData(mode);
    if ( !mimeData )
      return;

    QStringList mimeFormatList = mimeData->formats();
    QString text = pCb->text(mode);

    if (!text.isEmpty() && !mimeFormatList.contains("application/x-qrichtext"))
    {
        // do not create new paragraphs
        text.replace(QChar('\n'), QChar(0x2028));
        pCb->setText( text, mode );
    }
}
