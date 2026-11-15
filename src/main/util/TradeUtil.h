#ifndef NTRT_TRADEUTIL_H
#define NTRT_TRADEUTIL_H

#include "main/trade/TradeDef.h"
#include <string_view>
#include <QString>

namespace trtdata
{
    struct InstIdentifier;
}

namespace trttrade
{
    // For API usage

    constexpr std::string_view marginModeStr(MarginMode marginMode, trtdata::Exchange exchange)
    {
        switch (exchange)
        {
            case trtdata::Exchange::OKX:
            default:
                switch (marginMode)
                {
                    case MarginMode::Cross:
                        return "cross";
                    case MarginMode::Isolated:
                        return "isolated";
                    default:
                        return "unknown";
                }
        }
    }

    constexpr std::string_view positionSideStr(PositionSide posSide, trtdata::Exchange exchange)
    {
        switch (exchange)
        {
            case trtdata::Exchange::OKX:
            default:
                switch (posSide)
                {
                    case PositionSide::Long:
                        return "long";
                    case PositionSide::Short:
                        return "short";
                    case PositionSide::Net:
                        return "net";
                    default:
                        return "unknown";
                }
        }
    }

    constexpr std::string_view orderSideStr(OrderSide side, trtdata::Exchange exchange)
    {
        switch (exchange)
        {
            case trtdata::Exchange::OKX:
            default:
                switch (side)
                {
                    case OrderSide::Buy:
                        return "buy";
                    case OrderSide::Sell:
                        return "sell";
                    default:
                        return "unknown";
                }
        }
    }

    constexpr std::string_view OrderTypeStr(OrderType type, trtdata::Exchange exchange)
    {
        switch (exchange)
        {
            case trtdata::Exchange::OKX:
            default:
                switch (type)
                {
                    case OrderType::Market:
                        return "market";
                    case OrderType::Limit:
                        return "limit";
                    case OrderType::PostOnly:
                        return "post_only";
                    case OrderType::FOK:
                        return "fok";
                    case OrderType::IOC:
                        return "ioc";
                    case OrderType::OptimalLimitIOC:
                        return "optimal_limit_ioc";
                    case OrderType::MMP:
                        return "mmp";
                    case OrderType::MMPAndPostOnly:
                        return "mmp_and_post_only";
                    case OrderType::RPI:
                        return "rpi";
                    default:
                        return "unknown";
                }
        }
    }

    constexpr std::string_view instStr(InstType type, trtdata::Exchange exchange)
    {
        switch (exchange)
        {
            case trtdata::Exchange::OKX:
            default:
                switch (type)
                {
                    case InstType::Spot:
                        return "SPOT";
                    case InstType::Margin:
                        return "MARGIN";
                    case InstType::Swap:
                        return "PERPETUAL";
                    case InstType::Futures:
                        return "FUTURES";
                    case InstType::Option:
                        return "OPTION";
                    case InstType::Events:
                        return "EVENTS";
                    default:
                        return "unknown";
                }
        }
    }

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
    QString instrumentLabel(const trtdata::InstIdentifier& identifier);
}


#include "main/data/Exchanges.h"

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


#endif //NTRT_TRADEUTIL_H
