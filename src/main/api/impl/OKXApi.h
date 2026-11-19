//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_OKXAPI_H
#define NTRT_OKXAPI_H

#include "../AbstractApi.h"
#include "main/data/config/AccountConfig.h"
#include "main/trade/AccountDef.h"
#include "main/trade/AccountDataDef.h"
#include "main/market/abstract/Instrument.h"
#include "main/network/RestAPITemplate.h"

namespace trtdata::okx
{
    // 20 times per 2 seconds
    const int RATE_LIMIT_2S = 20;

    const trtnet::RestAPITemplate TICKERS{
        trtnet::HTTPMethod::GET,
        QUrl{"https://www.okx.com"},
        "/api/v5/market/tickers",
        {"instType"},
        {"instFamily"}
    };
    const trtnet::RestAPITemplate TICKER{
        trtnet::HTTPMethod::GET,
        QUrl{"https://www.okx.com"},
        "/api/v5/market/ticker",
        {"instId"}
    };

    const trtnet::RestAPITemplate CANDLES{
        trtnet::HTTPMethod::GET,
        QUrl{"https://www.okx.com"},
        "/api/v5/market/candles",
        {"instId"},
        {"bar", "limit"}
    };

    const trtnet::RestAPITemplate CANDLE_HISTORY{
        trtnet::HTTPMethod::GET,
        QUrl{"https://www.okx.com"},
        "/api/v5/market/history-candles",
        {"instId"},
        {"bar", "limit", "after"}
    };

    class OKXApi final : public trtcore::AbstractApi
    {
        static QNetworkReply* requestPrivate(const trtnet::RestAPITemplate& endpoint, const OKXCredentials& credentials,
            trttrade::TradingEnvironment environment, const QUrlQuery& query, const QByteArray& body = {});

    public:
        static QFuture<trttrade::AccountBalance> fetchBalance(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const std::string& ccy);
        static QFuture<std::vector<trttrade::FundingBalance>> fetchFundingBalance(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const std::string& ccy);
        static QFuture<std::vector<trttrade::PositionData>> fetchPositions(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query);
        static QFuture<std::vector<trttrade::OrderData>> listOrders(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query);
        static QFuture<std::vector<trttrade::OrderData>> listOrderHistory(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query);
        static QFuture<std::vector<trttrade::FillData>> listFills(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AccountQuery& query);
        static QFuture<trttrade::OrderData> fetchOrder(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::OrderIdentifier& order);
        static QFuture<trttrade::OrderResult> placeOrder(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::PlaceOrderRequest& order);
        static QFuture<trttrade::OrderResult> cancelOrder(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::OrderIdentifier& order);
        static QFuture<trttrade::OrderResult> amendOrder(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::AmendOrderRequest& order);
        static QFuture<trttrade::AccountConfiguration> fetchConfiguration(const OKXCredentials& credentials, trttrade::TradingEnvironment environment);
        static QFuture<std::vector<trttrade::LeverageData>> setLeverage(const OKXCredentials& credentials, trttrade::TradingEnvironment environment, const trttrade::LeverageRequest& request);
        std::unique_ptr<trtcore::BookStream> createBookStream() const override;

        std::unique_ptr<trtcore::CandleStream> createCandleStream() const override;

        QFuture<std::vector<trttrade::CandleData>> fetchCandleHistory(const InstIdentifier& instrument,
                                                                      trttrade::CandleTimeframe timeframe,
                                                                      TradingTimeZone zone,
                                                                      qint64 after) const override;

        static QFuture<TickerData> requestTicker(const std::string& instId);

        /**
         * @param instFamily the prefix of instId
         */
        static QFuture<std::shared_ptr<std::vector<TickerData>>> requestTickers(
            trttrade::InstType instType, const std::string& instFamily = {});

        std::vector<trttrade::InstType> instrumentTypes() const override;

        std::unique_ptr<trtcore::TickerStream> createTickerStream() const override;

        QFuture<std::vector<trttrade::CandleData>> fetchCandles(const InstIdentifier& identifier,
                                                                trttrade::CandleTimeframe timeframe,
                                                                TradingTimeZone timeZone, int limit) const override;

        QFuture<std::shared_ptr<std::vector<TickerData>>> fetchTickers(trttrade::InstType type) const override;

        QFuture<std::vector<std::shared_ptr<Instrument>>> fetchInstruments(trttrade::InstType type) const override;

        std::shared_ptr<Instrument> createInstrument(const std::string& id, trttrade::InstType type) const override;

        trttrade::InstType parseInstrumentType(std::string_view name) const override;

        int getRequestInterval() const override
        {
            return 2000 / RATE_LIMIT_2S;
        }
    };
} // trtdata

#endif //NTRT_OKXAPI_H
