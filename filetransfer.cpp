#include "filetransfer.h"

// ***************** Server related functions **********************
FileTransfer::FileTransfer(QString domain, quint16 port, QString priv_key, QString local_cert, QString ca_certs){
    server_domain = domain;
    server_port = port;
    private_key = priv_key;
    local_certificate = local_cert;
    ca_certificates = ca_certs;
}

void FileTransfer::incoming_data(){
    qDebug() << "Incoming ftp data...";
    if (active_data_transmission){
        QByteArray tmp_data = server->read(server->bytesAvailable());
        receive_data(&tmp_data);
        return;
    }
    data.append(server->read(server->bytesAvailable()));
    qDebug() << "Receiving DATA: " << data;
    if (data.last(5) == "N-E-X"){
        qDebug() << "Not active!";
        receive_message();
    }
}


void FileTransfer::receive_message(){
    QString message_type = data.split('|').at(0);
    qDebug() << "Receiving message FTP: " << message_type;
    if (message_type == "file_info"){
        // Data size, name etc...
        // Send ready_to_receive to server when ready, active_data_transmission ...)
    }
    else if (message_type == "receiving_data"){
        send_file();
    }
    else if (message_type == "session_accepted"){ // You're here right now, do logic from this point on.
        qDebug() << "Session accepted! FTP";
        if (receiving){
            server->write("receiving_data|" + file_to_handle.toUtf8() + "|N-E-X");
            active_data_transmission = true;
        }
        else
            send_file_info();
    }
    else if (message_type == "session_rejected"){
        qDebug() << "Session rejected! FTP";
        delete this;
    }
    else if (message_type == "data_received"){
        qDebug() << "Emitting file sent";
        date_added = data.split('|').at(1);
        emit file_sent(this);
        return;
    }
    data.clear();
}


void FileTransfer::receive_data(QByteArray *data){
    qDebug() << *data;
    encrypted_file.write(*data);
    data->clear();
    qDebug() << "File size: " << encrypted_file.size();
    qDebug() << "Data size: " << data_size;
    if (encrypted_file.size() != data_size){
        return;
    } else
        qDebug() << "File.size = " << encrypted_file.size() << " data_size = " << data_size;
    qDebug() << "¡@£@¡$@£½@£$¡@$¡@$¡@$£@½@£$@¡£$¡@£¡@$£$½@£½$@£½£@$¡@£¡@£¡@£¡@£¡@£¡@£¡@£¡@$£@½@$£½¥@$½¥@£$½@£$½ DATA RECEIVED!!!!!!!!!!!!!";
    active_data_transmission = false;
    server->write("data_received|N-E-X");
    qDebug() << "Hashed encryption key receive data: " << encryption_key;
    file_decryptor(encrypted_file.fileName(), destination, encryption_key);
    QFile file(destination);
    qDebug() << "True size: " << true_size;
    file.resize(true_size);
    encrypted_file.remove();
}


void FileTransfer::send_file(){
//    encrypted_file.setFileName("data/temp_encrypted_file.nex");
    encrypted_file.open(QIODevice::ReadOnly);
    quint64 file_size = encrypted_file.size();
    quint64 data_written = 0;
    while (data_written != file_size){
        QByteArray data;
        if (data_written + bytes_to_write > file_size){
            data = encrypted_file.read(file_size - data_written);
            data_written += file_size - data_written;
        } else {
            data = encrypted_file.read(bytes_to_write);
            data_written += bytes_to_write;
        }
        server->write(data);
    }
    qDebug() << "File successfully sent!";
    // File Transfer algorithm here... Temp solution below
}


void FileTransfer::send_file_info(){
    QByteArray message = "file_info|";
//    file.setFileName(file_to_handle);
    hashed_encryption_key = encryption_key_hasher(encryption_key, "encryption_key").toUtf8();
    qDebug() << "Hashed encryption key file info: " << hashed_encryption_key;
    verification_hash = encryption_key_hasher(encryption_key, "verification_hash").toUtf8();
    QFile file(file_to_handle);
    true_size = file.size();
    file.open(QIODevice::Append);
    QByteArray padding = "0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0x0"; // Some padding for cbc...
    file.write(padding);
    file.flush();
    file_encryptor(file_to_handle, hashed_encryption_key);
    file.resize(true_size);
    file.close();
    encrypted_file.setFileName("data/temp_encrypted_file.nex");
    file.setFileName(file_to_handle);
    QFileInfo fi(file.fileName());
    file_name = fi.fileName().toUtf8();
    file_size = QByteArray::number(encrypted_file.size());
    data_size = file_size.toInt();
    file_hash = hash_generator().toUtf8();


    message += file_name + "|" + file_size + "|" + verification_hash + "|" + file_hash + + "|" + password_protected.toUtf8() + "|";
    message += QByteArray::number(fi.size(), 10) + "|N-E-X";
    qDebug() << message;
    server->write(message);
}



void FileTransfer::connect_to_server(){
    qDebug() << "Connecting to ftp server...";
    server = new QSslSocket(this);
    // Connects signals for incoming data, server disconnect, and errors to their respective functions/slots.
    connect(server, &QSslSocket::readyRead, this, &FileTransfer::incoming_data);
    connect(server, &QSslSocket::disconnected, this, &FileTransfer::server_disconnect);
    connect(server, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ssl_errors(QList<QSslError>)));

    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.addCaCertificates(ca_certificates);
    server->setSslConfiguration(ssl_config);


    // Writes private_key and local_certificate into SSL Socket, and sets peer verify mode to none.
    server->setPrivateKey(private_key);
    server->setLocalCertificate(local_certificate);
    server->setPeerVerifyMode(QSslSocket::VerifyPeer);

    // Connects to server. Current object self destructs if connection fails
    qDebug() << "Connecting to: " << server_domain << ":" << server_port;
    server->connectToHostEncrypted(server_domain, server_port);
    if (server->waitForEncrypted(5000)) {
        qDebug() << "Connected!";
    }
    else {
        qDebug("Unable to connect to server");
        qDebug() << server->errorString();
        delete this;
        return;
    }
    send_session_id();
}


void FileTransfer::send_session_id(){
    qDebug() << "Sending session id...";
    server->write("session_id|" + QString::number(session_id).toUtf8() + "|N-E-X");
}


void FileTransfer::server_disconnect(){
    qDebug() << "Disconnected from FTP server.";
    //    delete this;
}


void FileTransfer::ssl_errors(const QList<QSslError> &errors){
    qDebug() << errors;
}
// ***************** Server related functions **********************
