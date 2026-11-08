//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_RESTREQUEST_H
#define NTRT_RESTREQUEST_H

#include <cstdint>
#include <QUrlQuery>
#include <QtNetwork/QNetworkReply>

namespace ntrt
{
    inline QNetworkAccessManager* netManager = nullptr;
}

namespace trtnet
{
    enum class RequestMethod : uint8_t
    {
        GET,
        POST,
        PUT,
        PATCH,
        DELETE,
        HEAD,
        OPTIONS,
        TRACE
    };

    class RequestTemplate
    {
    public:
        const RequestMethod requestMethod;

        // Example "www.okx.com"
        const QUrl domain;
        // Example "/api/v5/market/ticker"
        const QString endpoint;

        const QSet<QString> requiredParameters;
        const QSet<QString> optionalParameters;

        RequestTemplate(RequestMethod method, QUrl domain, QString endpoint, QSet<QString> required, QSet<QString> optional = {})
            : requestMethod(method), domain(std::move(domain)), endpoint(std::move(endpoint)),
              requiredParameters(std::move(required)), optionalParameters(std::move(optional))
        {
        }

        QNetworkReply* request(const QUrlQuery& parameters) const;
    };
} // trtnet

#endif //NTRT_RESTREQUEST_H
