/****************************************************************************
** Icon selector dialog (Qt6 rewrite)
*****************************************************************************/

#ifndef ICONSELECTORDIALOG_H
#define ICONSELECTORDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>

class IconSelector;
class QProgressBar;
class QComboBox;

class IconSelectorDialog : public QDialog{
    Q_OBJECT

public:
  IconSelectorDialog();

    IconSelector *fileView() { return fileview; }

    void    show();
    bool    getResult();
    QString getIconFileName();

protected:
  void setup();
  void setPathCombo();

  IconSelector *fileview;
  QLabel* location;
  QProgressBar *progress;
  QLabel *label;
  QComboBox *pathCombo;
  QPushButton *upButton, *mkdirButton;

  bool result;

protected slots:
  void directoryChanged( const QString & );
  void slotStartReadDir( int dirs );
  void slotReadNextDir();
  void slotReadDirDone();
  void cdUp();
  void changePath( const QString &path );
  void enableUp();
  void disableUp();

  void slotOkPressed();
  void slotCancelPressed();
  void slotFileSelected( QListWidgetItem* item );

private:
  QString mstrIconDir;
};

#endif
