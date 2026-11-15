//
// Created by NHCM on 2026/11/7.
//

#include "OKXApi.h"
#include "main/util/OKXUtil.h"
#include "OKXTickerStream.h"
#include "OKXCandleStream.h"
#include "main/trade/parser/OKXParser.h"
#include "main/market/abstract/instimpl/OKXInstrument.h"
#include "main/util/NetworkUtil.h"

namespace trtdata::okx
{
    QFuture<TickerData> OKXApi::requestTicker(const std::string& instId)
    {
        QUrlQuery query;
        query.addQueryItem("instId", QString::fromStdString(instId));

        return trtnet::replyToFuture(TICKER.request(query), [](const QByteArray& body)
        {
            return OKXParser::parseTicker(body);
        });
    }

    QFuture<std::shared_ptr<std::vector<TickerData>>> OKXApi::requestTickers(
        trttrade::InstType instType, const std::string& instFamily)
    {
        QUrlQuery query;
        query.addQueryItem("instType", QString::fromStdString(std::string(OKXParser::getInstType(instType))));
        if (!instFamily.empty())
        {
            query.addQueryItem(
                "instFamily",
                QString::fromStdString(instFamily)
            );
        }

        return trtnet::replyToFuture(TICKERS.request(query), [](const QByteArray& body)
        {
            return OKXParser::parseTickers(body);
        });
    }

    std::vector<trttrade::InstType> OKXApi::instrumentTypes() const
    {
        return {
            trttrade::InstType::Spot,
            trttrade::InstType::Swap,
            trttrade::InstType::Futures,
            trttrade::InstType::Option
        };
    }

    std::unique_ptr<trtcore::TickerStream> OKXApi::createTickerStream() const
    {
        return std::make_unique<OKXTickerStream>();
    }

    QFuture<std::shared_ptr<std::vector<TickerData>>> OKXApi::fetchTickers(trttrade::InstType type) const
    {
        return requestTickers(type);
    }

    std::unique_ptr<trtcore::CandleStream> OKXApi::createCandleStream() const
    {
        return std::make_unique<OKXCandleStream>();
    }

    QFuture<std::vector<trttrade::CandleData>> OKXApi::fetchCandleHistory(
        const InstIdentifier& instrument, trttrade::CandleTimeframe timeframe, TradingDataTimeZone zone,
        qint64 after) const
    {
        QUrlQuery query;
        query.addQueryItem("instId", QString::fromStdString(instrument.id));
        query.addQueryItem("bar", candleBar(timeframe, zone));
        query.addQueryItem("limit", "300");
        query.addQueryItem("after", QString::number(after));
        return trtnet::replyToFuture(CANDLE_HISTORY.request(query), [](const QByteArray& body)
        {
            return OKXParser::parseCandles(body);
        });
    }

    QFuture<std::vector<trttrade::CandleData>> OKXApi::fetchCandles(const InstIdentifier& identifier,
                                                                    trttrade::CandleTimeframe timeframe,
                                                                    TradingDataTimeZone timeZone, int limit) const
    {
        QUrlQuery query;
        query.addQueryItem("instId", QString::fromStdString(identifier.id));
        const QString bar = candleBar(timeframe, timeZone);
        query.addQueryItem("bar", bar);
        query.addQueryItem("limit", QString::number(limit));
        return trtnet::replyToFuture(CANDLES.request(query), [](const QByteArray& body)
        {
            return OKXParser::parseCandles(body);
        });
    }

    QFuture<std::vector<std::shared_ptr<Instrument>>> OKXApi::fetchInstruments(trttrade::InstType type) const
    {
        QUrlQuery query;
        query.addQueryItem("instType", QString::fromUtf8(OKXParser::getInstType(type)));
        return trtnet::replyToFuture(TICKERS.request(query), [this](const QByteArray& body)
        {
            const elementResult response = parseElement(body);
            OKXParser::ensureSuccess(response);
            const jsonArray rows = OKXParser::getDataField(response);
            std::vector<std::shared_ptr<Instrument>> instruments;
            instruments.reserve(rows.size());
            for (const jsonElement row : rows)
            {
                const std::string id(readString(row, "instId"));
                const trttrade::InstType instrumentType = parseInstrumentType(readString(row, "instType"));
                instruments.push_back(createInstrument(id, instrumentType));
            }
            return instruments;
        });
    }

    std::shared_ptr<Instrument> OKXApi::createInstrument(const std::string& id, trttrade::InstType type) const
    {
        return std::make_shared<OKXInstrument>(id, type);
    }

    trttrade::InstType OKXApi::parseInstrumentType(std::string_view name) const
    {
        return OKXParser::readType(name);
    }
} // trtdata
