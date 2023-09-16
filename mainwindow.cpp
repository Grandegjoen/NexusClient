#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){
    ui->setupUi(this);
    ui->verticalLayout->setEnabled(false);
    server = new SslServer(this);
    load_from_settings(true);
    display_note_list();
    initialize_pwm();
    initialize_ftp();
    connect_signals();
    server->connect_to_server();
    folder_icon.load("icons/folder.png");
    note_icon.load("icons/note1.png");
    locked_note_icon.load("icons/locked_icon.png");
    message_box.setText("Invalid password.");
    setStyleSheet("QInputDialog {color: red;};");
    ui->statusBar->setStyleSheet("background-color: rgb(33, 28, 46);");
}


MainWindow::~MainWindow(){
    delete ui;
}


void MainWindow::connect_signals(){
    connect(server, SIGNAL(server_connected(bool)), this, SLOT(login_result(bool)));
    connect(server, SIGNAL(server_is_connected(bool)), this, SLOT(server_connected(bool)));
    connect(server, SIGNAL(notelist_ready(QString)), this, SLOT(add_notes_to_list(QString)));
    connect(server, SIGNAL(note_ready(QString)), this, SLOT(note_ready(QString)));
    connect(server, SIGNAL(no_note()), this, SLOT(no_note()));
    connect(server, SIGNAL(pwm_list_ready(QFile*)), this, SLOT(add_passwords_to_pwm(QFile*)));
    connect(server, SIGNAL(ftp_list_ready(QFile*)), this, SLOT(add_files_to_ftp(QFile*)));
    connect(tree_widget, SIGNAL(note_item_dropped(QTreeWidgetItem*)), this, SLOT(note_list_updated(QTreeWidgetItem*)));
    connect(tree_widget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(note_clicked(QTreeWidgetItem*)));
    connect(tree_widget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(note_tree_changed(QTreeWidgetItem*)));
    connect(ui->textEdit, SIGNAL(textChanged()), this, SLOT(text_changed()));
    connect(tree_widget_pwm, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(tree_widget_pwm_item_clicked(QTreeWidgetItem*)));
    connect(tree_widget_ftp, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(tree_widget_ftp_item_clicked(QTreeWidgetItem*)));
}


void MainWindow::text_changed(){
    if (!new_note)
        current_note_synced = false;
    else
        new_note = false;
}


void MainWindow::server_connected(bool connected){
    qDebug() << "Server connection slot reached!";
    ui->connection_label->setText("Connected");
    ui->connection_label->setStyleSheet("background-color: green;");
}


void MainWindow::note_tree_changed(QTreeWidgetItem* item){
    if (updating_note_list)
        return;
    tree_widget->sortItems(0, Qt::AscendingOrder);
    note_list_synced = false;
    get_items_on_notelist();
}


void MainWindow::note_clicked(QTreeWidgetItem* item){
//    sync_if_unsynced();
    qDebug() << server->command_stack.size();
    qDebug() << item->data(5, 0).toString();
    if (item->data(4, 0).toString() != "false"){
        bool ok;
        QString password = QInputDialog::getText(this, "Unlock Note", "Enter Password", QLineEdit::EchoMode(QLineEdit::Password), "", &ok);
        if (!ok) return;
        if (item->data(5, 0).toString() != password_hasher(server->username, password)){
            return;
        }
        note_verification.note_hash = item->data(1, 0).toString();
        note_verification.note_password = password;
    }

    if (item == current_item || item->data(3, 0).toString() == "Folder"){
        qDebug() << "Previous item ...";
        return;
    };
    ui->textEdit->setDisabled(true);
    sync_if_unsynced();
    current_note = item->data(1, 0).toString();
    server->current_note = current_note;
    server->request_note(current_note);
}


void MainWindow::sync_if_unsynced(){
    if (!current_note_synced && current_item != nullptr){
        qDebug() << "On syncing if unsynced!";
        qDebug() << current_item;
        sync_note(current_note, current_item->data(4, 0).toString());
        server->notes_to_send.append(current_note);
        current_note_synced = true;
        server->send_note();
    }
}


void MainWindow::sync_note(QString note_name, QString password_protected, bool clear_text){
    QFile file("data/" + note_name);
    file.open(QIODevice::WriteOnly);
    QByteArray note;
    QString text = ui->textEdit->toPlainText();
    qDebug() << "Syncing note$> -----  " << password_protected;
    if (password_protected == "true"){
        qDebug() << "Current note password: " << current_note_password;
        note = note_encrypter_password(&text, current_note_password);
        current_note_password = "";
    }
    else {
        note = note_encrypter_no_password(&text, server->username);
        current_note_password = "";
    }
    if (clear_text){
        ui->textEdit->clear();
    }
    file.write(note);
    file.close();
    qDebug() << "Written text to file";
}


