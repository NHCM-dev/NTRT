//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_ABSTRACTAPI_H
#define NTRT_ABSTRACTAPI_H

#include "main/data/market/abstract/Instrument.h"
#include "TickerStream.h"
#include <memory>
#include <vector>

namespace trtcore
{
    class AbstractApi
    {
    public:
        virtual ~AbstractApi() = default;

        virtual int getRequestInterval() const = 0;
        virtual std::vector<trtdata::InstType> instrumentTypes() const = 0;
        virtual std::unique_ptr<TickerStream> createTickerStream() const = 0;
        virtual QFuture<std::shared_ptr<std::vector<trtdata::TickerData>>> fetchTickers(trtdata::InstType type) const = 0;
        virtual QFuture<std::vector<std::shared_ptr<trtdata::Instrument>>> fetchInstruments(trtdata::InstType type) const = 0;
        virtual std::shared_ptr<trtdata::Instrument> createInstrument(const std::string& id, trtdata::InstType type) const = 0;
        virtual trtdata::InstType parseInstrumentType(std::string_view name) const = 0;
    };
    const AbstractApi& exchangeApi(trtdata::Exchange exchange);
    std::vector<trtdata::Exchange> availableExchanges();
}

#endif //NTRT_ABSTRACTAPI_H
