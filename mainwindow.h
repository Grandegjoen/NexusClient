#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include "filetransfer.h"
#include "treewidget.h"
#include "sslserver.h"
#include "hasher.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool note_list_synced;

    // Note related
    QString current_note;
    QVector <QString> notes_to_sync;
    QVector <FileTransfer*> active_files;
    bool current_note_synced = true;
    bool new_note = false;
    void sync_note(QString, QString, bool clear_text = true);
    void sync_if_unsynced();
    QTreeWidgetItem *current_item;
    QTreeWidgetItem *previous_item;
    QTreeWidgetItem *right_clicked_item;
    QList<QTreeWidgetItem*> recursive_list_notes_to_delete(QTreeWidgetItem*);
    bool updating_note_list = false; // Need to use this when changing multiple notelist values fast to prevent crash...

public slots:
    void login_result(bool success);
    void add_notes_to_list(QString);
    void note_list_updated(QTreeWidgetItem*);
    void note_tree_changed(QTreeWidgetItem*);
    void note_ready(QString);
    void note_clicked(QTreeWidgetItem*);
    void tree_widget_pwm_item_clicked(QTreeWidgetItem*);
    void tree_widget_ftp_item_clicked(QTreeWidgetItem*);
    void text_changed();
    void get_items_on_notelist();
    void no_note();
    void prepare_menu(QPoint);
    void prepare_menu_pwm(QPoint);
    void prepare_menu_ftp(QPoint);
    void add_passwords_to_pwm(QFile*);
    void add_files_to_ftp(QFile*);
    void file_sent(FileTransfer*);
    void file_received(FileTransfer*);
    void server_connected(bool);

private slots:
    void on_login_btn_clicked();
    void on_new_note_clicked();
    void on_new_folder_clicked();

    // QAction functions
    void action_delete_item();
    void action_rename_item();
    void action_password();
    void action_delete_pwm_item();

    // Button slots
    void notes_btn_pressed();
    void pwm_btn_pressed();
    void files_btn_pressed();
    void test_btn_pressed();
    void settings_btn_pressed();
    void on_new_pwm_item_clicked();
    void on_delete_pwm_item_clicked();
    void on_pwm_settings_clicked();
    void on_password_copy_clicked();
    void on_password_lock_clicked();
    void on_settings_btn_clicked();
    void on_password_clicked();
    void on_delete_note_btn_clicked();
    void on_close_settings_clicked();
    void on_save_settings_clicked();
    void on_send_file_btn_clicked();
    void on_receive_file_btn_clicked();
    void on_delete_file_btn_clicked();
    void on_open_priv_key_btn_clicked();
    void on_open_loc_cert_btn_clicked();
    void on_open_ca_certs_btn_clicked();

    void on_choose_destination_btn_clicked();

private:
    QTreeWidget *tree_widget;
    QTreeWidget *tree_widget_pwm;
    QTreeWidget *tree_widget_ftp;
    SslServer *server;
    void connect_signals();
    Ui::MainWindow *ui;
    void display_note_list();
    void initialize_pwm();
    void initialize_ftp();
    void add_pwm_item(QString, QString, QString);
    QString current_note_password;
    QIcon icon;
    QPixmap folder_icon;
    QPixmap note_icon;
    QPixmap locked_note_icon;
    QMessageBox message_box;
    int previous_index;
    bool logged_in = false;

    void prepare_to_receive_file(QTreeWidgetItem*, QString user_key);
    void load_from_settings(bool launching = false);

    struct note_password_combo{
        QString note_hash;
        QString note_password;
    };
    note_password_combo note_verification;
};
#endif // MAINWINDOW_H