void MainWindow::note_ready(QString note){
    qDebug() << "Note ready!";
    QFile file(note);
    file.open(QIODevice::ReadOnly);
    new_note = true;
    QByteArray encrypted_data;
    encrypted_data = file.readAll();
    if (tree_widget->currentItem()->data(4, 0).toString() == "true"){
        QString note = note_decrypter(&encrypted_data, note_verification.note_password);
        if (note == "A02MA9D29P@$]MDA£@$]}¡@INVALID_P4$$W@RD¡@$[@¡"){
            qDebug() << "Something went wrong note_ready. Note unable to decrypt. Error 489";
            ui->textEdit->setText("");
            ui->textEdit->setDisabled(true);
            current_note = "";
            server->current_note = "";
            tree_widget->setCurrentItem(nullptr);
            server->announce_data_received();
            message_box.exec();
            previous_item = nullptr;
            current_item = nullptr;
            return;
        } else {
            current_note_password = note_verification.note_password;
        }
        ui->textEdit->setText(note);
        ui->textEdit->setEnabled(true);
        file.remove();
        server->announce_data_received();
    }
    else {
        ui->textEdit->setText(note_decrypter(&encrypted_data, server->username));
        ui->textEdit->setEnabled(true);
        file.remove();
        server->announce_data_received();
    }
    qDebug() << "Made it here 1";
    if (tree_widget->currentItem()->data(1, 0).toString() == current_note)
        current_item = tree_widget->currentItem();
    else {
        qDebug() << "Something has gone very wrong!";
        exit(143);
    }
    qDebug() << "Made it here 2";
}


void MainWindow::note_list_updated(QTreeWidgetItem* item){
    if (current_item == item){
        qDebug() << "Current item equals item!";
        item->setSelected(true);
    } else {
        current_item->setSelected(true);

    }
    note_list_synced = false;
}


void MainWindow::on_login_btn_clicked(){ // Login button...
    QString user_n = ui->username_field->text();
    QString pass_w = password_hasher(user_n, ui->password_field->text());
    server->login_to_server(user_n, pass_w);
    server->username = user_n;
}


void MainWindow::display_note_list(){
    // Sets up flags
    tree_widget = new treewidget();
    ui->verticalLayout_4->insertWidget(1, tree_widget);
    tree_widget->setDragEnabled(true);
    tree_widget->setAcceptDrops(true);
    tree_widget->setDefaultDropAction(Qt::MoveAction);
    tree_widget->sortItems(0, Qt::AscendingOrder);
    tree_widget->setHeaderHidden(true);
    tree_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_widget->setSelectionBehavior(QAbstractItemView::SelectItems);
    tree_widget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->textEdit->setDisabled(true);
    connect(tree_widget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(prepare_menu(QPoint)));
}


void MainWindow::prepare_menu(QPoint point){
    right_clicked_item = tree_widget->itemAt(point);
    if (right_clicked_item == nullptr)
        return;
    QMenu *menu = new QMenu("Menuu", tree_widget);
    QModelIndex index = tree_widget->indexAt(point);
    QString password_protected = "Remove Password";
    qDebug() << right_clicked_item->data(4,0).toString();
    if (right_clicked_item->data(4,0).toString() == "false")
        password_protected = "Set Password";
    if (index.isValid()){
        qDebug() << "Valid!";
        QAction *delete_action = new QAction("Delete", this);
        QAction *rename_action = new QAction("Rename", this);
        if (right_clicked_item->data(3,0).toString() != "Folder" && right_clicked_item->data(4,0).toString() == "false"){
            qDebug() << "If: " << right_clicked_item->data(4,0).toString();
            QAction *password_action = new QAction(password_protected, this);
            connect(password_action, SIGNAL(triggered()), this, SLOT(action_password()));
            menu->addAction(password_action);
        } else {
            qDebug() << "Else: " << right_clicked_item->data(4,0).toString();
        }
        connect(delete_action, SIGNAL(triggered()), this, SLOT(action_delete_item()));
        connect(rename_action, SIGNAL(triggered()), this, SLOT(action_rename_item()));
        menu->addAction(delete_action);
        menu->addAction(rename_action);
        menu->exec((QCursor::pos()));
    }
    if (current_item != nullptr)
        tree_widget->setCurrentItem(current_item);
    else
        tree_widget->setCurrentItem(nullptr);
    delete menu;
}


void MainWindow::action_delete_item(){
    if (right_clicked_item == nullptr)
        return;
    QString message = "server_delete_note|";
    // Check if folder, and if so delete all children and append to message
    QList<QTreeWidgetItem*> item_list = tree_widget->findItems("", Qt::MatchContains|Qt::MatchRecursive, 0);
    QTreeWidgetItem *item_to_delete;
    int index = 0;
    foreach(QTreeWidgetItem* x, item_list){
        if (x == right_clicked_item){
            item_to_delete = x;
            message += x->data(1, 0).toString();
            qDebug() << "Item to delete found!";
            break;
        }
        index += 1;
    }
    QMessageBox dialog;
    bool children = false;
    if (item_to_delete->childCount() != 0){
        children = true;
        dialog.setText("Do you wish to delete this folder?");
        dialog.setInformativeText("All children of this folder will be deleted as well.");
    } else {
        if (right_clicked_item->data(3, 0).toString() == "Folder")
            dialog.setText("Do you wish to delete this folder?");
        else
            dialog.setText("Do you wish to delete this note?");
        dialog.setInformativeText("You cannot undo this action later on.");
    }
    QPushButton *btn = dialog.addButton(tr("Delete"), QMessageBox::ActionRole);
    dialog.setStandardButtons(QMessageBox::Cancel);
    dialog.setDefaultButton(QMessageBox::Cancel);
    int result = dialog.exec();
    if (result != 0)
        return;
    message += "|N-E-X";
    if (!children)
        server->send_delete_note(message);
    else {
        QList<QTreeWidgetItem*> item_list = recursive_list_notes_to_delete(item_to_delete);
        item_list.append(item_to_delete);
        for (auto &x : item_list){
            if (x->data(3,0).toString() == "folder")
                continue;
            QString message = "server_delete_note|" + x->data(1, 0).toString() + "|N-E-X";
            server->send_delete_note(message);
            qDebug() << x->data(1,0).toString();
        }
    }
    if (right_clicked_item == current_item){
        delete item_to_delete;
        tree_widget->setCurrentItem(nullptr);
        ui->textEdit->clear();
        ui->textEdit->setDisabled(true);
        get_items_on_notelist();
        current_item = nullptr;
        previous_item = nullptr;
        qDebug() << "Deleting item!";
    } else {
        delete item_to_delete;
        qDebug() << "Deleting item!";


    }

}


