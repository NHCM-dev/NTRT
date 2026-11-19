#ifndef NTRT_ACCOUNTCREDENTIALS_H
#define NTRT_ACCOUNTCREDENTIALS_H

#include <QString>

namespace trtdata
{
    enum class OKXCredentialField { ApiKey, SecretKey, Passphrase };

    struct OKXCredentials
    {
        QString apiKey;
        QString secretKey;
        QString passphrase;
    };
}

#endif
