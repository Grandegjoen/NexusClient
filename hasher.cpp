#include "hasher.h"
#include <iomanip>
#include <iostream>
#include <QFile>


inline bool EndOfFile(const CryptoPP::FileSource& file){
  std::istream* stream = const_cast<CryptoPP::FileSource&>(file).GetStream();
  return stream->eof();
}


QString password_hasher(QString username, QString plain_text_pw){
    QString hashed_pw;
    username = username.toLower();
    // Salting pw with random chars from username
    plain_text_pw.push_back(QString(username.at(0)) + username[2] + "K9a");
    plain_text_pw.push_front("e!D" + QString(username[1]));
    QByteArray result = QCryptographicHash::hash(plain_text_pw.toUtf8(), QCryptographicHash::Sha256);
    hashed_pw = QLatin1String(result.toHex());
    return hashed_pw;
}


QString hash_generator(){
    QDateTime date = QDateTime::currentDateTime();
    QString unique_data = date.toString() + "RaNd0M!" + QString::number(QRandomGenerator::global()->generate64());
    QByteArray result = QCryptographicHash::hash(unique_data.toUtf8(), QCryptographicHash::Sha1);
    QString hashed_unique_data = QLatin1String(result.toHex());
    qDebug() << hashed_unique_data;
    return hashed_unique_data;
}


QString username_based_hash(QString username){
    QString hash;
    QByteArray result = QCryptographicHash::hash(username.toLower().toUtf8() + "AOLEOSKAOEJANE!", QCryptographicHash::Sha1);
    hash = QLatin1String(result.toHex());
    qDebug() << hash;
    return hash;
}

QString encryption_key_hasher(QString encryption_key, QString username){
    QString hash;
    QByteArray result = QCryptographicHash::hash(encryption_key.toUtf8() + username.toLower().toUtf8()
                                                 + "8JFJ8ASJFDSADasdas8J¤#&%/!&!", QCryptographicHash::Sha256);
    hash = QLatin1String(result.toHex());
    qDebug() << encryption_key;
    qDebug() << username;
    qDebug() << hash;
    return hash;
}



QString hashed_encryption_key(QString key){
    return key;
}


QByteArray note_encrypter_password(QString *note, QString password){
    if (*note == "")
        *note = "Empty note";
    QByteArray encrypted_note = aes_encryptor(*note, username_based_hash(password));
    return encrypted_note;
}


QByteArray note_encrypter_no_password(QString *note, QString username){
    if (*note == "")
        *note = "Empty note";
    QByteArray encrypted_note = aes_encryptor(*note, username_based_hash(username));
    return encrypted_note;
}


QString note_decrypter(QByteArray *encrypted_note, QString password){
    QString plain_note = aes_decryptor(*encrypted_note, username_based_hash(password));
    return plain_note;
}


QByteArray aes_encryptor(QString plain_text, QString aes_secret_key){
    CryptoPP::SHA256 hash;
    CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
    std::string message = aes_secret_key.toStdString() + "R@nd0m!Sh17$yS4!t!";

    hash.CalculateDigest(digest, (CryptoPP::byte*)message.c_str(), message.length());

    CryptoPP::HexEncoder encoder;
    std::string sKey;
    encoder.Attach(new CryptoPP::StringSink(sKey));
    encoder.Put(digest, sizeof(digest));
    encoder.MessageEnd();

    CryptoPP::byte key[CryptoPP::AES::MAX_KEYLENGTH]; //16 Bytes MAXKEYLENGTH 32 BYTES(SHA 256)
    CryptoPP::byte  iv[CryptoPP::AES::BLOCKSIZE];
    memcpy(key, sKey.c_str(), CryptoPP::AES::MAX_KEYLENGTH);;
    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

    std::string plain_text_std = plain_text.toStdString();
    std::string cipher_text;

    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher_text));
    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plain_text_std.c_str()), plain_text_std.length());
    stfEncryptor.MessageEnd();

    return QByteArray::fromStdString(cipher_text);
}


