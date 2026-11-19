#ifndef NTRT_ACCOUNTDATADEF_H
#define NTRT_ACCOUNTDATADEF_H
#include <string>
#include <vector>
namespace trttrade
{
    // Exchange decimals, timestamps and optional empty values are preserved losslessly.
    struct BalanceDetail
    {
        std::string ccy;
        std::string eq;
        std::string cashBal;
        std::string availBal;
        std::string availEq;
        std::string frozenBal;
        std::string upl;
        std::string liab;
        std::string eqUsd;
    };
    struct FundingBalance
    {
        std::string ccy;
        std::string bal;
        std::string availBal;
        std::string frozenBal;
    };
    struct PositionData
    {
        std::string instType;
        std::string instId;
        std::string posId;
        std::string posCcy;
        std::string bePx;
        std::string mgnRatio;
        std::string posSide;
        std::string mgnMode;
        std::string pos;
        std::string availPos;
        std::string avgPx;
        std::string markPx;
        std::string upl;
        std::string uplRatio;
        std::string lever;
        std::string liqPx;
        std::string margin;
        std::string ccy;
        std::string cTime;
        std::string uTime;
    };
    struct OrderData
    {
        std::string instType;
        std::string instId;
        std::string ordId;
        std::string clOrdId;
        std::string tdMode;
        std::string side;
        std::string posSide;
        std::string ordType;
        std::string state;
        std::string px;
        std::string sz;
        std::string accFillSz;
        std::string avgPx;
        std::string fee;
        std::string feeCcy;
        std::string pnl;
        std::string cTime;
        std::string uTime;
    };
    struct FillData
    {
        std::string instType;
        std::string instId;
        std::string ordId;
        std::string clOrdId;
        std::string tradeId;
        std::string billId;
        std::string side;
        std::string posSide;
        std::string fillPx;
        std::string fillSz;
        std::string fee;
        std::string feeCcy;
        std::string ts;
    };
    struct OrderResult
    {
        std::string ordId;
        std::string clOrdId;
        std::string sCode;
        std::string sMsg;
        std::string reqId;
    };
    struct AccountConfiguration
    {
        std::string uid;
        std::string acctLv;
        std::string posMode;
        bool autoLoan = false;
        std::string perm;
    };
    struct LeverageData
    {
        std::string instId;
        std::string ccy;
        std::string mgnMode;
        std::string posSide;
        std::string lever;
    };
    struct AccountBalance
    {
        std::string totalEq;
        std::string adjEq;
        std::string imr;
        std::string mmr;
        std::string uTime;
        std::vector<BalanceDetail> details;
    };
}
#endif
