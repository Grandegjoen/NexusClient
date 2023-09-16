#include "treewidget.h"
#include <QDebug>

treewidget::treewidget()
{
    setStyleSheet("background-color: rgb(61, 56, 70); color: white;");
}

void treewidget::dropEvent(QDropEvent *event){
    // get the list of the items that are about to be dragged
    QList<QTreeWidgetItem*> items = selectedItems();
    QTreeWidgetItem *item = items.first();
    bool drop_ok = false;
    DropIndicatorPosition drop_indicator = dropIndicatorPosition();

    switch (drop_indicator){
    case QAbstractItemView::AboveItem:
        drop_ok = false;
        break;
    case QAbstractItemView::BelowItem:
        drop_ok = false;
        break;
    case QAbstractItemView::OnItem:
        drop_ok = true;
        break;
    case QAbstractItemView::OnViewport:
        if (item->parent() == nullptr)
            break;
        drop_ok = true;
        break;
    }

    if(drop_ok){
        QTreeWidget::dropEvent(event);
        QString parent = "nullptr";
        if (item->parent() != nullptr)
            parent = item->parent()->data(1, 0).toString();
        item->setData(2, 0, parent);
        item->treeWidget()->sortItems(0, Qt::AscendingOrder);
        emit note_item_dropped(item);
    }
}
