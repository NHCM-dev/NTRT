//
// Created by NHCM on 2026/11/7.
//

#include "OKXApi.h"
#include "main/data/parser/OKXParser.h"
#include "main/data/market/abstract/instimpl/OKXInstrument.h"
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

    QFuture<std::shared_ptr<std::vector<TickerData>>> OKXApi::requestTickers(InstType instType, const std::string& instFamily)
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
    std::vector<InstType> OKXApi::instrumentTypes() const
    {
        return {InstType::Spot, InstType::Swap, InstType::Futures, InstType::Option};
    }

    QFuture<std::vector<std::shared_ptr<Instrument>>> OKXApi::fetchInstruments(InstType type) const
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
                const InstType instrumentType = parseInstrumentType(readString(row, "instType"));
                instruments.push_back(createInstrument(id, instrumentType));
            }
            return instruments;
        });
    }

    std::shared_ptr<Instrument> OKXApi::createInstrument(const std::string& id, InstType type) const
    {
        return std::make_shared<OKXInstrument>(id, type);
    }

    InstType OKXApi::parseInstrumentType(std::string_view name) const
    {
        return OKXParser::readType(name);
    }
} // trtdata
