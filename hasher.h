#ifndef HASHER_H
#define HASHER_H

#include <QCryptographicHash>
#include <cryptopp/aes.h>
#include <cryptopp/rijndael.h>
#include <cryptopp/hex.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include <cryptopp/channels.h>
#include <cryptopp/filters.h>
#include <cryptopp/sha.h>
#include <cryptopp/crc.h>
#include <QString>
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include <string>
#include <QFile>

QString password_hasher(QString username, QString password);
QString hash_generator();
QString username_based_hash(QString);
QString encryption_key_hasher(QString, QString);
QString create_encryption_key(QString username, QString user_hash);
QByteArray note_encrypter_password(QString *note, QString password);
QByteArray note_encrypter_no_password(QString *note, QString username);
QString note_decrypter(QByteArray *encrypted_note, QString password);
QByteArray aes_encryptor(QString plain_text, QString key);
QString aes_decryptor(QByteArray encrypted_text, QString key);

void file_encryptor(QString file_path, QString key);
void file_decryptor(QString file_path, QString file_destination, QString encryption_key);
void add_padding(QString file_path);
void remove_padding(QString file_path);

bool verify_password(QString file_key, QString user_key);

#endif // HASHER_H
