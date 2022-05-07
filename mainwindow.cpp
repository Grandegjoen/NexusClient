#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // Tree view experiment


}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_login_btn_clicked() {
    ui->main_stacked->setCurrentIndex(1);
}

