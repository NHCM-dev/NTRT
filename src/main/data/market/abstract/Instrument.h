//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_INSTRUMENT_H
#define NTRT_INSTRUMENT_H
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
        Margin,     // Leverage Currency vs Currency
        Swap,       // Perpetual Swap Contract
        Futures,    // Futures Contract
        Option,     // Option
        Events      // Events Contract
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

    class Instrument
    {
    public:
        virtual ~Instrument() = default;

        Instrument(const std::string& instId, InstType type, Exchange exchange)
            : instId(std::move(instId)), instType(type), exchange(exchange)
        {}

        const std::string instId;
        const InstType instType;
        const Exchange exchange;

        virtual QFuture<TickerData> requestTicker() = 0;
        virtual std::string getInstTypeStr() const = 0;
    };
} // trtdata

#endif //NTRT_INSTRUMENT_H
