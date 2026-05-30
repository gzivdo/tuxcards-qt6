/***************************************************************************
                          editorfindbar.cpp  -  description
 ***************************************************************************/

#include "editorfindbar.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QShortcut>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>


EditorFindBar::EditorFindBar( QTextEdit* pEditor, QWidget* pParent )
 : QWidget( pParent )
 , mpEditor( pEditor )
 , mpFindEdit( nullptr )
 , mpReplaceEdit( nullptr )
 , mpCaseSensitive( nullptr )
 , mpWholeWords( nullptr )
 , mpStatus( nullptr )
 , mpClose( nullptr )
 , mpNext( nullptr )
 , mpPrev( nullptr )
 , mpReplace( nullptr )
 , mpReplaceAll( nullptr )
 , mpReplaceRow( nullptr )
{
   QVBoxLayout* root = new QVBoxLayout( this );
   root->setContentsMargins( 4, 2, 4, 2 );
   root->setSpacing( 2 );

   // ---- find row ------------------------------------------------------
   QHBoxLayout* findRow = new QHBoxLayout();
   findRow->setSpacing( 4 );
   findRow->addWidget( new QLabel( tr("Find:"), this ) );

   mpFindEdit = new QLineEdit( this );
   findRow->addWidget( mpFindEdit, 1 );

   mpPrev = new QToolButton( this );
   mpPrev->setText( tr("◀") );
   mpPrev->setToolTip( tr("Find previous (Shift+F3)") );
   findRow->addWidget( mpPrev );

   mpNext = new QToolButton( this );
   mpNext->setText( tr("▶") );
   mpNext->setToolTip( tr("Find next (F3)") );
   findRow->addWidget( mpNext );

   mpCaseSensitive = new QCheckBox( tr("&Case sensitive"), this );
   findRow->addWidget( mpCaseSensitive );

   mpWholeWords = new QCheckBox( tr("&Whole words"), this );
   findRow->addWidget( mpWholeWords );

   mpStatus = new QLabel( this );
   findRow->addWidget( mpStatus, 1 );

   mpClose = new QToolButton( this );
   mpClose->setText( tr("✕") );
   mpClose->setToolTip( tr("Close (Esc)") );
   findRow->addWidget( mpClose );

   root->addLayout( findRow );

   // ---- replace row (hidden until Ctrl+H) -----------------------------
   mpReplaceRow = new QWidget( this );
   QHBoxLayout* repRow = new QHBoxLayout( mpReplaceRow );
   repRow->setContentsMargins( 0, 0, 0, 0 );
   repRow->setSpacing( 4 );
   repRow->addWidget( new QLabel( tr("Replace:"), mpReplaceRow ) );

   mpReplaceEdit = new QLineEdit( mpReplaceRow );
   repRow->addWidget( mpReplaceEdit, 1 );

   mpReplace = new QToolButton( mpReplaceRow );
   mpReplace->setText( tr("Replace") );
   repRow->addWidget( mpReplace );

   mpReplaceAll = new QToolButton( mpReplaceRow );
   mpReplaceAll->setText( tr("Replace All") );
   repRow->addWidget( mpReplaceAll );

   repRow->addStretch( 1 );
   root->addWidget( mpReplaceRow );
   mpReplaceRow->hide();

   hide();

   // ---- signals -------------------------------------------------------
   connect( mpClose,       &QToolButton::clicked, this, &QWidget::hide );
   connect( mpNext,        &QToolButton::clicked, this, &EditorFindBar::findNext );
   connect( mpPrev,        &QToolButton::clicked, this, &EditorFindBar::findPrev );
   connect( mpReplace,     &QToolButton::clicked, this, &EditorFindBar::replaceCurrent );
   connect( mpReplaceAll,  &QToolButton::clicked, this, &EditorFindBar::replaceAll );
   connect( mpFindEdit,    &QLineEdit::returnPressed, this, &EditorFindBar::findNext );
   connect( mpReplaceEdit, &QLineEdit::returnPressed, this, &EditorFindBar::replaceCurrent );
   connect( mpFindEdit,    &QLineEdit::textChanged,   this, &EditorFindBar::onFindTextChanged );

   // Shift+F3 = find previous (while the bar's host window has focus).
   // F3 = find-next is handled by MainWindow::editorFind() as a smart
   // open-or-next QAction; binding it here too would produce an
   // "Ambiguous shortcut overload" warning at runtime.
   QShortcut* sc = new QShortcut( QKeySequence( Qt::SHIFT | Qt::Key_F3 ), this );
   sc->setContext( Qt::WindowShortcut );
   connect( sc, &QShortcut::activated, this, &EditorFindBar::findPrev );
}


