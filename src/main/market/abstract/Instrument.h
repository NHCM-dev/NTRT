//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_INSTRUMENT_H
#define NTRT_INSTRUMENT_H
#include <QFuture>

#include "main/trade/TradeDataDef.h"

namespace trtdata
{
    class Instrument
    {
        const InstIdentifier uniqueIdentifier;

    public:
        const std::string instId;
        const trttrade::InstType instType;
        const Exchange exchange;

        virtual ~Instrument() = default;

        Instrument(const std::string& instId, trttrade::InstType type, Exchange exchange)
        : uniqueIdentifier{exchange, type, instId}, instId(instId), instType(type), exchange(exchange)
        {}

        virtual QFuture<TickerData> requestTicker() = 0;

        virtual std::string getInstTypeStr() const = 0;

        const InstIdentifier& getUniqueIdentifier() const
        {
            return uniqueIdentifier;
        }

        virtual bool operator=(Instrument& other)
        {
            return this->getUniqueIdentifier() == other.getUniqueIdentifier();
        }
    };
} // trtdata

#endif //NTRT_INSTRUMENT_H
