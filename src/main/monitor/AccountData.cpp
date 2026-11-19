#include "AccountData.h"
#include <QDateTime>
#include <QTimer>
#include <stdexcept>
#include <algorithm>
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
            } catch (const std::exception& error) { snapshot.balance.error = QString::fromUtf8(error.what()); failureCooldown.restart(); }
            emit snapshotChanged();
        });
        connect(&positions, &QFutureWatcher<std::vector<trttrade::PositionData>>::finished, this, [this]
        {
            snapshot.positions.loading = false;
            try
            {
                snapshot.positions.data = positions.result();
                snapshot.positions.timestamp = QDateTime::currentMSecsSinceEpoch();
                snapshot.positions.error.clear();
            } catch (const std::exception& error) { snapshot.positions.error = QString::fromUtf8(error.what()); failureCooldown.restart(); }
            emit snapshotChanged();
        });
        connect(&orders, &QFutureWatcher<std::vector<trttrade::OrderData>>::finished, this, &AccountData::finishOrders);
    }

    void AccountData::queryHistory(const QString& id, const trttrade::AccountHistoryQuery& query, bool append)
    {
        std::unique_ptr<AccountHistoryData>& history = histories[id];
        if (!history)
        {
            history = std::make_unique<AccountHistoryData>(manager);
            const AccountHistoryData* source = history.get();
            connect(source, &AccountHistoryData::snapshotChanged, this, [this, id, source]
            {
                snapshot.histories.insert_or_assign(id, source->getSnapshot());
                emit snapshotChanged();
            });
        }
        history->setQuery(query, append);
    }

    void AccountData::releaseHistory(const QString& id)
    {
        histories.erase(id);
        if (snapshot.histories.erase(id) != 0)
            emit snapshotChanged();
    }

    void AccountData::refreshHistory()
    {
        bool submitted = historyThrottle.isValid() && historyThrottle.elapsed() < 500;
        for (const std::pair<const QString, std::unique_ptr<AccountHistoryData>>& entry : histories)
        {
            if (!submitted && entry.second->submit()) { historyThrottle.restart(); submitted = true; }
        }
    }

    void AccountData::refresh()
    {
        refreshHistory();
        if ((lastRefresh.isValid() && lastRefresh.elapsed() < AccountSnapshot::refreshIntervalMs)
            || (lastOrderRequest.isValid() && lastOrderRequest.elapsed() < AccountSnapshot::refreshIntervalMs)
            || (failureCooldown.isValid() && failureCooldown.elapsed() < 5000)) return;
        if (snapshot.balance.loading || snapshot.positions.loading || snapshot.orders.loading || balance.isRunning() || positions.isRunning() || orders.isRunning())
            return;
        lastRefresh.restart();
        try
        {
            account = manager.createOKXAccount(trttrade::TradingEnvironment::Live);
        } catch (const std::exception&)
        {
            failureCooldown.restart();
            snapshot.balance = {};
            snapshot.positions = {};
            snapshot.orders = {};
            snapshot.balance.error = snapshot.positions.error = snapshot.orders.error = "Configure complete OKX credentials in Settings > API.";
            emit snapshotChanged();
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
            snapshot.balance.error = snapshot.positions.error = snapshot.orders.error = QString::fromUtf8(error.what()); failureCooldown.restart();
        }
        emit snapshotChanged();
    }

    void AccountData::requestOrders()
    {
        trttrade::AccountQuery query;
        query.after = cursor;
        lastOrderRequest.restart();
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
                QTimer::singleShot(AccountSnapshot::refreshIntervalMs, Qt::PreciseTimer, this, [this]
                {
                    try { requestOrders(); }
                    catch (const std::exception& error)
                    {
                        snapshot.orders.error = QString::fromUtf8(error.what());
                        snapshot.orders.loading = false;
                        failureCooldown.restart();
                        emit snapshotChanged();
                    }
                });
                return;
            }
            snapshot.orders.data = std::move(pendingOrders);
            snapshot.orders.timestamp = QDateTime::currentMSecsSinceEpoch();
            snapshot.orders.error.clear();
        } catch (const std::exception& error) { snapshot.orders.error = QString::fromUtf8(error.what()); failureCooldown.restart(); }
        snapshot.orders.loading = false;
        emit snapshotChanged();
    }

    const AccountSnapshot& AccountData::getSnapshot() const { return snapshot; }
}
