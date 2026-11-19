#include "AccountData.h"
#include <QDateTime>
#include <stdexcept>
namespace trtcore
{
    AccountData::AccountData(const trtdata::AccountManager& manager) : manager(manager)
    {
        connect(&balance, &QFutureWatcher<trttrade::AccountBalance>::finished, this, [this]
        {
            snapshot.balance.loading = false;
            try
            {
                snapshot.balance.data = balance.result();
                snapshot.balance.timestamp = QDateTime::currentMSecsSinceEpoch();
                snapshot.balance.error.clear();
            } catch (const std::exception& error) { snapshot.balance.error = QString::fromUtf8(error.what()); }
        });
        connect(&positions, &QFutureWatcher<std::vector<trttrade::PositionData>>::finished, this, [this]
        {
            snapshot.positions.loading = false;
            try
            {
                snapshot.positions.data = positions.result();
                snapshot.positions.timestamp = QDateTime::currentMSecsSinceEpoch();
                snapshot.positions.error.clear();
            } catch (const std::exception& error) { snapshot.positions.error = QString::fromUtf8(error.what()); }
        });
        connect(&orders, &QFutureWatcher<std::vector<trttrade::OrderData>>::finished, this, &AccountData::finishOrders);
    }

    void AccountData::refresh()
    {
        if (snapshot.balance.loading || snapshot.positions.loading || snapshot.orders.loading || balance.isRunning() || positions.isRunning() || orders.isRunning())
            return;
        try
        {
            account = manager.createOKXAccount(trttrade::TradingEnvironment::Live);
        } catch (const std::exception&)
        {
            snapshot = {};
            snapshot.balance.error = snapshot.positions.error = snapshot.orders.error = "Configure complete OKX credentials in Settings > API.";
            return;
        }
        try
        {
            snapshot.balance.loading = snapshot.positions.loading = snapshot.orders.loading = true;
            balance.setFuture(account->fetchBalance());
            positions.setFuture(account->fetchPositions());
            pendingOrders.clear();
            cursor.clear();
            requestOrders();
        } catch (const std::exception& error)
        {
            snapshot.balance.loading = snapshot.positions.loading = snapshot.orders.loading = false;
            snapshot.balance.error = snapshot.positions.error = snapshot.orders.error = QString::fromUtf8(error.what());
        }
    }

    void AccountData::requestOrders()
    {
        trttrade::AccountQuery query;
        query.after = cursor;
        orders.setFuture(account->listOrders(query));
    }

    void AccountData::finishOrders()
    {
        try
        {
            const std::vector<trttrade::OrderData> page = orders.result();
            pendingOrders.insert(pendingOrders.end(), page.begin(), page.end());
            if (page.size() == 100)
            {
                if (page.back().ordId.empty() || page.back().ordId == cursor || pendingOrders.size() > 100000)
                    throw std::runtime_error("Could not complete order pagination");
                cursor = page.back().ordId;
                requestOrders();
                return;
            }
            snapshot.orders.data = std::move(pendingOrders);
            snapshot.orders.timestamp = QDateTime::currentMSecsSinceEpoch();
            snapshot.orders.error.clear();
        } catch (const std::exception& error) { snapshot.orders.error = QString::fromUtf8(error.what()); }
        snapshot.orders.loading = false;
    }

    const AccountSnapshot& AccountData::getSnapshot() const { return snapshot; }
}
