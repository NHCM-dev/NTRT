#ifndef NTRT_TRADEUTIL_H
#define NTRT_TRADEUTIL_H

#include "main/trade/TradeDef.h"
#include "main/trade/AccountDataDef.h"
#include "main/trade/TradeDataDef.h"
#include <string_view>
#include <optional>
#include <vector>
#include <QString>

namespace trttrade
{
    // For display string usage

    constexpr std::string_view instDisplayStr(InstType type)
    {
        switch (type)
        {
            case InstType::Spot:
                return "Spot";
            case InstType::Margin:
                return "Margin";
            case InstType::Swap:
                return "Perpetual";
            case InstType::Futures:
                return "Futures";
            case InstType::Option:
                return "Options";
            case InstType::Events:
                return "Events";
            default:
                return "Unknown";
        }
    }
}

namespace trtutils
{
    // Best-first input; depth limits raw levels, step zero preserves native prices.
    std::vector<trttrade::BookLevel> aggregateBook(const std::vector<trttrade::BookLevel>& levels, bool bids,
                                                int depth, double step, int rows);

    std::optional<double> bookBuyShare(const std::vector<trttrade::BookLevel>& bids,
                                       const std::vector<trttrade::BookLevel>& asks, int depth);

    QString instrumentLabel(const trtdata::InstIdentifier& identifier);
}

namespace trtdata
{
    constexpr std::string_view getExchangeStr(Exchange exchange)
    {
        switch (exchange)
        {
            case Exchange::OKX:
                return "OKX";
        }

        return "";
    }

    Exchange readExchange(std::string_view name);
}



namespace trttrade
{
    QString accountHistorySide(const PositionHistoryData& position);
    QString accountTime(const std::string& value);
    QString accountNumber(const std::string& value, bool signedValue = false, int maximumDecimals = 8);
    QString accountPercent(const std::string& value);
    QString accountPositionSide(const PositionData& position);
}

#endif //NTRT_TRADEUTIL_H
