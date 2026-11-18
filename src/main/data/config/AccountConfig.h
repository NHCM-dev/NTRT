#ifndef NTRT_ACCOUNTCONFIGDEF_H
#define NTRT_ACCOUNTCONFIGDEF_H

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
