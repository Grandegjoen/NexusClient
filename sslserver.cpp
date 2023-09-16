#include "sslserver.h"

SslServer::SslServer(QObject *parent)
    : QTcpSocket{parent}{

}
// ---------------------------- HANDLES START OF SERVER AND ALL CONNECTIONS TO IT ----------------------------


// ---------------------------- HANDLES SERVER CONNECTION ----------------------------------------------------
void SslServer::connect_to_server(){
    qDebug() << "Connecting...";
    server = new QSslSocket(this);
    // Connects signals for incoming data, server disconnect, and errors to their respective functions/slots.
    connect(server, &QSslSocket::readyRead, this, &SslServer::incoming_data);
    connect(server, &QSslSocket::disconnected, this, &SslServer::server_disconnect);
    connect(server, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ssl_errors(QList<QSslError>)));

    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.addCaCertificates(ca_certificates);
    server->setSslConfiguration(ssl_config);


    // Writes private_key and local_certificate into SSL Socket, and sets peer verify mode to none.
    server->setPrivateKey(private_key);
    server->setLocalCertificate(local_certificate);
    server->setPeerVerifyMode(QSslSocket::VerifyPeer);

    // Connects to server. Current object self destructs if connection fails
    qDebug() << "Connecting...";
    server->connectToHostEncrypted(server_domain, server_port);
    if (server->waitForEncrypted(5000)) {
        connected = true;
        emit server_is_connected(true);
        qDebug() << "Connected! Emitting server_is_connected!";
    }
    else {
        qDebug("Unable to connect to server");
        qDebug() << server->errorString();
    }
}


void SslServer::login_to_server(QString username, QString password){
    QByteArray data = "login|" + username.toUtf8() + "|" + password.toUtf8() + "|N-E-X";
    qDebug() << data;
    server->write(data);
}


//void SslServer::set_keys(){
//    private_key = "/home/admin/keys/grandegjoen_server_keys/blue_local.key";
//    local_certificate = "/home/admin/keys/grandegjoen_server_keys/blue_local.pem";
//    ca_certificates = "/home/admin/keys/grandegjoen_server_keys/red_ca.pem";
//}


void SslServer::server_disconnect(){
    qDebug() << "Disconnected from SSL Server.";
}


void SslServer::ssl_errors(const QList<QSslError> &errors){
    qDebug() << errors;
}
// ---------------------------- HANDLES SERVER CONNECTION ----------------------------------------------------


// ---------------------------- HANDLES INCOMING DATA --------------------------------------------------------
void SslServer::incoming_data(){
    qDebug() << "Incoming data ... ";
    data.append(server->read(server->bytesAvailable()));
    qDebug() << "Data: " << data << "\n";
    if (active_data_transmission){
        qDebug() << "Active!";
        if (data.size() == data_size){
            receive_complete_data();
            data.clear();
        }
        else
            return;
    }
    else if (data.last(5) == "N-E-X"){
        qDebug() << "Not active!";
        receive_message();
    }
}


void SslServer::receive_message(){
    QString message_type = data.split('|').at(0);
    qDebug() << "Message type: " << message_type;
    if (message_type == "received_data"){
        qDebug() << "received_data!!!!!!!!!!!!!!!!!!";
        active_command = false;
        if (command_stack.size() != 0){
            qDebug() << "Popping stack!";
            qDebug() << command_stack.at(0);
            server->write(command_stack.at(0).toUtf8());
            command_stack.pop_front();
        }
    }
    else if (message_type == "server_send_note"){
        data_size = data.split('|').at(1).toInt();
        if (data_size == 0){
            qDebug() << "There is no note! Returning...";
            emit no_note();
        }
        else {
            qDebug() << data_size << "|||||||||||||";
            data_type = "note";
            active_data_transmission = true;
            server->write("server_send_note|receiving|N-E-X");
        }
    }
    else if (message_type == "server_receive_note"){
        send_data(&note);
    }
    else if (message_type == "server_send_notelist"){
        data_size = data.split('|').at(1).toInt();
        if (data_size == 0){
            data.clear();
            server->write("received_data|N-E-X");
        }
        else {
            active_data_transmission = true;
            data_type = "notelist";
            server->write("server_send_notelist|receiving|N-E-X");
        }
    }
    else if (message_type == "server_receive_notelist"){
        send_data(&note_list);
    }
    else if (message_type == "server_send_pwm"){
        qDebug() << "Server send pwm received!";
        data_size = data.split('|').at(1).toInt();
        if (data_size == 0){
            data.clear();
            server->write("received_data|N-E-X");
            return;
        }
        else {
            data_type = "pwm_list";
            active_data_transmission = true;
            server->write("server_send_pwm|receiving|N-E-X");
        }
    }
    else if (message_type == "server_send_ftp"){
        qDebug() << "Server send ftp received!";
        data_size = data.split('|').at(1).toInt();
        if (data_size == 0){
            data.clear();
            server->write("received_data|N-E-X");
            return;
        }
        else {
            data_type = "ftp_list";
            active_data_transmission = true;
            server->write("server_send_ftp|receiving|N-E-X");
        }
    }
    else if (message_type == "login"){
        if (data.split('|').at(1) == "success"){
            QString unique_user_hash = data.split('|').at(2);
            emit server_connected(true);
        }
        else
            emit server_connected(false);
    }
    else if (message_type == "note_received"){
        notes_to_send.pop_front();
        if (notes_to_send.size() != 0)
            send_note();
    }
    else if (message_type == "ftp_connection"){
        qDebug() << "FTP Connection received";
        if (data.split('|').at(1) != "00000"){
            file_transfer->session_id = data.split('|').at(1).toInt();
            file_transfer->connect_to_server();
        } else {
            qDebug() << "Invalid session id.";
        }
    }
    data.clear();
}