QList<QTreeWidgetItem*> MainWindow::recursive_list_notes_to_delete(QTreeWidgetItem* source){
    QList<QTreeWidgetItem*> item_list = source->takeChildren();
    for (auto &x : item_list){
        if (x->childCount() != 0)
            item_list.append(recursive_list_notes_to_delete(x));
    }

    return item_list;

}


void MainWindow::action_rename_item(){
    QString item_name = right_clicked_item->data(0, 0).toString();
    //check if new_name returns valid input?
    QString new_name = QInputDialog::getText(this, "Rename Item", "Name", QLineEdit::Normal, item_name);
    if (new_name == item_name)
        return;
    right_clicked_item->setText(0, new_name);
    note_list_synced = false;
    get_items_on_notelist();
    qDebug() << "Renaming item!";
}


void MainWindow::action_password(){
    if (right_clicked_item == nullptr)
        return;
    if (right_clicked_item->data(3, 0).toString() == "Folder")
        return;
    QString password_protected = right_clicked_item->data(4,0).toString();
    if (password_protected == "false"){
        bool ok;
        QString password1 = QInputDialog::getText(this, "Set Password", "Set Desired Password", QLineEdit::EchoMode(QLineEdit::Password), "", &ok);
        if (!ok) return;
        QString password2 = QInputDialog::getText(this, "Set Password", "Repeat Desired Password", QLineEdit::EchoMode(QLineEdit::Password),  "", &ok);
        if (!ok) return;
        qDebug() << "Comparing pw1 and pw2";
        if (password1 == password2){
            updating_note_list = true;
            icon.addPixmap(locked_note_icon);
            right_clicked_item->setData(4, 0, "true");
            right_clicked_item->setData(5, 0, password_hasher(server->username, password1));

            right_clicked_item->setIcon(0, icon);
            updating_note_list = false;

            if (current_item == right_clicked_item){
                qDebug() << "Current item equals right clicked item";
                current_note_password = password1;
                // NEW FROM HERE
                // NEW TILL HERE
                sync_note(current_note, "true", false);
                server->notes_to_send.append(current_note);
                current_note_synced = true;
                server->send_note();
                current_note_password = password1;
            } else {
                qDebug() << "Current item DOES NOT equal right clicked item";
                server->send_encryption_request(right_clicked_item->data(1,0).toString(), password1, password_hasher(server->username, password1));
            }
            qDebug() << "Getting items on notelist...23";
            get_items_on_notelist();

        } else {
            QMessageBox message;
            message.setText("Passwords don't match.");
            message.exec();
        }
    }
    else {
        QMessageBox dialog;
        dialog.setText("Do you wish to remove the password from this note?");
        QPushButton *btn = dialog.addButton(tr("Remove Password"), QMessageBox::ActionRole);
        dialog.setStandardButtons(QMessageBox::Cancel);
        dialog.setDefaultButton(QMessageBox::Cancel);
        int result = dialog.exec();
        if (result != 0)
            qDebug() << "Exec 0!";
        else{
            right_clicked_item->setData(4, 0, "false");
            current_note_synced = false;
            sync_note(current_note, right_clicked_item->data(4, 0).toString(), false);
            icon.addPixmap(note_icon);
            right_clicked_item->setIcon(0, icon);
        }
    }
    current_item->setSelected(true);
}


void MainWindow::on_new_note_clicked(){ // What happens when new note button is pressed
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setData(1, 0, hash_generator());
    item->setData(2, 0, "nullptr");
    item->setData(3, 0, "Note");
    item->setData(4, 0, "false");
    item->setText(0, "New Note");
//    QIcon icon(note_icon);
//    item->setIcon(0, icon);
    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    tree_widget->addTopLevelItem(item);
    tree_widget->sortItems(0, Qt::AscendingOrder);
    note_list_synced = false;
    qDebug() << "Getting items on notelist";
    get_items_on_notelist();
    qDebug() << "Syncing if unsynced";
    sync_if_unsynced();
    qDebug() << "End of new note clicked";
//    previous_item = item;
//    tree_widget->setCurrentItem(item);
//    current_item = item;
//    ui->textEdit->setEnabled(true);
}


