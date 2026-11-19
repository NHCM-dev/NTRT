#ifndef NTRT_ACCOUNTDEF_H
#define NTRT_ACCOUNTDEF_H
#include "TradeDef.h"
#include <optional>
#include <string>
#include <QtGlobal>
namespace trttrade
{
    enum class TradingEnvironment { Live, Demo };
    struct AccountQuery
    {
        std::optional<InstType> instType;
        std::string instId;
        std::string after;
        std::string before;
        int limit = 100;
        std::string begin;
        std::string end;
        bool archive = false;
    };
    struct AccountHistoryQuery
    {
        bool positions = false;
        InstType instType = InstType::Swap;
        std::string instId;
        qint64 begin = 0;
        qint64 end = 0;
        bool archive = true;
    };
    struct OrderIdentifier
    {
        std::string instId;
        std::string ordId;
        std::string clOrdId;
    };
    struct PlaceOrderRequest
    {
        std::string instId;
        MarginMode tdMode = MarginMode::Cash;
        OrderSide side = OrderSide::Buy;
        OrderType ordType = OrderType::Limit;
        std::string sz;
        std::string px;
        std::string clOrdId;
        std::string ccy;
        std::optional<PositionSide> posSide;
        std::optional<bool> reduceOnly;
        // For spot market orders: base_ccy or quote_ccy; empty uses exchange default.
        std::string tgtCcy;
    };
    struct AmendOrderRequest
    {
        OrderIdentifier order;
        std::string newSz;
        std::string newPx;
        bool cxlOnFail = false;
    };
    struct LeverageRequest
    {
        std::string instId;
        std::string ccy;
        std::string lever;
        TradeMode mgnMode = TradeMode::Cross;
        std::optional<PositionSide> posSide;
    };
}
#endif
