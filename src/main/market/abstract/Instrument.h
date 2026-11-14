//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_INSTRUMENT_H
#define NTRT_INSTRUMENT_H
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <QFuture>

#include "main/data/Exchanges.h"
#include "main/trade/TradeDef.h"

namespace trtdata
{
    struct TickerData
    {
        trttrade::InstType instType;
        std::string instId;
        double last;
        double open24h;
        double high24h;
        double low24h;
        double volCcy24h;
        double vol24h;
        std::int64_t ts;
        std::optional<double> previousLast{};
    };

    struct InstIdentifier
    {
        Exchange exchange;
        trttrade::InstType type;
        std::string id;

        std::strong_ordering operator<=>(const InstIdentifier&) const = default;
    };

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
        {
        }

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
