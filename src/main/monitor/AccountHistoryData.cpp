#include "AccountHistoryData.h"
#include <QDateTime>
#include <stdexcept>
#include <algorithm>

namespace trtcore
{
    AccountHistoryData::AccountHistoryData(const trtdata::AccountManager& manager) : manager(manager)
    {
        connect(&orders, &QFutureWatcher<std::vector<trttrade::OrderData>>::finished, this, &AccountHistoryData::finishOrders);
        connect(&positions, &QFutureWatcher<std::vector<trttrade::PositionHistoryData>>::finished, this, &AccountHistoryData::finishPositions);
    }

    void AccountHistoryData::setQuery(const trttrade::AccountHistoryQuery& query, bool append)
    {
        if (append && (snapshot.loading || !snapshot.hasMore)) return;
        generation++;
        appendPage = append;
        if (!append)
        {
            const qint64 revision = snapshot.revision + 1;
            snapshot = {};
            snapshot.revision = revision;
        }
        pending = query;
        snapshot.loading = true;
    }

    bool AccountHistoryData::submit()
    {
        if (!pending || orders.isRunning() || positions.isRunning()) return false;
        active = *pending;
        pending.reset();
        submittedGeneration = generation;
        try
        {
            if (active.begin <= 0 || active.end < active.begin) throw std::invalid_argument("Choose a valid date range.");
            account = manager.createOKXAccount(trttrade::TradingEnvironment::Live);
            trttrade::AccountQuery query;
            query.instType = active.instType;
            query.instId = active.instId;
            if (appendPage) query.after = snapshot.cursor;
            if (active.positions)
            {
                if (!appendPage) query.after = std::to_string(active.end + 1);
                query.before = std::to_string(active.begin - 1);
                positions.setFuture(account->listPositionHistory(query));
            }
            else
            {
                query.begin = std::to_string(active.begin);
                query.end = std::to_string(active.end);
                query.archive = active.archive;
                orders.setFuture(account->listOrderHistory(query));
            }
        }
        catch (const std::exception& error)
        {
            snapshot.error = QString::fromUtf8(error.what());
            snapshot.loading = false;
        }
        return true;
    }

    void AccountHistoryData::finishOrders()
    {
        if (submittedGeneration != generation) return;
        try
        {
            const std::vector<trttrade::OrderData> page = orders.result();
            const std::string cursor = page.empty() ? std::string() : page.back().ordId;
            snapshot.hasMore = page.size() >= 100 && !cursor.empty() && cursor != snapshot.cursor;
            snapshot.cursor = cursor;
            for (const trttrade::OrderData& row : page)
                if (std::none_of(snapshot.orders.begin(), snapshot.orders.end(), [&row](const trttrade::OrderData& existing) { return existing.ordId == row.ordId; })) snapshot.orders.push_back(row);
            snapshot.revision++;
            snapshot.error.clear();
        }
        catch (const std::exception& error) { snapshot.error = QString::fromUtf8(error.what()); }
        snapshot.loading = false;
    }

    void AccountHistoryData::finishPositions()
    {
        if (submittedGeneration != generation) return;
        try
        {
            const std::vector<trttrade::PositionHistoryData> page = positions.result();
            const std::string cursor = page.empty() ? std::string() : page.back().uTime;
            snapshot.hasMore = page.size() >= 100 && !cursor.empty() && cursor != snapshot.cursor;
            snapshot.cursor = cursor;
            for (const trttrade::PositionHistoryData& row : page)
                if (std::none_of(snapshot.positions.begin(), snapshot.positions.end(), [&row](const trttrade::PositionHistoryData& existing) { return existing.posId == row.posId && existing.uTime == row.uTime; })) snapshot.positions.push_back(row);
            snapshot.revision++;
            snapshot.error.clear();
        }
        catch (const std::exception& error) { snapshot.error = QString::fromUtf8(error.what()); }
        snapshot.loading = false;
    }

    const AccountHistorySnapshot& AccountHistoryData::getSnapshot() const { return snapshot; }
}