void MainWindow::on_new_folder_clicked(){ // What happens when new folder button is pressed
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setData(1, 0, hash_generator());
    item->setData(2, 0, "nullptr");
    item->setData(3, 0, "Folder");
    item->setData(4, 0, "false");
    item->setText(0, "New Folder");
    QIcon icon(folder_icon);
    item->setIcon(0, icon);
    item->setFlags(item->flags() | Qt::ItemIsDropEnabled);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    tree_widget->addTopLevelItem(item);
    tree_widget->sortItems(0, Qt::AscendingOrder);
    note_list_synced = false;
    get_items_on_notelist();
}


void MainWindow::add_notes_to_list(QString path){
    QFile note_list(path);
    qDebug() << path;
    note_list.open(QIODevice::ReadOnly);
    qDebug() << note_list.size();
    QTextStream stream(&note_list);
    while (!stream.atEnd()){
        QString line = stream.readLine();
        QTreeWidgetItem *item = new QTreeWidgetItem();
        QString item_name = line.split('|').at(0);
        QString item_hash = line.split('|').at(1);
        QString parent = line.split('|').at(2);
        QString type = line.split('|').at(3);
        QString password_protected = line.split('|').at(6);
        QString password_hash = line.split('|').at(7);
        if (password_hash.isEmpty())
            password_hash = "NOHASH";
        if (type == "Folder"){
            icon.addPixmap(folder_icon);
        }
        else if (password_protected == "false"){
            icon.addPixmap(note_icon);
        }
        else {
            icon.addPixmap(locked_note_icon);
        }
        item->setIcon(0, icon);
        item->setData(1, 0, item_hash);
        item->setData(2, 0, parent);
        item->setData(3, 0, type);
        item->setData(4, 0, password_protected);
        item->setData(5, 0, password_hash);
        item->setText(0, item_name);
        if (type != "Folder")
            item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
        else
            item->setFlags(item->flags() | Qt::ItemIsDropEnabled);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        if (parent == "nullptr")
            tree_widget->addTopLevelItem(item);
        else { // There's probably a much more efficient way of doing this... Oh well...
            QList<QTreeWidgetItem*> item_list = tree_widget->findItems("", Qt::MatchContains|Qt::MatchRecursive, 0);
            foreach(QTreeWidgetItem* x, item_list){
                if (x->data(1, 0).toString() == parent){
                    x->addChild(item);
                    continue;
                }
            }
        }
    }
    tree_widget->sortItems(0, Qt::AscendingOrder);
    server->announce_data_received();
}


void MainWindow::get_items_on_notelist(){
    qDebug() << "Getting items on note list...";
    QFile note_list("data/temp_n_l.nex");
    if (note_list.exists())
        note_list.remove();
    note_list.open(QIODevice::Append);
    int index = 0;
    QList<QTreeWidgetItem*> item_list = tree_widget->findItems("", Qt::MatchContains|Qt::MatchRecursive, 0);
    foreach(QTreeWidgetItem* x, item_list){
        QString item_description;
        QString parent_hash = "nullptr";
        if (x->parent() != nullptr)
            parent_hash = x->parent()->data(1, 0).toString();
        item_description = x->data(0, 0).toString() + "|" + x->data(1, 0).toString() + "|" + parent_hash + "|" +
                x->data(3, 0).toString() + "|" + x->data(4,0).toString() + "|";
        item_description += server->username + "|" + QString::number(index) + "|" + x->data(5,0).toString() + "|\n";
        index += 1;
        note_list.write(item_description.toUtf8());
    }
    note_list.close();
    server->send_notelist();
}


void MainWindow::no_note(){
    qDebug() << "No note";
    new_note = true;
    ui->textEdit->clear();
    ui->textEdit->setEnabled(true);
    current_item = tree_widget->currentItem();
    server->announce_data_received();
}


void MainWindow::login_result(bool success){
    if (success){
        logged_in = true;
        ui->stackedWidget->setCurrentIndex(1);
        server->request_note_list();
        server->request_pwm_list();
        server->request_ftp_list();
        server->encryption_key = create_encryption_key(server->username, server->unique_user_hash);
        QPushButton *button1 = new QPushButton("Notes");
        QPushButton *button2 = new QPushButton("PWM");
        QPushButton *button3 = new QPushButton("FT");
        QPushButton *button4 = new QPushButton("TS");
        QPushButton *button5 = new QPushButton("Settings");
        button1->setMinimumSize(75, 75);
        button2->setMinimumSize(75, 75);
        button3->setMinimumSize(75, 75);
        button4->setMinimumSize(75, 75);
        button5->setMinimumSize(75, 75);
        //        ui->verticalLayout->addStretch(1);
        ui->verticalLayout->insertWidget(0, button1);
        ui->verticalLayout->insertWidget(1, button2);
        ui->verticalLayout->insertWidget(2, button3);
        ui->verticalLayout->insertWidget(3, button4);
        ui->verticalLayout->insertWidget(4, button5);
        ui->verticalLayout->addStretch(5);
        connect(button1, SIGNAL(clicked()), this, SLOT(notes_btn_pressed()));
        connect(button2, SIGNAL(clicked()), this, SLOT(pwm_btn_pressed()));
        connect(button3, SIGNAL(clicked()), this, SLOT(files_btn_pressed()));
        connect(button4, SIGNAL(clicked()), this, SLOT(test_btn_pressed()));
        connect(button5, SIGNAL(clicked()), this, SLOT(settings_btn_pressed()));
    }
    else
        qDebug() << "Login failed";
}


