#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItem>
#include <QVector>
#include "sslserver.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void display_file_tree();

private slots:
    void on_login_btn_clicked();
    void on_cell_clicked(const QModelIndex &index);
    void on_queue_cell_clicked(const QModelIndex &index);
    void update_server_files_table();
    void update_notes_tree();

private:
    Ui::MainWindow *ui;
    void load_config();
    void queued_files_tree();
    QString file_name;
    QString file_dir;
    QString queued_file_selected;
    QVector<QString> files_to_send;
    SSLServer *server;

};
#endif // MAINWINDOW_H
