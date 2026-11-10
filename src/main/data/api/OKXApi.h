//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_OKXAPI_H
#define NTRT_OKXAPI_H

#include "AbstractApi.h"
#include "main/data/market/abstract/Instrument.h"
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

    class OKXApi final : public trtcore::AbstractApi
    {
    public:
        static QFuture<TickerData> requestTicker(const std::string& instId);
        /**
         * @param instFamily the prefix of instId
         */
        static QFuture<std::shared_ptr<std::vector<TickerData>>> requestTickers(InstType instType, const std::string& instFamily = {});

        std::vector<InstType> instrumentTypes() const override;
        std::unique_ptr<trtcore::TickerStream> createTickerStream() const override;
        QFuture<std::shared_ptr<std::vector<TickerData>>> fetchTickers(InstType type) const override;
        QFuture<std::vector<std::shared_ptr<Instrument>>> fetchInstruments(InstType type) const override;
        std::shared_ptr<Instrument> createInstrument(const std::string& id, InstType type) const override;
        InstType parseInstrumentType(std::string_view name) const override;

        int getRequestInterval() const override
        {
            return 2000 / RATE_LIMIT_2S;
        }
    };
} // trtdata

#endif //NTRT_OKXAPI_H