void SslServer::receive_complete_data(){
    qDebug() << "Receiving complete data << : " << data_type;
    if (data_type == "notelist"){
        QFile note_list("data/temp_n_l.nex");
        note_list.open(QIODevice::WriteOnly);
        note_list.write(data);
        note_list.close();
        qDebug() << "Emitting notelist ready.";
        emit notelist_ready("data/temp_n_l.nex");
    }
    else if (data_type == "note"){
        QFile note("data/" + current_note);
        note.open(QIODevice::WriteOnly);
        note.write(data);
        note.close();
        emit note_ready("data/" + current_note);
    }
    else if (data_type == "pwm_list"){
        qDebug() << "Receiving pwm list!";
        QFile pwm_list("data/temp_pwm.nex");
        pwm_list.open(QIODevice::WriteOnly);
        pwm_list.write(data);
        pwm_list.close();
        emit pwm_list_ready(&pwm_list);
    }
    else if (data_type == "ftp_list"){
        qDebug() << "Receiving pwm list!";
        QFile ftp_list("data/temp_ftp.nex");
        ftp_list.open(QIODevice::WriteOnly);
        ftp_list.write(data);
        qDebug() << "Ftp list: " << data;
        ftp_list.close();
        emit ftp_list_ready(&ftp_list);
    }
    active_data_transmission = false;
}


void SslServer::announce_data_received(){
    server->write("received_data|N-E-X");
}
// ---------------------------- HANDLES INCOMING DATA --------------------------------------------------------


// ---------------------------- HANDLES NOTE RELATED STUFF ---------------------------------------------------
QByteArray SslServer::return_note(){ // Returns note in QByteArray format
    QFile note("data/" + notes_to_send.at(0));
    note.open(QIODevice::ReadOnly);
    QByteArray data = note.readAll();
    note.close();
    qDebug() << "Returning note..";
    return data;
}


QByteArray SslServer::return_notelist(){ // Returns note in QByteArray format
    QFile note_list("data/temp_n_l.nex");
    note_list.open(QIODevice::ReadOnly);
    QByteArray data = note_list.readAll();
    note_list.close();
    qDebug() << "Returning notelist..";
    return data;
}


void SslServer::send_note(){
    note = return_note();
    QString message = "server_receive_note|" + QString::number(note.size()).toUtf8() + "|" + notes_to_send.at(0).toUtf8() + "|N-E-X";
    notes_to_send.pop_front();
    if (active_command)
        command_stack.append(message);
    else {
        server->write(message.toUtf8());
        active_command = true;
    }
}


void SslServer::send_delete_note(QString message){
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message.toUtf8());
        active_command = true;
    }
}


void SslServer::send_encryption_request(QString note, QString password, QString hashed_pw){
    QString message = "server_encrypt_note|" + note + "|" + password + "|" + hashed_pw + "|N-E-X";
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message.toUtf8());
        active_command = true;
    }
}


void SslServer::send_notelist(){
    note_list = return_notelist();
    QString message = "server_receive_notelist|" + QString::number(note_list.size()).toUtf8() + "|N-E-X";
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message.toUtf8());
        active_command = true;
    }
}


void SslServer::request_note_list(){
    QString message = "server_send_notelist|" + QString::number(note_list.size()).toUtf8() + "|N-E-X";
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message.toUtf8());
        active_command = true;
    }
}


void SslServer::request_note(QString note){
    QString message = "server_send_note|" + note.toUtf8() + "|N-E-X";
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message.toUtf8());
        active_command = true;
    }
}
// ---------------------------- HANDLES NOTE RELATED STUFF ---------------------------------------------------


// ---------------------------- HANDLES PWM RELATED STUFF ---------------------------------------------------
void SslServer::password_manager_update(QByteArray message){
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message);
    }
}


void SslServer::request_pwm_list(){
    QByteArray message = "server_send_pwm|N-E-X";
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message);
        active_command = true;
    }
}
// ---------------------------- HANDLES PWM RELATED STUFF ---------------------------------------------------


// ---------------------------- HANDLES FTP RELATED STUFF ---------------------------------------------------
void SslServer::send_ftp_connect(){
    QByteArray message = "ftp_connection|N-E-X";
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message);
    }
}


void SslServer::request_ftp_list(){
    QByteArray message = "server_send_ftp|N-E-X";
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message);
        active_command = true;
    }
}


void SslServer::request_file_deletion(QString file_to_delete){
    QByteArray message = "file_deletion|" + file_to_delete.toUtf8() + "|N-E-X";
    if (active_command)
        command_stack.append(message);
    else{
        server->write(message);
        active_command = true;
    }
}

// ---------------------------- HANDLES FTP RELATED STUFF ---------------------------------------------------

void SslServer::send_data(QByteArray* data){
    quint32 bytes_written = 0;
    quint32 bytes_to_write = data->size();
    while(bytes_written != bytes_to_write){
        if (bytes_to_write - bytes_written < max_packet_size){
            server->write(data->mid(bytes_written, bytes_to_write - bytes_written));
            bytes_written += bytes_to_write - bytes_written;
        }
        else {
            server->write(data->mid(bytes_written, max_packet_size));
            bytes_written += max_packet_size;
        }
    }
    active_data_transmission = false;
    qDebug() << "Done sending!";
}
