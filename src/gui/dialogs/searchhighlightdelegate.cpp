#include "searchhighlightdelegate.h"

#include <QPainter>
#include <QApplication>
#include <QFontMetrics>

// data roles stamped by SearchListItem
static constexpr int MATCH_POS_ROLE = Qt::UserRole + 1;
static constexpr int MATCH_LEN_ROLE = Qt::UserRole + 2;

SearchHighlightDelegate::SearchHighlightDelegate(QObject* parent)
   : QStyledItemDelegate(parent)
{
}

void SearchHighlightDelegate::paint(QPainter* p,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
   QVariant vPos = index.data(MATCH_POS_ROLE);
   QVariant vLen = index.data(MATCH_LEN_ROLE);

   // No highlight info — fall back to default rendering.
   if (!vPos.isValid() || !vLen.isValid()) {
      QStyledItemDelegate::paint(p, option, index);
      return;
   }

   const int pos = vPos.toInt();
   const int len = vLen.toInt();
   const QString text = index.data(Qt::DisplayRole).toString();

   QStyleOptionViewItem opt = option;
   initStyleOption(&opt, index);
   opt.text.clear();  // we draw the text ourselves
   QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
   style->drawControl(QStyle::CE_ItemViewItem, &opt, p, opt.widget);

   QRect r = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
   p->save();
   p->setClipRect(r);

   const QString before = pos > 0 ? text.left(pos) : QString();
   const QString match  = (pos >= 0 && pos < text.size())
                             ? text.mid(pos, len) : QString();
   const QString after  = (pos + len < text.size())
                             ? text.mid(pos + len) : QString();

   QFont normalFont = opt.font;
   QFont boldFont   = normalFont;
   boldFont.setBold(true);
   QFontMetrics fmNormal(normalFont);
   QFontMetrics fmBold(boldFont);

   int x = r.left() + 2;
   const int y = r.center().y() + (fmNormal.ascent() - fmNormal.descent()) / 2;

   const bool sel = (opt.state & QStyle::State_Selected);
   p->setPen(sel ? opt.palette.highlightedText().color()
                 : opt.palette.text().color());

   if (!before.isEmpty()) {
      p->setFont(normalFont);
      p->drawText(x, y, before);
      x += fmNormal.horizontalAdvance(before);
   }
   if (!match.isEmpty()) {
      p->setFont(boldFont);
      p->drawText(x, y, match);
      x += fmBold.horizontalAdvance(match) + 1;
   }
   if (!after.isEmpty()) {
      p->setFont(normalFont);
      p->drawText(x, y, after);
   }
   p->restore();
}
