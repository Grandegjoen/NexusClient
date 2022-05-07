#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <QTcpServer>
#include <QSslSocket>
#include <QSslKey>
#include <QSslCertificate>
#include <QTcpSocket>

class SSLServer : public QTcpServer
{
public:
    explicit SSLServer(QObject *parent = nullptr);
    void connect_to_server();
    void reconnect_to_server();
    QString username = "Admin";
    bool connected = false;

private slots:
    void server_disconnect();
    void incoming_data();
    void ssl_errors(const QList<QSslError> &errors);

private:
    QSslSocket *server;
    QString private_key;
    QString local_certificate;
    QString server_domain = "127.0.0.1";
    quint16 port_number = 12345;

};

#endif // SSLSERVER_H
