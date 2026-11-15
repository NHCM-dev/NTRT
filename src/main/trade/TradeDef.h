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
        Spot = 0, // Currency vs Currency
        Margin = 1, // Leverage Currency vs Currency
        Swap = 2, // Perpetual Swap Contract
        Futures = 3, // Futures Contract
        Option = 4, // Option
        Events = 5 // Events Contract
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
}

#endif //NTRT_TRADEDEF_H
