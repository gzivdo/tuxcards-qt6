/***************************************************************************
                          CSingleEntryView.h  -  description
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
#ifndef CSINGLE_ENTRY_VIEW_H
#define CSINGLE_ENTRY_VIEW_H

#include <QWidget>
#include "../information/IView.h"
#include "editor.h"

class EditorFindBar;
class QSplitter;
class QTextBrowser;
class QTimer;
class QVBoxLayout;


class CSingleEntryView : public QWidget,
                         public IView
{
   Q_OBJECT
public:
   CSingleEntryView( QWidget* pParent );
   ~CSingleEntryView( void );

   Editor*        getEditor( void );
   EditorFindBar* getFindBar( void );

   // methods added because of editor
   QString       getText( void );
   virtual void  setText( QString text );

   void          writeCurrentTextToActiveInformationElement( void );

   int           countBRs( void );
   // methods added because of editor - end

   // Configure whether MARKDOWN entries should show the live preview
   // pane next to the editor. Called by MainWindow when the user flips
   // the Options → Markdown checkbox and when the active element
   // changes (so non-markdown entries collapse back to single-pane).
   void          setMarkdownSplitView( bool on );

   // ************** IView *************************************
   virtual void aboutToRemoveElement( CInformationElement* pIE );
   // ************** IView - End *******************************

public slots:
   // methods added because of editor
   void          activeInformationElementChanged( CInformationElement* pIE );
   // methods added because of editor - end

protected:

private:
   CInformationElement* mpActiveElement;
   Editor*              mpEditor;
   EditorFindBar*       mpFindBar;

   // Split-view widgets — created lazily on first showSplit(true).
   QSplitter*           mpSplitter;
   QTextBrowser*        mpMdPreview;
   QTimer*              mpMdPreviewDebounce;
   bool                 mbSplitActive;
   bool                 mbSplitEnabled;   // user preference from config

   void                 buildSplitterLazy();
   bool                 currentEntryIsMarkdown() const;
   void                 applySplitVisibility();

private slots:
   void entryDecrypted( void );
   void refreshMdPreview();

signals:
   void signalEntryDecrypted();
};

#endif
