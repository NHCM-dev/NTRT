#include "CredentialUtil.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>

namespace trtutils
{
    static QByteArray transformCredentials(const QByteArray& input, const QByteArray& nonce, const QByteArray& key)
    {
        QByteArray result = input;
        for (qsizetype offset = 0; offset < result.size(); offset += 32)
        {
            const QByteArray block = QMessageAuthenticationCode::hash("stream" + nonce + QByteArray::number(offset / 32), key, QCryptographicHash::Sha256);
            for (qsizetype index = 0; index < block.size() && offset + index < result.size(); index++)
                result[offset + index] = result.at(offset + index) ^ block.at(index);
        }
        return result;
    }

    QByteArray encryptCredentials(const QByteArray& plaintext)
    {
        const QByteArray key = QCryptographicHash::hash("NTRT/local-credentials/v1", QCryptographicHash::Sha256);
        QByteArray nonce(16, '\0');
        for (qsizetype index = 0; index < nonce.size(); index++)
            nonce[index] = static_cast<char>(QRandomGenerator::system()->generate() & 0xff);
        const QByteArray ciphertext = transformCredentials(plaintext, nonce, key);
        const QByteArray body = "NTRTKEY1" + nonce + ciphertext;
        return body + QMessageAuthenticationCode::hash("integrity" + body, key, QCryptographicHash::Sha256);
    }

    bool decryptCredentials(const QByteArray& encrypted, QByteArray& plaintext)
    {
        if (encrypted.size() <= 56 || !encrypted.startsWith("NTRTKEY1"))
            return false;
        const QByteArray key = QCryptographicHash::hash("NTRT/local-credentials/v1", QCryptographicHash::Sha256);
        const QByteArray body = encrypted.first(encrypted.size() - 32);
        const QByteArray expected = QMessageAuthenticationCode::hash("integrity" + body, key, QCryptographicHash::Sha256);
        unsigned int difference = 0;
        for (qsizetype index = 0; index < expected.size(); index++)
            difference |= static_cast<unsigned char>(expected.at(index) ^ encrypted.at(body.size() + index));
        if (difference != 0)
            return false;
        const QByteArray nonce = body.sliced(8, 16);
        plaintext = transformCredentials(body.sliced(24), nonce, key);
        return true;
    }
}
