//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_RESTREQUEST_H
#define NTRT_RESTREQUEST_H

#include <cstdint>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QSet>
#include <QUrlQuery>
#include <QtNetwork/QNetworkReply>

#include "NetworkDef.h"

namespace trtnet
{
    class RestAPITemplate
    {
    public:
        const HTTPMethod httpMethod;

        // Example "www.okx.com"
        const QUrl domain;
        // Example "/api/v5/market/ticker"
        const QString endpoint;

        const QSet<QString> requiredParameters;
        const QSet<QString> optionalParameters;

        RestAPITemplate(HTTPMethod method, QUrl domain, QString endpoint, QSet<QString> required,
                        QSet<QString> optional = {})
        : httpMethod(method), domain(std::move(domain)), endpoint(std::move(endpoint)),
          requiredParameters(std::move(required)), optionalParameters(std::move(optional))
        {}

        QNetworkReply* request(const QUrlQuery& parameters, const QByteArray& body = {},
                               const QList<QPair<QByteArray, QByteArray>>& headers = {}) const;
    };
} // trtnet

#endif //NTRT_RESTREQUEST_H
