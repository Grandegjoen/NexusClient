#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSslSocket>
#include <QSslKey>
#include <QSslCertificate>
#include <QTcpSocket>
#include <QFile>
#include <QSslConfiguration>
#include <string>

class SSLServer : public QTcpServer
{

    Q_OBJECT

public:
    explicit SSLServer(QObject *parent = nullptr);
    void connect_to_server();
    void reconnect_to_server();
    void get_server_fileinfo(QString path);
    QString username = "Admin";
    bool connected = false;
    QVector <QString> server_files;
    QVector <QString> server_notes;
    QString file_tree_path = "/";
    void test_shit();

private slots:
    void server_disconnect();
    void incoming_data();
    void ssl_errors(const QList<QSslError> &errors);

signals:
    void files_updated();
    void notes_updated();

private:
    QSslSocket *server;
    qint32      m_blockSize;
    QString private_key;
    QString local_certificate;
    QString ca_certificates;

    QString server_domain = "127.0.0.1";
    quint16 port_number = 12345;
    void set_keys();

    // Temp for testing stuff:
    bool data_stream_active;
    QString data_type;
    QString file_name;
    qint64 file_size;
    int separators_read = 0;
    QFile file;
};

#endif // SSLSERVER_H
