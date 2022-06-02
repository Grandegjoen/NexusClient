#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    server = new SSLServer(this);
    display_file_tree();
    queued_files_tree();

    // Signals and slots
    connect(server, SIGNAL(files_updated()), this, SLOT(update_server_files_table())); // Updates/loads file tree when signal is received from server.
    connect(server, SIGNAL(notes_updated()), this, SLOT(update_notes_tree())); // Updates/loads note tree when signal is received from server.
    server->connect_to_server();

    // Sends server request to retrieve file info
//    server->get_server_fileinfo("/");

}

// Fixes server file table with name, size, date added and eventual comment
void MainWindow::display_file_tree(){
    // Creates model
    QStandardItemModel *model = new QStandardItemModel();
    model->clear();
    ui->server_files_table->setModel(model);

    // Add column titles
    model->setHorizontalHeaderItem(0, new QStandardItem("Name"));
    model->setHorizontalHeaderItem(1, new QStandardItem("Size"));
    model->setHorizontalHeaderItem(2, new QStandardItem("Date added"));
    model->setHorizontalHeaderItem(3, new QStandardItem("Comment"));


    // Load server file information
    // Display files in tableview using loop
    for (int i = 0; i < server->server_files.size(); ++i){
        QList <QStandardItem *> file;

        // Fix algorithm here
        file.append(new QStandardItem("test1"));
        file.append(new QStandardItem("test2"));
        file.append(new QStandardItem("test3test3test3test3test3test3"));
        file.append(new QStandardItem("test4"));


        model->appendRow(file);
    }

    // Resizing and such...
    ui->server_files_table->resizeColumnsToContents();
    ui->server_files_table->verticalHeader()->hide();
    ui->server_files_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->server_files_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->server_files_table, SIGNAL(clicked(const QModelIndex&)), this, SLOT(on_cell_clicked(const QModelIndex&)));
}

// Files ready to be sent.
void MainWindow::queued_files_tree(){
    QStandardItemModel *model = new QStandardItemModel();
    model->clear();
    ui->queued_files_table->setModel(model);

    // Configure column titles
    model->setHorizontalHeaderItem(0, new QStandardItem("Name"));
    model->setHorizontalHeaderItem(1, new QStandardItem("Size"));

    // Display files in tableview using loop
    for (int i = 0; i < files_to_send.size(); ++i){
        QList <QStandardItem *> file;

        // Fix algorithm here
        file.append(new QStandardItem("Notes.txt"));
        file.append(new QStandardItem("4 kb"));


        model->appendRow(file);
    }

    // Resizing and such
    ui->queued_files_table->resizeColumnsToContents();
    ui->queued_files_table->verticalHeader()->hide();
    ui->queued_files_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->queued_files_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    connect(ui->queued_files_table, SIGNAL(clicked(const QModelIndex&)), this, SLOT(on_queue_cell_clicked(const QModelIndex&)));
}


void MainWindow::on_cell_clicked(const QModelIndex &index){
    // Get filename and path, store it to temp variables if it's a file
    // Else change directory to dir clicked and update file tree
    file_name = index.siblingAtColumn(0).data(0).toString();
}

void MainWindow::on_queue_cell_clicked(const QModelIndex &index){
    // Get filename and path, store it to temp variables if it's a file
    // Else change directory to dir clicked and update file tree
    queued_file_selected = index.siblingAtColumn(0).data(0).toString();

}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_login_btn_clicked() {
    ui->main_stacked->setCurrentIndex(1);
    server->test_shit();
}

void MainWindow::update_server_files_table(){
//    display_file_tree();
    qDebug() << "SERVER FILES UPDATEDOKR3JFIGADNGIOAGHIOGSFNIUDFSGPIUBDFJTYI9POGLHBFHCGHÅPDXIOPØB!";
}

void MainWindow::update_notes_tree(){
    qDebug() << "NOTES UPDATEDOKR3JFIGADNGIOAGHIOGSFNIUDFSGPIUBDFJTYI9POGLHBFHCGHÅPDXIOPØB!";

}
