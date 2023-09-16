#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include <QTreeWidget>
#include <QObject>
#include <QWidget>

class treewidget : public QTreeWidget
{
    Q_OBJECT
public:
    treewidget();
    void dropEvent(QDropEvent *event);

signals:
    void note_item_dropped(QTreeWidgetItem*);
};

#endif // TREEWIDGET_H
