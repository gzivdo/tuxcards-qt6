/***************************************************************************
                          editor.h  -  description
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
#ifndef EDITOR_H
#define EDITOR_H

#include "../global.h"
#include "./../information/IView.h"
#include <QTextEdit>
#include <qclipboard.h>
#include <QKeyEvent>

#include "./../information/CInformationElement.h"
#include "./../fontsettings.h"
#include <iostream>


class Editor : public QTextEdit,
               public IView
{
   Q_OBJECT
public:
   Editor( QWidget *pParent = nullptr, const char *pName = nullptr );
   virtual       ~Editor( void );

   QString       getText( void );
   virtual void  setText( QString text );
   void          clear( void );

   // Preview mode: while a MARKDOWN entry is shown as rendered output
   // (rich text + images) the editor's document no longer holds the
   // authoritative .md source, so writeCurrentTextToActiveInformationElement()
   // must NOT save it — that would overwrite the markdown source with
   // rendered HTML. MainWindow drives this around the preview toggle.
   void          setPreviewMode( bool on ) { mbPreviewMode = on; }
   bool          previewMode() const       { return mbPreviewMode; }

   void          setWordWrap( int wordWrap );

   int           countBRs( void );

   // ************** IView *************************************
   virtual void aboutToRemoveElement( CInformationElement* pIE );
   // ************** IView - End *******************************

signals:
    void         formatRecognized( InformationFormat );
    // Emitted when the user chooses "Reset formatting" from the editor's
    // right-click menu. MainWindow drives the actual conversion (touches
    // the active CInformationElement and the format toolbar wiring).
    void         resetFormattingRequested();


protected:
    void         contextMenuEvent( QContextMenuEvent* ev ) override;


public slots:
   //void        setBold(bool);
   void          activeInformationElementChanged( CInformationElement* );
   // Re-show the current element after its content/format changed in
   // place (format conversion / markdown import) without saving the
   // stale editor buffer over it.
   void          reloadActiveElement( void );
   virtual void  paste();
   void          insertImage();

   void          writeCurrentTextToActiveInformationElement( void );

private slots:
   void          sendUndoAvailableSignal( void );
   void          sendRedoAvailableSignal( void );

   void          rereadInformation();


protected:
   void                  initialize( void );
   virtual void          keyPressEvent( QKeyEvent* pKeyEv );

   CInformationElement*  mpActiveElement;


private:
   void                  printBRs( void );
   void                  adaptClipboardText( QClipboard::Mode mode );
   void                  loadElementContent( CInformationElement* pElement );

   bool                  SEMAPHORE_TEXT_WAS_SET;
   bool                  mbPreviewMode = false;
};

#endif

