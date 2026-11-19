#ifndef NTRT_TRADINGACCOUNT_H
#define NTRT_TRADINGACCOUNT_H
#include "AccountDef.h"
#include "AccountDataDef.h"
#include <QFuture>
namespace trttrade
{
    // Invoke on a Qt event-dispatcher thread. Futures own their returned data.
    // Invalid request parameters throw before submission; transport/API errors fail the future.
    class TradingAccount
    {
    public:
        virtual ~TradingAccount() = default;
        virtual trtdata::Exchange getExchange() const = 0;
        virtual TradingEnvironment getEnvironment() const = 0;
        virtual QFuture<AccountBalance> fetchBalance(const std::string& ccy = {}) const = 0;
        virtual QFuture<std::vector<FundingBalance>> fetchFundingBalance(const std::string& ccy = {}) const = 0;
        virtual QFuture<std::vector<PositionData>> fetchPositions(const AccountQuery& query = {}) const = 0;
        virtual QFuture<std::vector<OrderData>> listOrders(const AccountQuery& query = {}) const = 0;
        virtual QFuture<std::vector<OrderData>> listOrderHistory(const AccountQuery& query = {}) const = 0;
        virtual QFuture<std::vector<FillData>> listFills(const AccountQuery& query = {}) const = 0;
        virtual QFuture<OrderData> fetchOrder(const OrderIdentifier& order) const = 0;
        virtual QFuture<OrderResult> placeOrder(const PlaceOrderRequest& order) const = 0;
        virtual QFuture<OrderResult> cancelOrder(const OrderIdentifier& order) const = 0;
        virtual QFuture<OrderResult> amendOrder(const AmendOrderRequest& order) const = 0;
        virtual QFuture<AccountConfiguration> fetchConfiguration() const = 0;
        virtual QFuture<std::vector<LeverageData>> setLeverage(const LeverageRequest& request) const = 0;
    };
}
#endif