// ******************************* Password Manager Related Functions *******************************
void MainWindow::initialize_pwm(){
    tree_widget_pwm = new QTreeWidget(this);
    tree_widget_pwm->setStyleSheet("background-color: rgb(61, 56, 70); color: white;");
    tree_widget_pwm->setColumnCount(2);
    tree_widget_pwm->headerItem()->setText(0, "Domain");
    tree_widget_pwm->headerItem()->setText(1, "Username");
    tree_widget_pwm->sortItems(0, Qt::AscendingOrder);
    tree_widget_pwm->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    tree_widget_pwm->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->horizontalLayout_6->insertWidget(0, tree_widget_pwm);
    connect(tree_widget_pwm, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(prepare_menu_pwm(QPoint)));
}


void MainWindow::prepare_menu_pwm(QPoint point){
    qDebug() << "FDAIFAIDSFA";
    right_clicked_item = tree_widget_pwm->itemAt(point);
    if (right_clicked_item == nullptr)
        return;
    QMenu *menu = new QMenu("Menuu", tree_widget_pwm);
    QModelIndex index = tree_widget_pwm->indexAt(point);
    if (index.isValid()){
        QAction *delete_action = new QAction("Delete", this);
        connect(delete_action, SIGNAL(triggered()), this, SLOT(action_delete_pwm_item()));
        menu->addAction(delete_action);
        menu->exec((QCursor::pos()));
    }
    delete menu;
}


void MainWindow::action_delete_pwm_item(){
    on_delete_pwm_item_clicked();
}


void MainWindow::tree_widget_pwm_item_clicked(QTreeWidgetItem* item){
    if (item->data(3, 0).toString() == "true"){
        ui->password_display->setText("**************************************************");
        ui->password_lock->setText("Unlock");
    }
    else{
        ui->password_display->setText(aes_decryptor(item->data(2,0).toByteArray(), server->username.toLower()));
        ui->password_lock->setText("Lock");
    }
    qDebug() << item->data(2, 0).toByteArray();
}


void MainWindow::add_passwords_to_pwm(QFile *file){ // Slot
    file->open(QIODevice::ReadOnly);
    while(!file->atEnd()){
        QByteArray line = file->readLine();
        if (line.length() <= 5) {qDebug() << "Less than five, continue!"; continue; }
        QTreeWidgetItem *item = new QTreeWidgetItem();
        QString unique_hash = line.split('|').at(0);
        QString domain = line.split('|').at(1);
        QString username = line.split('|').at(2);
        QString password_protected = line.split('|').at(3);
        QString owner = line.split('|').at(4);
        int begin_length = unique_hash.length() + domain.length() + username.length() + password_protected.length() + owner.length() + 5;
        QByteArray password = line.mid(begin_length, line.length() - begin_length - 2); // 2 being newline "\n"
        qDebug() << "Password belonging to " << username << " is: " << password;
        item->setData(0, 0, domain);
        item->setData(1, 0, username);
        item->setData(2, 0, password);
        item->setData(3, 0, password_protected);
        item->setData(4, 0, unique_hash);
        tree_widget_pwm->addTopLevelItem(item);
    }
    server->announce_data_received();
    tree_widget_pwm->sortItems(0, Qt::AscendingOrder);
    file->remove();
}


void MainWindow::on_new_pwm_item_clicked(){
    bool valid_input = false;
    QDialog *dialog = new QDialog(this);
    QFormLayout form(dialog);
    form.addRow(new QLabel("Add new object to password manager"));
    QLineEdit *line_edit1 = new QLineEdit(dialog);
    QLineEdit *line_edit2 = new QLineEdit(dialog);
    QLineEdit *line_edit3 = new QLineEdit(dialog);
    QLineEdit *line_edit4 = new QLineEdit(dialog);
    line_edit3->setEchoMode(QLineEdit::EchoMode(QLineEdit::Password));
    line_edit4->setEchoMode(QLineEdit::EchoMode(QLineEdit::Password));

    form.addRow("Domain", line_edit1);
    form.addRow("Username", line_edit2);
    form.addRow("Password", line_edit3);
    form.addRow("Confirm Password", line_edit4);
    QDialogButtonBox button_box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                Qt::Horizontal, dialog);
    form.addRow(&button_box);
    QObject::connect(&button_box, SIGNAL(accepted()), dialog, SLOT(accept()));
    QObject::connect(&button_box, SIGNAL(rejected()), dialog, SLOT(reject()));

    if (dialog->exec() == QDialog::Accepted){
        if (line_edit1->text().size() > 0 && line_edit2->text().size() > 0 && line_edit3->text().size() > 0
                && line_edit3->text() == line_edit4->text()){
            add_pwm_item(line_edit1->text(), line_edit2->text(), line_edit3->text());
        } else {
            QMessageBox message;
            message.setText("Passwords don't match.");
            message.exec();
        }
    }
}


