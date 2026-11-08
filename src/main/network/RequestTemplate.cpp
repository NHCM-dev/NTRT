//
// Created by NHCM on 2026/11/7.
//

#include "RequestTemplate.h"

namespace trtnet
{
    QNetworkReply* RequestTemplate::request(
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
        switch (requestMethod)
        {
        case RequestMethod::GET:
            return ntrt::netManager->get(req);

        case RequestMethod::POST:
            return ntrt::netManager->post(req, QByteArray{});

        case RequestMethod::PUT:
            return ntrt::netManager->put(req, QByteArray{});

        case RequestMethod::PATCH:
            return ntrt::netManager->sendCustomRequest(
                req, "PATCH", QByteArray{}
            );

        case RequestMethod::DELETE:
            return ntrt::netManager->deleteResource(req);

        case RequestMethod::HEAD:
            return ntrt::netManager->head(req);

        case RequestMethod::OPTIONS:
            return ntrt::netManager->sendCustomRequest(
                req, "OPTIONS", QByteArray{}
            );

        case RequestMethod::TRACE:
            return ntrt::netManager->sendCustomRequest(
                req, "TRACE", QByteArray{}
            );

        default:
            return nullptr;
        }
    }
} // trtnet