void EditorFindBar::showFind( bool withReplace )
{
   mpReplaceRow->setVisible( withReplace );

   // If the user has a selection in the editor, prefill the search box.
   if ( mpEditor ) {
      const QString sel = mpEditor->textCursor().selectedText();
      if ( !sel.isEmpty() && !sel.contains(QChar::ParagraphSeparator) )
         mpFindEdit->setText( sel );
   }

   show();
   mpFindEdit->setFocus();
   mpFindEdit->selectAll();
   mpStatus->clear();
}


void EditorFindBar::keyPressEvent( QKeyEvent* ev )
{
   if ( ev->key() == Qt::Key_Escape ) {
      hide();
      if ( mpEditor ) mpEditor->setFocus();
      return;
   }
   QWidget::keyPressEvent( ev );
}


void EditorFindBar::onFindTextChanged()
{
   mpStatus->clear();
}


bool EditorFindBar::doFind( bool backward )
{
   if ( !mpEditor )
      return false;
   const QString needle = mpFindEdit->text();
   if ( needle.isEmpty() )
      return false;

   QTextDocument::FindFlags flags;
   if ( backward )
      flags |= QTextDocument::FindBackward;
   if ( mpCaseSensitive->isChecked() )
      flags |= QTextDocument::FindCaseSensitively;
   if ( mpWholeWords->isChecked() )
      flags |= QTextDocument::FindWholeWords;

   if ( mpEditor->find( needle, flags ) ) {
      mpStatus->clear();
      return true;
   }

   // Wrap around — move cursor to one end of the document and retry.
   QTextCursor wrap = mpEditor->textCursor();
   wrap.movePosition( backward ? QTextCursor::End : QTextCursor::Start );
   mpEditor->setTextCursor( wrap );
   if ( mpEditor->find( needle, flags ) ) {
      mpStatus->setText( tr("(wrapped)") );
      return true;
   }

   mpStatus->setText( tr("<i>not found</i>") );
   return false;
}


void EditorFindBar::findNext() { doFind( false ); }
void EditorFindBar::findPrev() { doFind( true ); }


void EditorFindBar::replaceCurrent()
{
   if ( !mpEditor )
      return;

   QTextCursor cur = mpEditor->textCursor();
   const QString needle = mpFindEdit->text();
   if ( needle.isEmpty() )
      return;

   // If the current selection is the match we just found, replace it.
   // Otherwise step to the next match and stop — user clicks Replace
   // a second time to actually substitute (browser-style behaviour).
   const Qt::CaseSensitivity cs = mpCaseSensitive->isChecked()
      ? Qt::CaseSensitive : Qt::CaseInsensitive;
   if ( cur.hasSelection() &&
        QString::compare( cur.selectedText(), needle, cs ) == 0 )
   {
      cur.insertText( mpReplaceEdit->text() );
   }
   doFind( false );
}


void EditorFindBar::replaceAll()
{
   if ( !mpEditor )
      return;
   const QString needle = mpFindEdit->text();
   if ( needle.isEmpty() )
      return;

   QTextDocument::FindFlags flags;
   if ( mpCaseSensitive->isChecked() )
      flags |= QTextDocument::FindCaseSensitively;
   if ( mpWholeWords->isChecked() )
      flags |= QTextDocument::FindWholeWords;

   // Move to start, then iterate. Wrap into a single undo step.
   QTextCursor cur( mpEditor->document() );
   cur.beginEditBlock();
   QTextCursor it = mpEditor->document()->find( needle, 0, flags );
   int count = 0;
   while ( !it.isNull() ) {
      it.insertText( mpReplaceEdit->text() );
      it = mpEditor->document()->find( needle, it, flags );
      ++count;
   }
   cur.endEditBlock();

   mpStatus->setText( tr("<i>%n replacement(s)</i>", "", count) );
}