void MainWindow::add_pwm_item(QString domain, QString username, QString password){
    QTreeWidgetItem *item = new QTreeWidgetItem();
    QByteArray encrypted_password = aes_encryptor(password, server->username.toLower());
    qDebug() << "ENCRYPTED PASSWORD IS AS FOLLOWS: " << encrypted_password;
    qDebug() << domain << " | " << username << " | " << password;
    item->setData(0, 0, domain);
    item->setData(1, 0, username);
    item->setData(2, 0, encrypted_password);
    item->setData(3, 0, "false");
    item->setData(4, 0, hash_generator());
    tree_widget_pwm->addTopLevelItem(item);
    QByteArray message = "server_add_pwm|" + item->data(4,0).toByteArray() + "|" + domain.toUtf8() + "|"
            + username.toUtf8() + "|false|" + encrypted_password + "|N-E-X";
    server->password_manager_update(message);
}


void MainWindow::on_delete_pwm_item_clicked(){
    if (tree_widget_pwm->currentItem() == nullptr)
        return;
    QByteArray message = "server_remove_pwm|" + tree_widget_pwm->currentItem()->data(4,0).toByteArray() + "|N-E-X";
    server->password_manager_update(message);
    QList<QTreeWidgetItem*> item_list = tree_widget_pwm->findItems("", Qt::MatchContains|Qt::MatchRecursive, 0);
    foreach(QTreeWidgetItem* x, item_list){
        if (tree_widget_pwm->currentItem() == x){
            delete x;
            break;
        }
    }
    ui->password_display->setText("");
    server->password_manager_update(message);
}


void MainWindow::on_pwm_settings_clicked(){

}


void MainWindow::on_password_lock_clicked(){
    QTreeWidgetItem *item = tree_widget_pwm->currentItem();
    QByteArray message = "server_update_pwm|" + item->data(4,0).toByteArray() + "|";

    if (item->data(3, 0).toString() == "false"){
        bool ok;
        QString password1 = QInputDialog::getText(this, "Set Password", "Set Desired Password", QLineEdit::EchoMode(QLineEdit::Password), "", &ok);
        if (!ok) return;
        QString password2 = QInputDialog::getText(this, "Set Password", "Repeat Desired Password", QLineEdit::EchoMode(QLineEdit::Password),  "", &ok);
        if (!ok) return;
        qDebug() << password1;
        if (password1 == password2){
            QByteArray encrypted_password = aes_encryptor(ui->password_display->text(), password1);
            item->setData(2, 0, encrypted_password);
            item->setData(3, 0, "true");
            ui->password_display->setText("**************************************************");
        } else {
            QMessageBox message;
            message.setText("Passwords don't match.");
            message.exec();
            return;
        }
    }
    else {
        bool ok;
        QString password = QInputDialog::getText(this, "Unlock Password", "Enter the Password's Password", QLineEdit::EchoMode(QLineEdit::Password), "", &ok);
        if (!ok) return;
        QString result = aes_decryptor(item->data(2, 0).toByteArray(), password);
        if (result == "A02MA9D29P@$]MDA£@$]}¡@INVALID_P4$$W@RD¡@$[@¡"){
            QMessageBox msg_box;
            msg_box.setText("Invalid password");
            msg_box.exec();
            return;
        }
        qDebug() << "Password checks out! " << result;
        ui->password_display->setText(result);
        return;
    }
    message += item->data(3,0).toByteArray() + "|" + item->data(2,0).toByteArray() + "|N-E-X";
    server->password_manager_update(message);
}


void MainWindow::on_password_copy_clicked(){

}
// ******************************* Password Manager Related Functions *******************************


// ******************************* File Transfer Related Functions **********************************
void MainWindow::initialize_ftp(){
    tree_widget_ftp = new QTreeWidget(this);
    tree_widget_ftp->setStyleSheet("background-color: rgb(61, 56, 70); color: white;");
    tree_widget_ftp->setColumnCount(2);
    tree_widget_ftp->headerItem()->setText(0, "File");
    tree_widget_ftp->headerItem()->setText(1, "Size");
    tree_widget_ftp->headerItem()->setText(2, "Date Added");
    tree_widget_ftp->sortItems(0, Qt::AscendingOrder);
    tree_widget_ftp->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    tree_widget_ftp->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->treewidgetlayout->insertWidget(0, tree_widget_ftp);
    connect(tree_widget_ftp, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(prepare_menu_ftp(QPoint)));
}


void MainWindow::add_files_to_ftp(QFile *file){ // Slot
    file->open(QIODevice::ReadOnly);
    while(!file->atEnd()){
        QByteArray line = file->readLine();
        if (line.length() <= 5) {qDebug() << "Less than five, continue!"; continue; }
        QTreeWidgetItem *item = new QTreeWidgetItem();
        QString file_name = line.split('|').at(0);
        QString file_hash = line.split('|').at(1);
        QString file_size = line.split('|').at(2);
        QString date_added = line.split('|').at(3);
        QString password_protected = line.split('|').at(4);
        QString sample_sentence = line.split('|').at(5);
        QString true_size = line.split('|').at(7);

        //            int begin_length = unique_hash.length() + domain.length() + username.length() + password_protected.length() + owner.length() + 5;
        //            QByteArray password = line.mid(begin_length, line.length() - begin_length - 2); // 2 being newline "\n"
        //            qDebug() << "Password belonging to " << username << " is: " << password;
        item->setData(0, 0, file_name);
        item->setData(1, 0, true_size);
        item->setData(2, 0, date_added);
        item->setData(3, 0, password_protected);
        item->setData(4, 0, sample_sentence);
        item->setData(5, 0, file_hash);
        item->setData(6, 0, file_size);


        tree_widget_ftp->addTopLevelItem(item);
    }
    tree_widget_ftp->sortItems(0, Qt::AscendingOrder);
    //        file->remove();
}


