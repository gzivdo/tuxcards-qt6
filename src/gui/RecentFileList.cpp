/***************************************************************************
                          RecentFileList.cpp  -  description
                             -------------------
    Adapted from TuxCards 2.2.1.
 ***************************************************************************/

#include "RecentFileList.h"
#include "../global.h"

#include <QFile>
#include <QMessageBox>
#include <QComboBox>
#include <QMenu>
#include <QAction>

const QString RecentFileList::SEPARATOR         = ",";
const int     RecentFileList::MAX_ELEMENT_COUNT = 5;

RecentFileList::RecentFileList( QWidget* pParent, QMenu* pParentMenu, const QString& sFiles )
 : QObject()
 , mpParentMenu( pParentMenu )
 , mpRecentlyFilesMenu( nullptr )
 , mpComboBox( nullptr )
{
   mpRecentlyFilesMenu = new QMenu( tr("Recently Used Files"), pParent );
   if ( pParentMenu )
      pParentMenu->addMenu( mpRecentlyFilesMenu );

   createActions( pParent );

   setList( sFiles );
   update();
}

void RecentFileList::createActions( QWidget* pParent )
{
   for ( int i = 0; i < MAX_ELEMENT_COUNT; i++ )
   {
      QAction* pAction = new QAction( pParent );
      pAction->setVisible( false );
      connect( pAction, SIGNAL(triggered()), this, SLOT(slotOpenRecentFile()) );

      mpRecentlyFilesMenu->addAction( pAction );
      mRecentFileActs.append( pAction );
   }
}

void RecentFileList::setOnTop( const QString& sAbsPath )
{
   remove( sAbsPath );
   mFileList.prepend( sAbsPath );

   update();
}

void RecentFileList::setList( const QString& sFiles )
{
   if ( sFiles.isEmpty() )
   {
      mFileList.clear();
      return;
   }
   mFileList = sFiles.split( SEPARATOR, Qt::SkipEmptyParts );
}

void RecentFileList::update()
{
   checkSize();
   updateMenu();
   updateComboBox();
}

void RecentFileList::checkSize()
{
   while ( mFileList.count() > MAX_ELEMENT_COUNT )
      mFileList.removeLast();
}

void RecentFileList::updateMenu()
{
   for ( int i = 0; i < mFileList.count(); i++ )
   {
      QString sText = QString("&%1  %2").arg(i + 1).arg( mFileList[i] );
      mRecentFileActs[i]->setText( sText );
      mRecentFileActs[i]->setData( mFileList[i] );
      mRecentFileActs[i]->setVisible( true );
   }

   for ( int j = mFileList.count(); j < MAX_ELEMENT_COUNT; ++j )
      mRecentFileActs[j]->setVisible( false );
}

void RecentFileList::slotOpenRecentFile()
{
   QAction* pAction = qobject_cast<QAction*>( sender() );
   if ( !pAction )
      return;

   QString sFileName = pAction->data().toString();

   if ( QFile::exists(sFileName) )
   {
      emit openFile( sFileName );
      setOnTop( sFileName );
   }
   else
   {
      if ( QMessageBox::Yes == QMessageBox::warning(0, tr("File not found"),
                tr("The file '") + sFileName + tr("' does not exist.\n"
                   "Do you want to remove it from the recent file menu?"),
                QMessageBox::Yes | QMessageBox::No) )
      {
         remove( sFileName );
         update();
      }
   }
}

void RecentFileList::remove( const QString& sAbsPath )
{
   if ( mFileList.contains( sAbsPath ) )
   {
      int iIndex = mFileList.indexOf( sAbsPath );
      if ( -1 != iIndex )
         mFileList.removeAt( iIndex );
   }
}

void RecentFileList::createComboBox( QWidget& parentWidget )
{
   if ( mpComboBox )
      DELETE( mpComboBox );

   mpComboBox = new QComboBox( &parentWidget );
   connect( mpComboBox, SIGNAL( activated( int ) ),
            this, SLOT( slotComboActivated( int ) ) );

   updateComboBox();
}

void RecentFileList::updateComboBox()
{
   if ( !mpComboBox )
      return;

   mpComboBox->clear();
   mpComboBox->addItems( mFileList );
}

void RecentFileList::slotComboActivated( int idx )
{
   if ( idx < 0 || idx >= mFileList.count() )
      return;
   QString sFileName = mFileList[idx];
   if ( QFile::exists(sFileName) )
   {
      emit openFile( sFileName );
      setOnTop( sFileName );
   }
}

QString RecentFileList::toString() const
{
   return mFileList.join( SEPARATOR );
}
