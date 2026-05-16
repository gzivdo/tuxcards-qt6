/****************************************************************************
** Icon selector (Qt6 rewrite)
*****************************************************************************/

#ifndef ICONSELECTOR_H
#define ICONSELECTOR_H


#include <QIcon>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QListWidget>
#include <QPixmap>
#include <QResizeEvent>
#include <QDropEvent>
#include <QList>
#include <QKeyEvent>


/*****************************************************************************
 *
 * Class IconSelector
 *
 *****************************************************************************/
class IconSelector : public QListWidget
{
    Q_OBJECT

public:
    IconSelector( const QString &dir, QWidget *parent = nullptr );

    enum ViewMode { Large, Small };

    void setViewMode( ViewMode m );
    ViewMode viewMode() const { return vm; }

public slots:
    void setDirectory( const QString &dir );
    void setDirectory( const QDir &dir );
    QDir currentDir();

signals:
    void directoryChanged( const QString & );
    void startReadDir( int dirs );
    void readNextDir();
    void readDirDone();
    void enableUp();
    void disableUp();

protected slots:
    void itemDoubleClickedSlot( QListWidgetItem* item );

protected:
    void readDir( const QDir &dir );

    QDir viewDir;
    QSize sz;
    ViewMode vm;
};

#endif
