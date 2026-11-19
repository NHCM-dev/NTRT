#ifndef NTRT_OKXPRIVATEENDPOINTS_H
#define NTRT_OKXPRIVATEENDPOINTS_H
#include "main/network/RestAPITemplate.h"
namespace trtdata::okx
{
    inline const trtnet::RestAPITemplate BALANCE{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/account/balance", {}, {"ccy"}};
    inline const trtnet::RestAPITemplate FUNDING_BALANCE{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/asset/balances", {}, {"ccy"}};
    inline const trtnet::RestAPITemplate POSITIONS{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/account/positions", {}, {"instType", "instId"}};
    inline const trtnet::RestAPITemplate POSITION_HISTORY{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/account/positions-history", {}, {"instType", "instId", "after", "before", "limit"}};
    inline const trtnet::RestAPITemplate ORDERS{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/trade/orders-pending", {}, {"instType", "instId", "after", "before", "limit"}};
    inline const trtnet::RestAPITemplate ORDER_HISTORY{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/trade/orders-history", {"instType"}, {"instId", "after", "before", "limit", "begin", "end"}};
    inline const trtnet::RestAPITemplate ORDER_ARCHIVE{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/trade/orders-history-archive", {"instType"}, {"instId", "after", "before", "limit", "begin", "end"}};
    inline const trtnet::RestAPITemplate FILLS{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/trade/fills", {}, {"instType", "instId", "after", "before", "limit"}};
    inline const trtnet::RestAPITemplate ORDER{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/trade/order", {"instId"}, {"ordId", "clOrdId"}};
    inline const trtnet::RestAPITemplate PLACE_ORDER{trtnet::HTTPMethod::POST, QUrl{"https://www.okx.com"}, "/api/v5/trade/order", {}, {}};
    inline const trtnet::RestAPITemplate CANCEL_ORDER{trtnet::HTTPMethod::POST, QUrl{"https://www.okx.com"}, "/api/v5/trade/cancel-order", {}, {}};
    inline const trtnet::RestAPITemplate AMEND_ORDER{trtnet::HTTPMethod::POST, QUrl{"https://www.okx.com"}, "/api/v5/trade/amend-order", {}, {}};
    inline const trtnet::RestAPITemplate CONFIGURATION{trtnet::HTTPMethod::GET, QUrl{"https://www.okx.com"}, "/api/v5/account/config", {}, {}};
    inline const trtnet::RestAPITemplate SET_LEVERAGE{trtnet::HTTPMethod::POST, QUrl{"https://www.okx.com"}, "/api/v5/account/set-leverage", {}, {}};
}
#endif
