#ifndef NTRT_OKXTRADINGACCOUNT_H
#define NTRT_OKXTRADINGACCOUNT_H
#include "main/trade/TradingAccount.h"
#include "main/trade/AccountCredentials.h"
namespace trttrade
{
    class OKXTradingAccount final : public TradingAccount
    {
        trtdata::OKXCredentials credentials;
        TradingEnvironment environment;
    public:
        explicit OKXTradingAccount(trtdata::OKXCredentials credentials, TradingEnvironment environment);
        trtdata::Exchange getExchange() const override;
        TradingEnvironment getEnvironment() const override;
        QFuture<AccountBalance> fetchBalance(const std::string& ccy = {}) const override;
        QFuture<std::vector<FundingBalance>> fetchFundingBalance(const std::string& ccy = {}) const override;
        QFuture<std::vector<PositionData>> fetchPositions(const AccountQuery& query = {}) const override;
        QFuture<std::vector<OrderData>> listOrders(const AccountQuery& query = {}) const override;
        QFuture<std::vector<PositionHistoryData>> listPositionHistory(const AccountQuery& query = {}) const override;
        QFuture<std::vector<OrderData>> listOrderHistory(const AccountQuery& query = {}) const override;
        QFuture<std::vector<FillData>> listFills(const AccountQuery& query = {}) const override;
        QFuture<OrderData> fetchOrder(const OrderIdentifier& order) const override;
        QFuture<OrderResult> placeOrder(const PlaceOrderRequest& order) const override;
        QFuture<OrderResult> cancelOrder(const OrderIdentifier& order) const override;
        QFuture<OrderResult> amendOrder(const AmendOrderRequest& order) const override;
        QFuture<AccountConfiguration> fetchConfiguration() const override;
        QFuture<std::vector<LeverageData>> setLeverage(const LeverageRequest& request) const override;
    };
}
#endif
