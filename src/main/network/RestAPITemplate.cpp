//
// Created by NHCM on 2026/11/7.
//

#include "RestAPITemplate.h"
#include "main/util/NetworkUtil.h"

namespace trtnet
{
    QNetworkReply* RestAPITemplate::request(
        const QUrlQuery& parameters
    ) const
    {
        const QList<QPair<QString, QString>> items = parameters.queryItems();

        // Validate parameters
        QSet<QString> provided;

        for (const QPair<QString, QString>& item : items)
        {
            const QString& key = item.first;
            provided.insert(key);

            if (!requiredParameters.contains(key) &&
                !optionalParameters.contains(key))
            {
                qWarning() << "Unknown parameter:" << key;
                return nullptr;
            }
        }

        // Check required parameters
        for (const QString& key : requiredParameters)
        {
            if (!provided.contains(key))
            {
                qWarning() << "Missing required parameter:" << key;
                return nullptr;
            }
        }

        // Build URL
        QUrl url = domain;
        url.setPath(endpoint);

        QUrlQuery query;

        for (const QPair<QString, QString>& item : items)
        {
            query.addQueryItem(item.first, item.second);
        }

        url.setQuery(query);

        QNetworkRequest req(url);

        // Execute request
        switch (httpMethod)
        {
            case HTTPMethod::GET:
                return getNetworkManager().get(req);

            case HTTPMethod::POST:
                return getNetworkManager().post(req, QByteArray{});

            case HTTPMethod::PUT:
                return getNetworkManager().put(req, QByteArray{});

            case HTTPMethod::PATCH:
                return getNetworkManager().sendCustomRequest(
                    req, "PATCH", QByteArray{}
                );

            case HTTPMethod::DELETE:
                return getNetworkManager().deleteResource(req);

            case HTTPMethod::HEAD:
                return getNetworkManager().head(req);

            case HTTPMethod::OPTIONS:
                return getNetworkManager().sendCustomRequest(
                    req, "OPTIONS", QByteArray{}
                );

            case HTTPMethod::TRACE:
                return getNetworkManager().sendCustomRequest(
                    req, "TRACE", QByteArray{}
                );

            default:
                return nullptr;
        }
    }
} // trtnet
