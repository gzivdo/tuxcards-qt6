/****************************************************************************
** Icon selector dialog implementation (Qt6 rewrite)
*****************************************************************************/

#include "iconselector.h"
#include "iconselectordialog.h"

#include <QSplitter>
#include <QProgressBar>
#include <QLabel>
#include <QStatusBar>
#include <QToolBar>
#include <QComboBox>
#include <QPixmap>
#include <QToolButton>
#include <QDir>
#include <QFileInfo>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <iostream>

static const char* cdtoparent_xpm[]={
    "15 13 3 1",
    ". c None",
    "* c #000000",
    "a c #ffff99",
    "..*****........",
    ".*aaaaa*.......",
    "***************",
    "*aaaaaaaaaaaaa*",
    "*aaaa*aaaaaaaa*",
    "*aaa***aaaaaaa*",
    "*aa*****aaaaaa*",
    "*aaaa*aaaaaaaa*",
    "*aaaa*aaaaaaaa*",
    "*aaaa******aaa*",
    "*aaaaaaaaaaaaa*",
    "*aaaaaaaaaaaaa*",
    "***************"};

IconSelectorDialog::IconSelectorDialog()
 : QDialog( nullptr )
{
    setObjectName("IconSelectorDialog");
    setModal(true);
    setup();
}

void IconSelectorDialog::show(){
    QDialog::show();
}

void IconSelectorDialog::setup()
{
    QVBoxLayout* layout = new QVBoxLayout( this );

    setMinimumSize(400, 400);

    // upper-section
    QWidget* upper = new QWidget(this);
    QHBoxLayout* upperLayout = new QHBoxLayout(upper);
    QLabel* pathLabel = new QLabel( tr( " Path: " ), upper);
    pathLabel->setMaximumWidth(35);
    upperLayout->addWidget(pathLabel);

    pathCombo = new QComboBox(upper);
    pathCombo->setEditable(true);
    upperLayout->addWidget(pathCombo);
    connect(pathCombo, &QComboBox::textActivated,
            this, &IconSelectorDialog::changePath );

    QPixmap pix = QPixmap( cdtoparent_xpm );
    upButton = new QPushButton( pix, "", upper);
    upButton->setMaximumWidth(26);
    upperLayout->addWidget(upButton);
    connect(upButton, &QAbstractButton::pressed, this, &IconSelectorDialog::cdUp);

    layout->addWidget(upper);

    // middle-section
    fileview = new IconSelector("/", this);
    fileview->setSelectionMode(QAbstractItemView::ExtendedSelection);
    fileview->setDirectory(QDir::current());
    layout->addWidget(fileview);

    location = new QLabel("", this);
    location->setFrameStyle(QFrame::Box | QFrame::Raised);
    layout->addWidget(location);

    // lower-section
    QWidget* lower = new QWidget(this);
    QHBoxLayout* lowerLayout = new QHBoxLayout(lower);
    QPushButton* ok = new QPushButton(tr("Ok"), lower);
    connect(ok, &QAbstractButton::released, this, &IconSelectorDialog::slotOkPressed);
    lowerLayout->addWidget(ok);
    QPushButton* cancel = new QPushButton(tr("Cancel"), lower);
    connect(cancel, &QAbstractButton::released, this, &IconSelectorDialog::slotCancelPressed);
    lowerLayout->addWidget(cancel);
    layout->addWidget(lower);

    connect( fileview, &IconSelector::directoryChanged,
             this, &IconSelectorDialog::directoryChanged );
    connect( fileview, &IconSelector::startReadDir,
             this, &IconSelectorDialog::slotStartReadDir );
    connect( fileview, &IconSelector::readNextDir,
             this, &IconSelectorDialog::slotReadNextDir );
    connect( fileview, &IconSelector::readDirDone,
             this, &IconSelectorDialog::slotReadDirDone );
    connect( fileview, &QListWidget::itemClicked,
             this, &IconSelectorDialog::slotFileSelected );

    progress = new QProgressBar(this);
    layout->addWidget(progress);

    connect( fileview, &IconSelector::enableUp,  this, &IconSelectorDialog::enableUp );
    connect( fileview, &IconSelector::disableUp, this, &IconSelectorDialog::disableUp );

    directoryChanged( QDir::current().absolutePath() );
}

void IconSelectorDialog::slotOkPressed(){
    accept();
}

void IconSelectorDialog::slotCancelPressed(){
    reject();
}

QString IconSelectorDialog::getIconFileName(){
    if (location->text().isEmpty())
        return "";
    return pathCombo->currentText() + "/" + location->text();
}

void IconSelectorDialog::slotFileSelected( QListWidgetItem* item ){
    if ( !item ) return;
    location->setText( item->text() );
}

void IconSelectorDialog::setPathCombo(){
    QString dir = windowTitle();
    int found = -1;
    for ( int i = 0; i < pathCombo->count(); i++ ) {
        if ( pathCombo->itemText(i) == dir ) {
            found = i;
            break;
        }
    }

    if ( found >= 0 ) {
        pathCombo->setCurrentIndex( found );
    } else {
        pathCombo->addItem( dir );
        pathCombo->setCurrentIndex( pathCombo->count() - 1 );
    }
}

void IconSelectorDialog::directoryChanged(const QString &dir){
    setWindowTitle(dir);
    setPathCombo();
}

void IconSelectorDialog::slotStartReadDir( int dirs )
{
    progress->reset();
    progress->setRange(0, dirs);
}

void IconSelectorDialog::slotReadNextDir()
{
    int p = progress->value();
    progress->setValue( ++p );
}

void IconSelectorDialog::slotReadDirDone()
{
    progress->setValue( progress->maximum() );
}

void IconSelectorDialog::cdUp(){
    QDir dir = fileview->currentDir();
    dir.cd("..");
    fileview->setDirectory(dir);
}

void IconSelectorDialog::changePath(const QString &path){
    if ( QFileInfo(path).exists() )
        fileview->setDirectory(path);
    else
        setPathCombo();
}

void IconSelectorDialog::enableUp(){
    upButton->setEnabled(true);
}

void IconSelectorDialog::disableUp(){
    upButton->setEnabled(false);
}
