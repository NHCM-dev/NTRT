//
// Created by NHCM on 2026/11/7.
//

#include "RestAPITemplate.h"
#include "main/util/NetworkUtil.h"

namespace trtnet
{
    QNetworkReply* RestAPITemplate::request(
        const QUrlQuery& parameters, const QByteArray& body, const QList<QPair<QByteArray, QByteArray>>& headers
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

        if (!parameters.isEmpty())
            url.setQuery(parameters);

        QNetworkRequest req(url);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        if (!body.isEmpty())
            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        for (const QPair<QByteArray, QByteArray>& header : headers)
            req.setRawHeader(header.first, header.second);

        // Execute request
        switch (httpMethod)
        {
            case HTTPMethod::GET:
                return getNetworkManager().get(req);

            case HTTPMethod::POST:
                return getNetworkManager().post(req, body);

            case HTTPMethod::PUT:
                return getNetworkManager().put(req, body);

            case HTTPMethod::PATCH:
                return getNetworkManager().sendCustomRequest(
                    req, "PATCH", body
                );

            case HTTPMethod::DELETE:
                return getNetworkManager().deleteResource(req);

            case HTTPMethod::HEAD:
                return getNetworkManager().head(req);

            case HTTPMethod::OPTIONS:
                return getNetworkManager().sendCustomRequest(
                    req, "OPTIONS", body
                );

            case HTTPMethod::TRACE:
                return getNetworkManager().sendCustomRequest(
                    req, "TRACE", body
                );

            default:
                return nullptr;
        }
    }
} // trtnet
