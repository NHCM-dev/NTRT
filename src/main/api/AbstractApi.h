//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_ABSTRACTAPI_H
#define NTRT_ABSTRACTAPI_H

#include "main/market/abstract/Instrument.h"
#include "TickerStream.h"
#include "main/trade/TradeDataDef.h"
#include "main/data/TradingDataConfig.h"
#include <memory>
#include <vector>

namespace trtcore
{
    class AbstractApi
    {
    public:
        virtual ~AbstractApi() = default;

        virtual int getRequestInterval() const = 0;
        virtual std::vector<trttrade::InstType> instrumentTypes() const = 0;
        virtual std::unique_ptr<TickerStream> createTickerStream() const = 0;
        virtual QFuture<std::vector<trttrade::CandleData>> fetchDailyCandles(const trtdata::InstIdentifier& identifier, trtdata::TradingDataTimeZone timeZone, int limit) const = 0;
        virtual QFuture<std::shared_ptr<std::vector<trtdata::TickerData>>> fetchTickers(trttrade::InstType type) const = 0;
        virtual QFuture<std::vector<std::shared_ptr<trtdata::Instrument>>> fetchInstruments(trttrade::InstType type) const = 0;
        virtual std::shared_ptr<trtdata::Instrument> createInstrument(const std::string& id, trttrade::InstType type) const = 0;
        virtual trttrade::InstType parseInstrumentType(std::string_view name) const = 0;
    };
    const AbstractApi& exchangeApi(trtdata::Exchange exchange);
    std::vector<trtdata::Exchange> availableExchanges();
}

#endif //NTRT_ABSTRACTAPI_H
