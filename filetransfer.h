#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QTcpServer>
#include <QObject>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslKey>
#include <QFile>
#include <QFileInfo>
#include "hasher.h"

class FileTransfer : public QTcpServer
{
    Q_OBJECT
public:
    FileTransfer(QString, quint16, QString, QString, QString);
    // Server Functions
    void connect_to_server();
    void send_session_id(); // Junk? Nvm...

    // Server Variables
    QString private_key;
    QString local_certificate;
    QString ca_certificates;
    QSslKey key;
    QSslCertificate cert;
    QString server_domain = "127.0.0.1";
    quint16 server_port = 13580;
    quint32 session_id;
    QSslSocket *server;
    QString username;


    // File Transfer Functions
    void send_password();
    void receive_data(QByteArray*);
    void send_file();
    void send_file_info();
    void receive_message();


    // File Transfer Variables
    QString file_to_handle; // file path
    QFile file;
    QFile encrypted_file;
    QFile decrypted_file;
    bool receiving;
    QString password_protected = "false";
    QString encryption_key;
    QByteArray data;
    quint64 data_size;
    bool active_data_transmission = false;
    QString date_added;
    quint32 bytes_to_write = 2048;
    QByteArray verification_hash;
    QString destination;
    quint64 true_size;

    // File information / metadata
    QByteArray file_name;
    QByteArray file_size;
    QByteArray file_hash;
    QByteArray hashed_encryption_key;

signals:
    void file_received();
    void file_sent(FileTransfer*);

private slots:
    void server_disconnect();
    void incoming_data();
    void ssl_errors(const QList<QSslError> &errors);

};

#endif // FILETRANSFER_H
