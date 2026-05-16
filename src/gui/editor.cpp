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
#include <QRegExp>
#include <QKeyEvent>
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


QString Editor::getText( void )
{
  if ( acceptRichText() )
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

      writeCurrentTextToActiveInformationElement();
      mpActiveElement->setInformationYPos( verticalScrollBar()->value() );
   }

   if ( pElement->getInformationFormat() == &InformationFormat::RTF )
   {
      emit formatRecognized( InformationFormat::RTF );
      setAcceptRichText( true );
   }
   else if ( pElement->getInformationFormat() == &InformationFormat::ASCII )
   {
      emit formatRecognized( InformationFormat::ASCII );
      setAcceptRichText( false );
   }

   setText( pElement->getInformation() );
   verticalScrollBar()->setValue( pElement->getInformationYPos() );

   mpActiveElement = pElement;
   connect( mpActiveElement, &CInformationElement::informationHasChanged, this, &Editor::rereadInformation );
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