QString aes_decryptor(QByteArray encrypted_text, QString aes_secret_key){
    CryptoPP::SHA256 hash;
    CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
    std::string message = aes_secret_key.toStdString() + "R@nd0m!Sh17$yS4!t!";


    hash.CalculateDigest(digest, (CryptoPP::byte*)message.c_str(), message.length());

    CryptoPP::HexEncoder encoder;
    std::string sKey;
    encoder.Attach(new CryptoPP::StringSink(sKey));
    encoder.Put(digest, sizeof(digest));
    encoder.MessageEnd();

    CryptoPP::byte key[CryptoPP::AES::MAX_KEYLENGTH]; //16 Bytes MAXKEYLENGTH 32 BYTES(SHA 256)
    CryptoPP::byte  iv[CryptoPP::AES::BLOCKSIZE];
    memcpy(key, sKey.c_str(), CryptoPP::AES::MAX_KEYLENGTH);;
    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);

    CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

    std::string cipher_text = encrypted_text.toStdString();
    qDebug() << QString::fromStdString(cipher_text);
    std::string decrypted_text;
    try {
        CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted_text));
        stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cipher_text.c_str()), cipher_text.size());
        stfDecryptor.MessageEnd();
    } catch (const CryptoPP::Exception& e){ // If password does not match, it goes here.
        return "A02MA9D29P@$]MDA£@$]}¡@INVALID_P4$$W@RD¡@$[@¡";
    }
    return QString::fromStdString(decrypted_text);
}

void file_encryptor(QString file_path, QString encryption_key){
    try {
        std::string key = encryption_key.toStdString();
        std::string clear_file = file_path.toStdString();
        std::string enc_file = "data/temp_encrypted_file.nex";
        CryptoPP::FileSource source(clear_file.c_str(), false);
        CryptoPP::FileSink sink(enc_file.c_str());

        CryptoPP::AutoSeededRandomPool prng;
        unsigned char iv[CryptoPP::AES::BLOCKSIZE];
        prng.GenerateBlock(iv, sizeof(iv));
        CryptoPP::ArraySource(iv, sizeof(iv), true,
                              new CryptoPP::Redirector(sink)
                              );

        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryptor;
        encryptor.SetKeyWithIV((unsigned char*)key.c_str(), CryptoPP::AES::DEFAULT_KEYLENGTH, iv);

        CryptoPP::StreamTransformationFilter filter(encryptor);

        source.Attach(new CryptoPP::Redirector(filter));
        filter.Attach(new CryptoPP::Redirector(sink));

        const CryptoPP::word64 BLOCK_SIZE = 4096;
        CryptoPP::word64 processed = 0;

        while (!EndOfFile(source) && !source.SourceExhausted()) {
            if (filter.MaxRetrievable() == 0){
                qDebug() << "It's 0!";
            }
            source.Pump(BLOCK_SIZE);
            filter.Flush(true);
            processed += BLOCK_SIZE;
        }

        qDebug() << "Success!";
//        file.resize(file_path, file.size() - 16);
    } catch (const CryptoPP::Exception& ex) {
        qDebug() << "Failure!";
        qDebug() << QString::fromStdString(ex.GetWhat());
    }
}


void file_decryptor(QString file_path, QString file_destination, QString encryption_key){

    try {
        std::string key = encryption_key.toStdString();
        std::string clear_file = file_destination.toStdString();
        std::string enc_file = file_path.toStdString();

        CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryptor;
        CryptoPP::StreamTransformationFilter filter(decryptor);

        CryptoPP::FileSource source(enc_file.c_str(), false);
        CryptoPP::FileSink sink(clear_file.c_str());

        unsigned char iv[CryptoPP::AES::BLOCKSIZE];
        CryptoPP::ArraySink ivSink(iv, sizeof(iv));
        source.Attach(new CryptoPP::Redirector(ivSink));
        source.Pump(CryptoPP::AES::BLOCKSIZE);

        decryptor.SetKeyWithIV((unsigned char*)key.c_str(), CryptoPP::AES::DEFAULT_KEYLENGTH, iv);

        source.Attach(new CryptoPP::Redirector(filter));
        filter.Attach(new CryptoPP::Redirector(sink));

        const CryptoPP::word64 BLOCK_SIZE = 4096;
        CryptoPP::word64 processed = 0;

        while (!EndOfFile(source) && !source.SourceExhausted()) {
            source.Pump(BLOCK_SIZE);
            filter.Flush(true);
            processed += BLOCK_SIZE;
        }
        qDebug() << "Success!";
    }
        catch (const CryptoPP::Exception& ex) {
        qDebug() << "Failure!";
        qDebug() << QString::fromStdString(ex.GetWhat());
    }
}


QString create_encryption_key(QString username, QString user_hash){
    return username_based_hash(username_based_hash(username) + "465£@¡$¥,fadsf8 FE9fasmf, . F,DS.Aikmaf" + username_based_hash(user_hash) + "3!@¡$¡@$ ,fdasfIFM29fmaf$");
}


bool verify_password(QString file_key, QString user_key){
    qDebug() << file_key;
    qDebug() << user_key;
    if (file_key == user_key)
            return true;
    return false;
}

