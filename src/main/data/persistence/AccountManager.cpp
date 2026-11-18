#include "AccountManager.h"
#include "main/util/CredentialUtil.h"
#include "main/util/JsonUtil.h"
#include "main/util/FileUtil.h"
#include <QFile>
#include <QFileInfo>

namespace trtdata
{
    AccountManager::AccountManager(const QString& dataPath)
        : credentialPath(trtutils::appendPath(dataPath, "okx-credentials.dat"))
    {}

    bool AccountManager::loadOKXCredentials(OKXCredentials& credentials, bool& found) const
    {
        found = QFileInfo::exists(credentialPath);
        if (!found)
        {
            credentials = {};
            return true;
        }
        QByteArray bytes;
        if (!trtutils::decryptCredentials(trtutils::readAllBytes(credentialPath), bytes))
            return false;
        try
        {
            const elementResult json = parseElement(bytes);
            OKXCredentials loaded{QString::fromStdString(std::string(readString(json, "apiKey"))),
                QString::fromStdString(std::string(readString(json, "secretKey"))),
                QString::fromStdString(std::string(readString(json, "passphrase")))};
            credentials = std::move(loaded);
            return true;
        } catch (const simdjson::simdjson_error&)
        {
            return false;
        }
    }

    bool AccountManager::importOKXCredentials(const OKXCredentials& credentials)
    {
        if (credentials.apiKey.trimmed().isEmpty() || credentials.secretKey.trimmed().isEmpty() || credentials.passphrase.trimmed().isEmpty())
            return false;
        return saveOKXCredentials(credentials);
    }

    bool AccountManager::setOKXCredential(OKXCredentialField field, const QString& value)
    {
        if (value.trimmed().isEmpty())
            return false;
        OKXCredentials loaded;
        bool found = false;
        if (!loadOKXCredentials(loaded, found))
            return false;
        switch (field)
        {
            case OKXCredentialField::ApiKey: loaded.apiKey = value.trimmed(); break;
            case OKXCredentialField::SecretKey: loaded.secretKey = value.trimmed(); break;
            case OKXCredentialField::Passphrase: loaded.passphrase = value; break;
            default: return false;
        }
        return saveOKXCredentials(loaded);
    }

    bool AccountManager::saveOKXCredentials(const OKXCredentials& credentials)
    {
        const std::string key = credentials.apiKey.trimmed().toStdString();
        const std::string secret = credentials.secretKey.trimmed().toStdString();
        const std::string passphrase = credentials.passphrase.toStdString();
        jsonBuilder builder = newBuilder();
        builder.start_object();
        builder.append_key_value("apiKey", std::string_view(key));
        builder.append_comma();
        builder.append_key_value("secretKey", std::string_view(secret));
        builder.append_comma();
        builder.append_key_value("passphrase", std::string_view(passphrase));
        builder.end_object();
        const std::string_view json = builder.view();
        return trtutils::writeAllBytesAtomically(credentialPath,
            trtutils::encryptCredentials(QByteArray(json.data(), static_cast<qsizetype>(json.size()))));
    }

    bool AccountManager::removeOKXCredentials()
    {
        return !QFileInfo::exists(credentialPath) || QFile::remove(credentialPath);
    }
}
