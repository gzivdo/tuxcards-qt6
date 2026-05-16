/***************************************************************************
                          search.cpp  -  description
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

#include "../../global.h"
#include "searchdialog.h"
#include "searchhighlightdelegate.h"
#include <iostream>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>

SearchDialog::SearchDialog( QWidget* pParent )
  : QDialog( pParent )
{
   setObjectName("SearchDialog");
   setModal(false);
   setWindowTitle(tr("Search"));
   setMinimumSize(650, 380);

   QVBoxLayout* root = new QVBoxLayout(this);
   root->setContentsMargins(10,10,10,10);
   root->setSpacing(6);

   // -- top: "Search for ..." + line + Go --
   root->addWidget(new QLabel(tr("Search for ..."), this));
   QHBoxLayout* topRow = new QHBoxLayout();
   edit = new QLineEdit(this);
   topRow->addWidget(edit, 1);
   QPushButton* go = new QPushButton(tr("Go"), this);
   go->setDefault(true);
   topRow->addWidget(go);
   root->addLayout(topRow);

   // -- middle: case sensitive + More button --
   QHBoxLayout* midRow = new QHBoxLayout();
   caseSensitive = new QCheckBox(tr("Case &Sensitive"), this);
   searchTitles  = new QCheckBox(tr("Search Only &Titles"), this);
   midRow->addWidget(caseSensitive);
   midRow->addWidget(searchTitles);
   midRow->addStretch(1);
   moreBtn = new QPushButton(tr("More >>>"), this);
   moreBtn->setCheckable(true);
   midRow->addWidget(moreBtn);
   root->addLayout(midRow);

   // -- collapsible: "Search in ..." radio group --
   moreBox = new QGroupBox(tr("Search in ..."), this);
   QHBoxLayout* moreLay = new QHBoxLayout(moreBox);
   rbWholeTree         = new QRadioButton(tr(".. &Whole tree"), moreBox);
   rbActiveAndChildren = new QRadioButton(tr(".. active Entry and &Children"), moreBox);
   rbActiveOnly        = new QRadioButton(tr(".. active &Entry only"), moreBox);
   rbWholeTree->setChecked(true);
   moreLay->addWidget(rbWholeTree);
   moreLay->addWidget(rbActiveAndChildren);
   moreLay->addWidget(rbActiveOnly);
   moreBox->hide();
   root->addWidget(moreBox);

   // -- results list --
   list = new QTreeWidget(this);
   list->setColumnCount(2);
   QStringList headers; headers << tr("Entry Name") << tr("Entry Content");
   list->setHeaderLabels(headers);
   list->setColumnWidth(0, 200);
   list->setColumnWidth(1, 400);
   list->setSortingEnabled(false);
   list->setRootIsDecorated(false);
   list->setItemDelegateForColumn(0, new SearchHighlightDelegate(list));
   list->setItemDelegateForColumn(1, new SearchHighlightDelegate(list));
   root->addWidget(list, 1);

   // -- status line --
   status = new QLabel("", this);
   root->addWidget(status);

   // Communication
   connect( edit, SIGNAL(returnPressed()), this, SLOT(startSearching()) );
   connect( go,   SIGNAL(clicked()),       this, SLOT(startSearching()) );
   connect( list, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(selectionChange(QTreeWidgetItem*)) );
   connect( moreBtn, SIGNAL(toggled(bool)), this, SLOT(toggleMore(bool)) );
}


void SearchDialog::toggleMore( bool bChecked )
{
   moreBox->setVisible(bChecked);
   moreBtn->setText( bChecked ? "More <<<" : "More >>>" );
}


int SearchDialog::setUp( CTreeElement* rootTreeElement_,
                         CTreeElement* activeTreeElement_ )
{
  this->rootTreeElement   = rootTreeElement_;
  this->activeTreeElement = activeTreeElement_;
  status->setText("");
  list->clear();

  if (size().width() < 650 || size().height() < 380)
     resize(650, 380);

  edit->selectAll();
  edit->setFocus();

  show();
  return exec();
}



void SearchDialog::startSearching( void )
{
  status->setText(tr("Searching ..."));
  list->clear();

  QString text = edit->text();
  if (text.length() == 0) return;
  int mode = whatMode();
  int nSkippedEncrypted = 0;

  if (mode == 0)
    rootTreeElement->search(edit->text(), true, caseSensitive->isChecked(),
                            searchTitles->isChecked(), *list, nSkippedEncrypted);
  else if (mode == 1)
    activeTreeElement->search(edit->text(), true, caseSensitive->isChecked(),
                              searchTitles->isChecked(), *list, nSkippedEncrypted);
  else if (mode == 2)
    activeTreeElement->search(edit->text(), false, caseSensitive->isChecked(),
                              searchTitles->isChecked(), *list, nSkippedEncrypted);

  int n = list->topLevelItemCount();
  QString s;
  if (n == 0)
    s = tr("<b>No match found.</b>");
  else if (n == 1)
    s = tr("<b>One match found.</b>");
  else
    s = tr("<b>%1 matches found.</b>").arg(n);

  if (nSkippedEncrypted > 0)
    s += " " + tr("(%n encrypted entry not scanned — open it once to include it)",
                  "", nSkippedEncrypted);

  status->setText(s);
}


int SearchDialog::whatMode( void )
{
   if (rbActiveAndChildren && rbActiveAndChildren->isChecked()) return 1;
   if (rbActiveOnly        && rbActiveOnly->isChecked())        return 2;
   return 0;  // whole tree (default)
}


QString SearchDialog::getText( void )
{ return edit->text(); }


void SearchDialog::selectionChange( QTreeWidgetItem* x )
{
  if (!x) return;
  SearchListItem* sli = dynamic_cast<SearchListItem*>(x);
  if (sli)
     emit makeVisible( sli->getSearchPosition() );
}

void SearchDialog::close( void )
{ reject(); }
