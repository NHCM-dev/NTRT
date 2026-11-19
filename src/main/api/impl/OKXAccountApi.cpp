#include "OKXApi.h"
#include "OKXPrivateEndpoints.h"
#include "main/util/exchange/OKXAccountUtil.h"
#include "main/util/NetworkUtil.h"
#include <QDateTime>
#include <stdexcept>
namespace trtdata::okx
{
    QNetworkReply* OKXApi::requestPrivate(const trtnet::RestAPITemplate& endpoint, const OKXCredentials& credentials,
        trttrade::TradingEnvironment environment, const QUrlQuery& query, const QByteArray& body)
    {
        if (credentials.apiKey.trimmed().isEmpty() || credentials.secretKey.trimmed().isEmpty() || credentials.passphrase.trimmed().isEmpty())
            throw std::invalid_argument("Complete OKX credentials required");
        for (const QString& value : {credentials.apiKey, credentials.secretKey, credentials.passphrase})
        {
            if (value.contains('\r') || value.contains('\n'))
                throw std::invalid_argument("Invalid credential characters");
        }

        QByteArray method;
        switch (endpoint.httpMethod)
        {
            case trtnet::HTTPMethod::GET: method = "GET"; break;
            case trtnet::HTTPMethod::POST: method = "POST"; break;
            default: throw std::invalid_argument("Unsupported private HTTP method");
        }

        QUrl url = endpoint.domain;
        url.setPath(endpoint.endpoint);
        if (!query.isEmpty())
            url.setQuery(query);
        QByteArray path = url.path(QUrl::FullyEncoded).toUtf8();
        if (url.hasQuery()) path += '?' + url.query(QUrl::FullyEncoded).toUtf8();
        const QByteArray timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).toUtf8();
        const QByteArray signature = trtdata::okx::sign(credentials.secretKey.toUtf8(), timestamp, method, path, body);
        QList<QPair<QByteArray, QByteArray>> headers{
            {"OK-ACCESS-KEY", credentials.apiKey.toUtf8()}, {"OK-ACCESS-SIGN", signature},
            {"OK-ACCESS-TIMESTAMP", timestamp}, {"OK-ACCESS-PASSPHRASE", credentials.passphrase.toUtf8()}};
        switch (environment)
        {
            case trttrade::TradingEnvironment::Demo: headers.append(QPair<QByteArray, QByteArray>{"x-simulated-trading", "1"}); break;
            case trttrade::TradingEnvironment::Live: break;
            default: throw std::invalid_argument("Invalid trading environment");
        }

        return endpoint.request(query, body, headers);
    }

    QFuture<trttrade::AccountBalance> OKXApi::fetchBalance(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const std::string& ccy)
    {
        QUrlQuery currencyQuery;
        if (!ccy.empty()) currencyQuery.addQueryItem("ccy", QString::fromStdString(ccy));
        return trtnet::replyToFuture(requestPrivate(BALANCE, credentials, environment, currencyQuery), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseOne<trttrade::AccountBalance>(bytes, trtdata::okx::parseAccountBalance);
        });
    }

    QFuture<std::vector<trttrade::FundingBalance>> OKXApi::fetchFundingBalance(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const std::string& ccy)
    {
        QUrlQuery currencyQuery;
        if (!ccy.empty()) currencyQuery.addQueryItem("ccy", QString::fromStdString(ccy));
        return trtnet::replyToFuture(requestPrivate(FUNDING_BALANCE, credentials, environment, currencyQuery), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseList<trttrade::FundingBalance>(bytes, trtdata::okx::parseFundingBalance);
        });
    }

    QFuture<std::vector<trttrade::PositionData>> OKXApi::fetchPositions(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query)
    {
        return trtnet::replyToFuture(requestPrivate(POSITIONS, credentials, environment, trtdata::okx::query(query, false)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseList<trttrade::PositionData>(bytes, trtdata::okx::parsePositionData);
        });
    }

    QFuture<std::vector<trttrade::OrderData>> OKXApi::listOrders(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query)
    {
        return trtnet::replyToFuture(requestPrivate(ORDERS, credentials, environment, trtdata::okx::query(query, true)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseList<trttrade::OrderData>(bytes, trtdata::okx::parseOrderData);
        });
    }

    QFuture<std::vector<trttrade::PositionHistoryData>> OKXApi::listPositionHistory(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query)
    {
        return trtnet::replyToFuture(requestPrivate(POSITION_HISTORY, credentials, environment, trtdata::okx::query(query, true)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseList<trttrade::PositionHistoryData>(bytes, trtdata::okx::parsePositionHistoryData);
        });
    }

    QFuture<std::vector<trttrade::OrderData>> OKXApi::listOrderHistory(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query)
    {
        if (!query.instType) throw std::invalid_argument("Instrument type required for order history");
        return trtnet::replyToFuture(requestPrivate(query.archive ? ORDER_ARCHIVE : ORDER_HISTORY, credentials, environment, trtdata::okx::query(query, true)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseList<trttrade::OrderData>(bytes, trtdata::okx::parseOrderData);
        });
    }

    QFuture<std::vector<trttrade::FillData>> OKXApi::listFills(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query)
    {
        return trtnet::replyToFuture(requestPrivate(FILLS, credentials, environment, trtdata::okx::query(query, true)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseList<trttrade::FillData>(bytes, trtdata::okx::parseFillData);
        });
    }

    QFuture<trttrade::OrderData> OKXApi::fetchOrder(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::OrderIdentifier& order)
    {
        return trtnet::replyToFuture(requestPrivate(ORDER, credentials, environment, trtdata::okx::identifier(order)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseOne<trttrade::OrderData>(bytes, trtdata::okx::parseOrderData);
        });
    }

    QFuture<trttrade::OrderResult> OKXApi::placeOrder(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::PlaceOrderRequest& order)
    {
        return trtnet::replyToFuture(requestPrivate(PLACE_ORDER, credentials, environment, {}, trtdata::okx::orderBody(order)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseOne<trttrade::OrderResult>(bytes, trtdata::okx::parseOrderResult);
        });
    }

    QFuture<trttrade::OrderResult> OKXApi::cancelOrder(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::OrderIdentifier& order)
    {
        return trtnet::replyToFuture(requestPrivate(CANCEL_ORDER, credentials, environment, {}, trtdata::okx::cancelBody(order)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseOne<trttrade::OrderResult>(bytes, trtdata::okx::parseOrderResult);
        });
    }

    QFuture<trttrade::OrderResult> OKXApi::amendOrder(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AmendOrderRequest& order)
    {
        return trtnet::replyToFuture(requestPrivate(AMEND_ORDER, credentials, environment, {}, trtdata::okx::amendBody(order)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseOne<trttrade::OrderResult>(bytes, trtdata::okx::parseOrderResult);
        });
    }

    QFuture<trttrade::AccountConfiguration> OKXApi::fetchConfiguration(const OKXCredentials& credentials, trttrade::TradingEnvironment environment)
    {
        return trtnet::replyToFuture(requestPrivate(CONFIGURATION, credentials, environment, {}), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseOne<trttrade::AccountConfiguration>(bytes, trtdata::okx::parseAccountConfiguration);
        });
    }

    QFuture<std::vector<trttrade::LeverageData>> OKXApi::setLeverage(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::LeverageRequest& request)
    {
        return trtnet::replyToFuture(requestPrivate(SET_LEVERAGE, credentials, environment, {}, trtdata::okx::leverageBody(request)), [](const QByteArray& bytes)
        {
            return trtdata::okx::parseList<trttrade::LeverageData>(bytes, trtdata::okx::parseLeverageData);
        });
    }
}
