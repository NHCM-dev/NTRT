//
// Created by NHCM on 2026/11/14.
//

#ifndef NTRT_TRADEDEF_H
#define NTRT_TRADEDEF_H

#include <cstdint>

#include "main/data/Exchanges.h"

namespace trttrade
{
    using leverage = float;

    enum class InstType : uint8_t
    {
        Spot = 0,   // Currency vs Currency
        Margin = 1,     // Leverage Currency vs Currency
        Swap = 2,       // Perpetual Swap Contract
        Futures = 3,    // Futures Contract
        Option = 4,     // Option
        Events = 5      // Events Contract
    };

    enum class MarginMode : uint8_t
    {
        Isolated = 0,
        Cross = 1,
        Cash = 2,
        SpotIsolated = 3,
        // (only applicable to SPOT lead trading, tdMode should be spot_isolated for SPOT lead trading.)
    };

    enum class TradeMode : uint8_t
    {
        Cross = 0,
        Isolated = 1
    };

    enum class PositionSide : uint8_t
    {
        Long = 0,
        Short = 1,
        Net = 2
    };

    enum class OrderSide : uint8_t
    {
        Buy = 0,
        Sell = 1
    };

    enum class OrderType : uint8_t
    {
        Market = 0,
        Limit = 1,
        PostOnly = 2,
        FOK = 3, // Fill-or-kill
        IOC = 4, // Immediate-or-cancel
        OptimalLimitIOC = 5,
        /**
        * Places a limit order at the maximum buy price (upper price limit) for buy orders,
        * or the minimum sell price (lower price limit) for sell orders,
        * as defined by the exchange's price limit bands.
        * Any unfilled portion is immediately cancelled (IOC).
        * Applicable only to Expiry Futures and Perpetual Futures.
        **/
        MMP = 6, // Market Maker Protection (only applicable to Option in Portfolio Margin mode)
        MMPAndPostOnly = 7,
        RPI = 8, // Retail Price Improvement order
    };

    // For API usage

    constexpr std::string_view marginModeStr(MarginMode marginMode, trtdata::Exchange exchange)
    {
        switch(exchange)
        {
        case trtdata::Exchange::OKX:
        default:
            switch(marginMode)
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
        switch(exchange)
        {
        case trtdata::Exchange::OKX:
        default:
            switch(posSide)
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
        switch(exchange)
        {
        case trtdata::Exchange::OKX:
        default:
            switch(side)
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
        switch(exchange)
        {
        case trtdata::Exchange::OKX:
        default:
            switch(type)
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

using namespace trttrade;

#endif //NTRT_TRADEDEF_H
