#ifndef NTRT_CREDENTIALUTIL_H
#define NTRT_CREDENTIALUTIL_H

#include <QByteArray>

namespace trtutils
{
    // Portable reversible protection with an application key, not a user-held secret.
    QByteArray encryptCredentials(const QByteArray& plaintext);
    bool decryptCredentials(const QByteArray& encrypted, QByteArray& plaintext);
}

#endif
