/***************************************************************************
   searchhighlightdelegate.h
   Item delegate for the search result tree: renders the matched
   substring of the "content" column in bold.
 ***************************************************************************/
#ifndef SEARCHHIGHLIGHTDELEGATE_H
#define SEARCHHIGHLIGHTDELEGATE_H

#include <QStyledItemDelegate>

class SearchHighlightDelegate : public QStyledItemDelegate
{
   Q_OBJECT
public:
   explicit SearchHighlightDelegate(QObject* parent = nullptr);

   void paint(QPainter* painter,
              const QStyleOptionViewItem& option,
              const QModelIndex& index) const override;
};

#endif
