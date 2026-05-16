/****************************************************************************
** Icon selector implementation (Qt6 rewrite)
*****************************************************************************/

#include "iconselector.h"

#include <QDir>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QPixmap>
#include <QIcon>
#include <QImageReader>
#include <QListWidgetItem>

IconSelector::IconSelector( const QString &dir, QWidget *parent )
    : QListWidget( parent )
    , vm( Large )
{
    QListWidget::setViewMode( QListView::IconMode );
    setResizeMode( QListView::Adjust );
    setMovement( QListView::Static );
    setIconSize( QSize(32, 32) );
    setSpacing( 4 );
    setUniformItemSizes( true );
    setWordWrap( true );

    connect( this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this, SLOT(itemDoubleClickedSlot(QListWidgetItem*)) );

    setDirectory( dir );
}

void IconSelector::setViewMode( ViewMode m )
{
    vm = m;
    if ( m == Large )
        setIconSize( QSize(32, 32) );
    else
        setIconSize( QSize(16, 16) );
}

void IconSelector::setDirectory( const QString &dir )
{
    setDirectory( QDir(dir) );
}

void IconSelector::setDirectory( const QDir &dir )
{
    viewDir = dir;
    readDir( viewDir );
    emit directoryChanged( viewDir.absolutePath() );
    if ( viewDir.isRoot() )
        emit disableUp();
    else
        emit enableUp();
}

QDir IconSelector::currentDir()
{
    return viewDir;
}

void IconSelector::readDir( const QDir &dir )
{
    clear();

    QFileInfoList entries = dir.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot,
            QDir::DirsFirst | QDir::Name );

    emit startReadDir( entries.size() );

    QFileIconProvider iconProvider;
    QList<QByteArray> imageFormats = QImageReader::supportedImageFormats();

    for (const QFileInfo &fi : entries) {
        QListWidgetItem* item = new QListWidgetItem( this );
        item->setText( fi.fileName() );
        item->setData( Qt::UserRole, fi.absoluteFilePath() );

        if ( fi.isDir() ) {
            item->setIcon( iconProvider.icon(QFileIconProvider::Folder) );
        } else if ( imageFormats.contains( fi.suffix().toLower().toLatin1() ) ) {
            QPixmap pix( fi.absoluteFilePath() );
            if ( !pix.isNull() )
                item->setIcon( QIcon( pix ) );
            else
                item->setIcon( iconProvider.icon(QFileIconProvider::File) );
        } else {
            item->setIcon( iconProvider.icon(QFileIconProvider::File) );
        }

        emit readNextDir();
    }

    emit readDirDone();
}

void IconSelector::itemDoubleClickedSlot( QListWidgetItem* item )
{
    if ( !item ) return;
    QString path = item->data( Qt::UserRole ).toString();
    QFileInfo fi( path );
    if ( fi.isDir() )
        setDirectory( QDir(path) );
}
