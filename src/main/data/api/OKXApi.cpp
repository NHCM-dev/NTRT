//
// Created by NHCM on 2026/11/7.
//

#include "OKXApi.h"
#include "main/data/parser/OKXParser.h"
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
} // trtdata
