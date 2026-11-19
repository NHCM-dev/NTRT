#ifndef NTRT_ACCOUNTMANAGER_H
#define NTRT_ACCOUNTMANAGER_H

#include "main/data/config/AccountConfig.h"
#include "main/trade/TradingAccount.h"
#include <memory>

namespace trtdata
{
    class AccountManager final
    {
        QString credentialPath;

        bool saveOKXCredentials(const OKXCredentials& credentials);

    public:
        explicit AccountManager(const QString& dataPath);
        bool loadOKXCredentials(OKXCredentials& credentials, bool& found) const;
        bool importOKXCredentials(const OKXCredentials& credentials);
        bool setOKXCredential(OKXCredentialField field, const QString& value);
        bool removeOKXCredentials();
        std::unique_ptr<trttrade::TradingAccount> createOKXAccount(trttrade::TradingEnvironment environment) const;
    };
}

#endif
