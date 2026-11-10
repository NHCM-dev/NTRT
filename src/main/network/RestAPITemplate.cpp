//
// Created by NHCM on 2026/11/7.
//

#include "RestAPITemplate.h"

namespace trtnet
{
    QNetworkReply* RestAPITemplate::request(
        const QUrlQuery& parameters
    ) const
    {
        if (!ntrt::netManager)
        {
            qWarning() << "Network manager is not initialized";
            return nullptr;
        }

        const auto items = parameters.queryItems();

        // Validate parameters
        QSet<QString> provided;

        for (const auto& [key, value] : items)
        {
            provided.insert(key);

            if (!requiredParameters.contains(key) &&
                !optionalParameters.contains(key))
            {
                qWarning() << "Unknown parameter:" << key;
                return nullptr;
            }
        }

        // Check required parameters
        for (const auto& key : requiredParameters)
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

        for (const auto& [key, value] : items)
        {
            query.addQueryItem(key, value);
        }

        url.setQuery(query);

        QNetworkRequest req(url);

        // Execute request
        switch (httpMethod)
        {
        case HTTPMethod::GET:
            return ntrt::netManager->get(req);

        case HTTPMethod::POST:
            return ntrt::netManager->post(req, QByteArray{});

        case HTTPMethod::PUT:
            return ntrt::netManager->put(req, QByteArray{});

        case HTTPMethod::PATCH:
            return ntrt::netManager->sendCustomRequest(
                req, "PATCH", QByteArray{}
            );

        case HTTPMethod::DELETE:
            return ntrt::netManager->deleteResource(req);

        case HTTPMethod::HEAD:
            return ntrt::netManager->head(req);

        case HTTPMethod::OPTIONS:
            return ntrt::netManager->sendCustomRequest(
                req, "OPTIONS", QByteArray{}
            );

        case HTTPMethod::TRACE:
            return ntrt::netManager->sendCustomRequest(
                req, "TRACE", QByteArray{}
            );

        default:
            return nullptr;
        }
    }
} // trtnet