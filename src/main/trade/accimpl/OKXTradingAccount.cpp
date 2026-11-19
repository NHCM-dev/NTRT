#include "OKXTradingAccount.h"
#include "main/api/impl/OKXApi.h"
namespace trttrade
{
    OKXTradingAccount::OKXTradingAccount(trtdata::OKXCredentials credentials, TradingEnvironment environment)
        : credentials(std::move(credentials)), environment(environment)
    {}
    trtdata::Exchange OKXTradingAccount::getExchange() const { return trtdata::Exchange::OKX; }
    TradingEnvironment OKXTradingAccount::getEnvironment() const { return environment; }
    QFuture<AccountBalance> OKXTradingAccount::fetchBalance(const std::string& ccy) const
    {
        return trtdata::okx::OKXApi::fetchBalance(credentials, environment, ccy);
    }

    QFuture<std::vector<FundingBalance>> OKXTradingAccount::fetchFundingBalance(const std::string& ccy) const
    {
        return trtdata::okx::OKXApi::fetchFundingBalance(credentials, environment, ccy);
    }

    QFuture<std::vector<PositionData>> OKXTradingAccount::fetchPositions(const AccountQuery& query) const
    {
        return trtdata::okx::OKXApi::fetchPositions(credentials, environment, query);
    }

    QFuture<std::vector<OrderData>> OKXTradingAccount::listOrders(const AccountQuery& query) const
    {
        return trtdata::okx::OKXApi::listOrders(credentials, environment, query);
    }

    QFuture<std::vector<PositionHistoryData>> OKXTradingAccount::listPositionHistory(const AccountQuery& query) const
    {
        return trtdata::okx::OKXApi::listPositionHistory(credentials, environment, query);
    }

    QFuture<std::vector<OrderData>> OKXTradingAccount::listOrderHistory(const AccountQuery& query) const
    {
        return trtdata::okx::OKXApi::listOrderHistory(credentials, environment, query);
    }

    QFuture<std::vector<FillData>> OKXTradingAccount::listFills(const AccountQuery& query) const
    {
        return trtdata::okx::OKXApi::listFills(credentials, environment, query);
    }

    QFuture<OrderData> OKXTradingAccount::fetchOrder(const OrderIdentifier& order) const
    {
        return trtdata::okx::OKXApi::fetchOrder(credentials, environment, order);
    }

    QFuture<OrderResult> OKXTradingAccount::placeOrder(const PlaceOrderRequest& order) const
    {
        return trtdata::okx::OKXApi::placeOrder(credentials, environment, order);
    }

    QFuture<OrderResult> OKXTradingAccount::cancelOrder(const OrderIdentifier& order) const
    {
        return trtdata::okx::OKXApi::cancelOrder(credentials, environment, order);
    }

    QFuture<OrderResult> OKXTradingAccount::amendOrder(const AmendOrderRequest& order) const
    {
        return trtdata::okx::OKXApi::amendOrder(credentials, environment, order);
    }

    QFuture<AccountConfiguration> OKXTradingAccount::fetchConfiguration() const
    {
        return trtdata::okx::OKXApi::fetchConfiguration(credentials, environment);
    }

    QFuture<std::vector<LeverageData>> OKXTradingAccount::setLeverage(const LeverageRequest& request) const
    {
        return trtdata::okx::OKXApi::setLeverage(credentials, environment, request);
    }
}
