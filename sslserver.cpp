#include "sslserver.h"

SSLServer::SSLServer(QObject *parent)
    : QTcpServer{parent}{

}

void SSLServer::connect_to_server(){
    server = new QSslSocket(this);

    // Connects signals for incoming data, server disconnect, and errors to their respective functions/slots.
    connect(server, &QSslSocket::readyRead, this, &SSLServer::incoming_data);
    connect(server, &QSslSocket::disconnected, this, &SSLServer::server_disconnect);
    connect(server, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ssl_errors(QList<QSslError>)));

    // Writes private_key and local_certificate into SSL Socket, and sets peer verify mode to none.
    server->setPrivateKey(private_key);
    server->setLocalCertificate(local_certificate);
    server->setPeerVerifyMode(QSslSocket::VerifyNone);

    // Connects to server. Current object self destructs if connection fails
    server->connectToHostEncrypted(server_domain, port_number);
    if (server->waitForEncrypted(5000)) {
        connected = true;
    }
    else {
        qDebug("Unable to connect to serverx");
        qDebug() << server->errorString();
        delete this;
    }
}

void SSLServer::reconnect_to_server(){

}

void SSLServer::incoming_data(){
    // Protocols similar to server side. Identify what kind of data it is and what to do with it.


}

void SSLServer::server_disconnect(){
    // Do later...

}

void SSLServer::ssl_errors(const QList<QSslError> &errors){
    qDebug() << errors;
}
