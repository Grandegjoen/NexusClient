#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <QTcpSocket>
#include <QObject>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslKey>
#include <QFile>
#include "filetransfer.h"

class SslServer : public QTcpSocket
{
    Q_OBJECT
public:
    explicit SslServer(QObject *parent = nullptr);


    // ---------------------------- HANDLES SERVER CONNECTION ----------------------------------------------------
    // Functions
    void connect_to_server();
    void login_to_server(QString, QString);

    // Variables
    bool connected = false;
    QString private_key;
    QString local_certificate;
    QString ca_certificates;
    QSslKey key;
    QSslCertificate cert;
    QString server_domain = "127.0.0.1";
    quint16 server_port = 13579;
    quint16 ftp_server_port = 13580;
    QString username;
    QString unique_user_hash;
    QString encryption_key;

    // ---------------------------- HANDLES SERVER CONNECTION ----------------------------------------------------


    // ---------------------------- HANDLES NOTE RELATED STUFF ---------------------------------------------------
    // Functions
    void request_note(QString);
    void request_note_list();
    void send_note();
    void send_notelist();
    void send_delete_note(QString);
    void send_encryption_request(QString, QString, QString);
    QByteArray return_note();
    QByteArray return_notelist();

    // Variables
    QString current_note;
    QVector <QString> notes_to_send;
    QByteArray note;
    QByteArray note_list;
    // ---------------------------- HANDLES NOTE RELATED STUFF ---------------------------------------------------


    // ---------------------------- HANDLES PWM RELATED STUFF ---------------------------------------------------
    void password_manager_update(QByteArray);
    void request_pwm_list();
    void receive_pwm_list();
    // ---------------------------- HANDLES PWM RELATED STUFF ---------------------------------------------------


    // ---------------------------- HANDLES DATA TRANSMISSION STUFF ---------------------------------------------------
    bool active_data_transmission = false;
    quint64 data_size;
    QString data_type;
    QByteArray data;
    quint16 max_packet_size = 1024;
    QVector <QString> command_stack;
    bool active_command = false;

    void receive_message();
    void receive_complete_data();
    void announce_data_received();
    void send_data(QByteArray* data);
    // ---------------------------- HANDLES DATA TRANSMISSION STUFF ---------------------------------------------------


    // ---------------------------- HANDLES FTP STUFF ---------------------------------------------------
    FileTransfer *file_transfer;
    void send_ftp_connect();
    void request_ftp_list();
    void receive_ftp_list();
    void request_file_deletion(QString);
    // ---------------------------- HANDLES FTP STUFF ---------------------------------------------------


signals:
    void server_connected(bool);
    void note_ready(QString);
    void notelist_ready(QString);
    void pwm_list_ready(QFile*);
    void ftp_list_ready(QFile*);
    void no_note();
    void server_is_connected(bool);


private slots:
    void server_disconnect();
    void incoming_data();
    void ssl_errors(const QList<QSslError> &errors);


private:
    QSslSocket *server;
};

#endif // SSLSERVER_H
