#include "sslserver.h"

SSLServer::SSLServer(QObject *parent)
    : QTcpServer{parent}{
}

void SSLServer::set_keys(){
    private_key = "/home/admin/keys/grandegjoen_server_keys/blue_local.key";
    local_certificate = "/home/admin/keys/grandegjoen_server_keys/blue_local.pem";
    ca_certificates = "/home/admin/keys/grandegjoen_server_keys/red_ca.pem"; // Is this necessary?
}

void SSLServer::connect_to_server(){
    server = new QSslSocket(this);
    set_keys();
    // Connects signals for incoming data, server disconnect, and errors to their respective functions/slots.
    connect(server, &QSslSocket::readyRead, this, &SSLServer::incoming_data);
    connect(server, &QSslSocket::disconnected, this, &SSLServer::server_disconnect);
    connect(server, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ssl_errors(QList<QSslError>)));

    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    ssl_config.addCaCertificates(ca_certificates);
    server->setSslConfiguration(ssl_config);

    // Writes private_key and local_certificate into SSL Socket, and sets peer verify mode to none.
    server->setPrivateKey(private_key);
    server->setLocalCertificate(local_certificate);


    server->setPeerVerifyMode(QSslSocket::VerifyPeer);

    // Connects to server. Current object self destructs if connection fails
    server->connectToHostEncrypted(server_domain, port_number);
    if (server->waitForEncrypted(5000)) {
        connected = true;
        qDebug() << "Connected!";
    }
    else {
        qDebug("Unable to connect to serverx");
        qDebug() << server->errorString();
        delete this;
    }
}

void SSLServer::get_server_fileinfo(QString path){
    // Decide on a proper protocol...
    server->write("SERVER_FILE_INFO");
}


void SSLServer::reconnect_to_server(){

}


// Do different things with data depending on data_type
// Create file(1) if file already exists.
void SSLServer::incoming_data(){
    // Protocols similar to server side. Identify what kind of data it is and what to do with it.
    if (!data_stream_active){
        qDebug() << "Data stream not active!";
        qDebug() << server->bytesAvailable();
        QByteArray server_data = server->readAll();
        qDebug() << server_data;
        data_type = server_data.split('|').at(0);
        qDebug() << data_type;
        if (data_type == "file_transfer"){
            file_name = server_data.split('|').at(1);
            file_size = server_data.split('|').at(2).toInt(0, 10) - 500;
            file.setFileName(file_name); // Create file(1) if file_name already exists...
            file.open(QIODevice::Append);
            file.write(server_data.mid(500, -1));
        }
        else if (data_type == "file_info"){

        }
        else if (data_type == ""){

        }
        data_stream_active = true;
    }
    if (data_type == "file_transfer"){
        if (file.size() != file_size){ // File___Size and file...size... A bit confusing naming, two different variables
            file.write(server->read(server->bytesAvailable()));
        }
        if (file.size() == file_size){
            qDebug() << file_size << " and " << file.size();
            qDebug() << file.size();
            file.close();
            file_name.clear();
            file_size = 0;
            file.setFileName("");
            qDebug() << "Done!";
        }
    }
    else if (data_type == "file_info"){

    }
    else if (data_type == "..."){

    }


    return;
//    qDebug() << test;
//    return; // Rememeber the return here if you're looking for bugs!!!!!!!"!¤#"%#¤%#"%#"
////    qDebug() << test;
//    if (separators_read != 4){
//        data_type = test.split('|').at(0);
//        file_name_size = std::stoi(test.split('|').at(1).toStdString(), 0, 2);
//        file_name = test.split('|').at(2);
//        file_size = std::stoi(test.split('|').at(3).toStdString(), 0, 2);
//        separators_read = 4;
//        // Check if file already exists..
//        file.setFileName(file_name);
//        file.open(QIODevice::Append);
//        return;
//    }
//    file.write(test);
//    if (data_type == "file"){
//        if (file.size() == file_size){
//            file.close();
//            file_name_size = 0;
//            file_name = "";
//            file_size = 0;
//            data_type = "";
//        }
//    }

}

void SSLServer::test_shit(){
    QFile file("test123.png");
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);
    QString file_name;
    qint32 num;
    in >> file_name;
    in >> num;
    qDebug() << file_name;
    qDebug() << num;

}

void SSLServer::server_disconnect(){
    // Do later...

}

void SSLServer::ssl_errors(const QList<QSslError> &errors){
    qDebug() << errors;
}