void MainWindow::prepare_menu_ftp(QPoint){

}


void MainWindow::tree_widget_ftp_item_clicked(QTreeWidgetItem* item){
    qDebug() << "CLICKED!";
    qDebug() << item->data(6, 0);
    // OLD STUFF COPIED FROM PWM FUNC
    //    if (item->data(3, 0).toString() == "true"){
    //        ui->password_display->setText("**************************************************");
    //        ui->password_lock->setText("Unlock");
    //    }
    //    else{
    //        ui->password_display->setText(aes_decryptor(item->data(2,0).toByteArray(), server->username.toLower()));
    //        ui->password_lock->setText("Lock");
    //    }
    //    qDebug() << item->data(2, 0).toByteArray();
}


void MainWindow::file_sent(FileTransfer* ft){
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setData(0, 0, server->file_transfer->file_name);
    item->setData(1, 0, server->file_transfer->true_size);
    item->setData(2, 0, server->file_transfer->date_added);
    item->setData(3, 0, server->file_transfer->password_protected);
    item->setData(4, 0, server->file_transfer->verification_hash);
    item->setData(5, 0, server->file_transfer->file_hash);
    item->setData(6, 0, server->file_transfer->data_size);
    tree_widget_ftp->addTopLevelItem(item);
}


void MainWindow::file_received(FileTransfer *ft){

}
// ******************************* File Transfer Related Functions **********************************


// *********************************** Settings Related Functions ***********************************
void MainWindow::load_from_settings(bool launching){
    if (!QDir("data").exists())
        QDir().mkdir(QDir::currentPath() + "/data");

    QFile file("data/config.nex");
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    while (!stream.atEnd()){
        QString line = stream.readLine();
        if (line.size() < 5 || !line.contains('='))
            continue;
        QString field = line.split('=').at(0);
        QString value = line.mid(field.size() + 1);

        if (field == "server_domain")
            ui->server_domain->setText(value);
        else if (field == "server_port")
            ui->server_port->setText(value);
        else if (field == "private_key")
            ui->private_key->setText(value);
        else if (field == "local_certificate")
            ui->local_certificate->setText(value);
        else if (field == "ca_certificates")
            ui->ca_certificates->setText(value);
        else if (field == "file_destination")
            ui->file_destination->setText(value);
    }
    if (ui->file_destination->text() == "")
        ui->file_destination->setText(QDir::currentPath() + "/data");
    file.close();
    if (launching || !logged_in){
        server->server_domain = ui->server_domain->text();
        server->server_port = ui->server_port->text().toInt();
        server->private_key = ui->private_key->text();
        server->local_certificate = ui->local_certificate->text();
        server->ca_certificates = ui->ca_certificates->text();
    }
}


void MainWindow::on_save_settings_clicked(){
    QString settings;
    settings += "server_domain=" + ui->server_domain->text() + "\n";
    settings += "server_port=" + ui->server_port->text() + "\n";
    settings += "private_key=" + ui->private_key->text() + "\n";
    settings += "local_certificate=" + ui->local_certificate->text() + "\n";
    settings += "ca_certificates=" + ui->ca_certificates->text() + "\n";
    settings += "file_destination=" + ui->file_destination->text() + "\n";

    QFile file("data/config.nex");
    qDebug() << settings;
    file.open(QIODevice::WriteOnly);

    file.write(settings.toUtf8());
    return;
    if (logged_in){
        // Sync and close stuff...
        sync_if_unsynced();
        QMessageBox msg;
        msg.setText("Close client to apply changes.");
        msg.exec();
        exit(33);
    }
}
// *********************************** Settings Related Functions ***********************************


// ******************************* Buttons **********************************************************
void MainWindow::notes_btn_pressed(){
    ui->stackedWidget->setCurrentIndex(1);
}


void MainWindow::pwm_btn_pressed(){
    ui->stackedWidget->setCurrentIndex(2);
    sync_if_unsynced();
}


void MainWindow::files_btn_pressed(){    
    ui->stackedWidget->setCurrentIndex(3);
    sync_if_unsynced();
}


void MainWindow::test_btn_pressed(){
    qDebug() << current_item->data(4,0).toString();
    qDebug() << current_item->data(5,0).toString();
    qDebug() << current_note_password;

}


void MainWindow::settings_btn_pressed(){ // Leftside panel
    sync_if_unsynced();
//    load_from_settings(); Do I really need this when already inside?
    previous_index = ui->stackedWidget->currentIndex();
    ui->stackedWidget->setCurrentIndex(4);
}


void MainWindow::on_settings_btn_clicked(){ // Main screen
//    load_from_settings();

    previous_index = ui->stackedWidget->currentIndex();
    ui->stackedWidget->setCurrentIndex(4);
}


void MainWindow::on_password_clicked(){
    right_clicked_item = tree_widget->currentItem();
    action_password();
}


void MainWindow::on_delete_note_btn_clicked(){
    right_clicked_item = tree_widget->currentItem();
    action_delete_item();
}


void MainWindow::on_close_settings_clicked(){
    ui->stackedWidget->setCurrentIndex(previous_index);
}


