/***************************************************************************
                          editorfindbar.h  -  description
                             -------------------
    In-editor Find / Replace bar — embedded at the bottom of the
    CSingleEntryView. Lives next to (not inside of) the tree-wide
    SearchDialog: Ctrl+F / Ctrl+H drive this bar, Ctrl+Shift+F still
    opens the SearchDialog for cross-entry search.
 ***************************************************************************/
#ifndef EDITORFINDBAR_H
#define EDITORFINDBAR_H

#include <QWidget>

class QLineEdit;
class QCheckBox;
class QLabel;
class QToolButton;
class QTextEdit;

class EditorFindBar : public QWidget
{
   Q_OBJECT
public:
   explicit EditorFindBar( QTextEdit* pEditor, QWidget* pParent = nullptr );

   // showFind(false)         -> Find-only mode (replace row hidden, F3)
   // showFind(true)          -> Find + Replace (replace row visible, Ctrl+H)
   void showFind( bool withReplace );

protected:
   void keyPressEvent( QKeyEvent* ev ) override;

public slots:
   void findNext();
   void findPrev();

private slots:
   void replaceCurrent();
   void replaceAll();
   void onFindTextChanged();

private:
   bool doFind( bool backward );

   QTextEdit*   mpEditor;

   QLineEdit*   mpFindEdit;
   QLineEdit*   mpReplaceEdit;
   QCheckBox*   mpCaseSensitive;
   QCheckBox*   mpWholeWords;
   QLabel*      mpStatus;

   QToolButton* mpClose;
   QToolButton* mpNext;
   QToolButton* mpPrev;
   QToolButton* mpReplace;
   QToolButton* mpReplaceAll;

   QWidget*     mpReplaceRow;
};

#endif
