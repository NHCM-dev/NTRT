//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_INSTRUMENT_H
#define NTRT_INSTRUMENT_H
#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <QFuture>

#include "main/data/Exchanges.h"

namespace trtdata
{
    enum class InstType : uint8_t
    {
        Spot = 0,   // Currency vs Currency
        Margin = 1,     // Leverage Currency vs Currency
        Swap = 2,       // Perpetual Swap Contract
        Futures = 3,    // Futures Contract
        Option = 4,     // Option
        Events = 5      // Events Contract
    };

    constexpr std::string_view instrumentTypeName(InstType type)
    {
        switch (type)
        {
        case InstType::Spot: return "Spot";
        case InstType::Margin: return "Margin";
        case InstType::Swap: return "Perpetual";
        case InstType::Futures: return "Futures";
        case InstType::Option: return "Options";
        case InstType::Events: return "Events";
        }
        return {};
    }

    struct TickerData
    {
        InstType instType;
        std::string instId;
        double last;
        double open24h;
        double high24h;
        double low24h;
        double volCcy24h;
        double vol24h;
        std::int64_t ts;
    };

    struct InstIdentifier
    {
        Exchange exchange;
        InstType type;
        std::string id;

        std::strong_ordering operator<=>(const InstIdentifier&) const = default;
    };

    class Instrument
    {
        const InstIdentifier uniqueIdentifier;

    public:
        const std::string instId;
        const InstType instType;
        const Exchange exchange;

        virtual ~Instrument() = default;

        Instrument(const std::string& instId, InstType type, Exchange exchange)
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