void MainWindow::on_send_file_btn_clicked(){
    QFileDialog dialog;
    QString file_to_send = dialog.getOpenFileName();
    qDebug() << file_to_send;

    if (file_to_send == "")
        return;

    bool ok;
    QString encryption_key = QInputDialog::getText(this, "Encrypt File", "Enter Encryption Key.\n"
            "Leave the field blank if you wish to use a default encryption key.",
                QLineEdit::EchoMode(QLineEdit::Password), "", &ok, Qt::WindowFlags(Qt::Dialog));

    server->file_transfer = new FileTransfer(server->server_domain, server->ftp_server_port, server->private_key, server->local_certificate, server->ca_certificates);
    server->file_transfer->file_to_handle = file_to_send;

    if (!encryption_key.isEmpty()){
        server->file_transfer->encryption_key = encryption_key;
        server->file_transfer->password_protected = "true";
    }
    else{
        server->file_transfer->encryption_key = server->encryption_key;
        server->file_transfer->password_protected = "false";
    }

    server->file_transfer->username = server->username;
    server->file_transfer->receiving = false;
    server->send_ftp_connect();
    connect(server->file_transfer, SIGNAL(file_sent(FileTransfer*)), this, SLOT(file_sent(FileTransfer*)));
}

void MainWindow::on_receive_file_btn_clicked(){ // verify that the correct encryption key is present before transferring file.
    QTreeWidgetItem *item = tree_widget_ftp->currentItem();
    qDebug() << item->data(3, 0).toString();
    QString verification_key;
    QString encryption_key;
    if (item->data(3, 0).toString() == "true"){
        bool ok;
        encryption_key = QInputDialog::getText(this, "Unlock File", "Enter Encryption Key",
                                                  QLineEdit::EchoMode(QLineEdit::Password), "", &ok);
        verification_key = encryption_key_hasher(encryption_key, "verification_hash");
        encryption_key = encryption_key_hasher(encryption_key, "encryption_key");
        if (!ok)
            return;
    } else {
        verification_key = encryption_key_hasher(server->encryption_key, "verification_hash");
        encryption_key = encryption_key_hasher(server->encryption_key, "encryption_key");

    }
    bool verified = verify_password(item->data(4, 0).toString(), verification_key);
    qDebug() << verified;
    if (!verified){
        // error message then return
        qDebug() << "Wrong encryption key!";
        return;
    }
    qDebug() << "Correct password!";

    prepare_to_receive_file(item, encryption_key);
}


void MainWindow::prepare_to_receive_file(QTreeWidgetItem *item, QString user_key){
    server->file_transfer = new FileTransfer(server->server_domain, server->ftp_server_port, server->private_key, server->local_certificate, server->ca_certificates);
    server->file_transfer->file_to_handle = item->data(5, 0).toString();
    server->file_transfer->data_size = item->data(6, 0).toInt();
    server->file_transfer->encryption_key = user_key;
    server->file_transfer->destination = ui->file_destination->text() + "/" + item->data(0, 0).toString();
    server->file_transfer->encrypted_file.setFileName("data/tmpfile001.nex");
    server->file_transfer->true_size = item->data(1, 0).toInt();
    if(server->file_transfer->encrypted_file.exists())
        server->file_transfer->encrypted_file.remove();
    server->file_transfer->encrypted_file.open(QIODevice::Append);
    qDebug() << server->file_transfer->file_to_handle;
    qDebug() << server->file_transfer->data_size;
    server->file_transfer->receiving = true;
    server->send_ftp_connect();
    connect(server->file_transfer, SIGNAL(file_received(FileTransfer*)), this, SLOT(file_received(FileTransfer*)));

}


void MainWindow::on_delete_file_btn_clicked(){
    QMessageBox dialog;
    QTreeWidgetItem *item_to_delete = tree_widget_ftp->currentItem();
    dialog.setText("Do you wish to delete this file?");
    QPushButton *btn = dialog.addButton(tr("Delete"), QMessageBox::ActionRole);
    dialog.setStandardButtons(QMessageBox::Cancel);
    dialog.setDefaultButton(QMessageBox::Cancel);
    int result = dialog.exec();
    if (result != 0)
        return;
    QString file_to_delete = item_to_delete->data(5, 0).toString();
    delete tree_widget_ftp->currentItem();
    server->request_file_deletion(file_to_delete);
}


void MainWindow::on_open_priv_key_btn_clicked(){
    QFileDialog dialog;
    QString file_chosen = dialog.getOpenFileName();
    if (file_chosen == "")
        return;
    ui->private_key->setText(file_chosen);
}


void MainWindow::on_open_loc_cert_btn_clicked(){
    QFileDialog dialog;
    QString file_chosen = dialog.getOpenFileName();
    if (file_chosen == "")
        return;
    ui->local_certificate->setText(file_chosen);
}


void MainWindow::on_open_ca_certs_btn_clicked(){
    QFileDialog dialog;
    QString file_chosen = dialog.getOpenFileName();
    if (file_chosen == "")
        return;
    ui->ca_certificates->setText(file_chosen);
}

void MainWindow::on_choose_destination_btn_clicked(){
    QFileDialog dialog;
    QString file_chosen = dialog.getExistingDirectory();
    if (file_chosen == "")
        return;
    ui->file_destination->setText(file_chosen);
}

