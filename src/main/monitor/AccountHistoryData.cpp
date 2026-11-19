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

    void AccountHistoryData::setQuery(const trttrade::AccountHistoryQuery& query)
    {
        if (selected == query && snapshot.error.isEmpty())
            return;

        generation++;
        selected = query;
        cursor.clear();
        cursors.clear();
        orderIds.clear();
        positionIds.clear();
        const qint64 revision = snapshot.revision + 1;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        requestEnd = std::min(query.end, now);

        std::erase_if(cache, [now](const CachedQuery& entry) { return entry.expiresAt <= now; });
        const std::deque<CachedQuery>::iterator cached = std::find_if(cache.begin(), cache.end(),
            [&query](const CachedQuery& entry) { return entry.query == query; });
        if (cached != cache.end())
        {
            snapshot = cached->result;
            requestPending = false;
        }
        else
        {
            snapshot = {};
            snapshot.loading = true;
            requestPending = true;
        }
        snapshot.revision = revision;
        emit snapshotChanged();
    }

    bool AccountHistoryData::submit()
    {
        if (!requestPending || inFlight) return false;

        requestPending = false;
        submittedGeneration = generation;
        const trttrade::AccountHistoryQuery& selectedQuery = *selected;
        try
        {
            if (selectedQuery.begin <= 0 || requestEnd < selectedQuery.begin)
                throw std::invalid_argument("Choose a valid date range.");
            if (cursor.empty())
                account = manager.createOKXAccount(trttrade::TradingEnvironment::Live);
            trttrade::AccountQuery query;
            query.instType = selectedQuery.instType;
            query.instId = selectedQuery.instId;
            query.after = cursor;
            inFlight = true;
            if (selectedQuery.positions)
            {
                if (cursor.empty()) query.after = std::to_string(requestEnd + 1);
                query.before = std::to_string(selectedQuery.begin - 1);
                positions.setFuture(account->listPositionHistory(query));
            }
            else
            {
                query.begin = std::to_string(selectedQuery.begin);
                query.end = std::to_string(requestEnd);
                query.archive = selectedQuery.archive;
                orders.setFuture(account->listOrderHistory(query));
            }
        }
        catch (const std::exception& error) { fail(error); }
        return true;
    }

    void AccountHistoryData::finishPage(std::size_t size, const std::string& nextCursor)
    {
        if (size >= static_cast<std::size_t>(trttrade::AccountQuery{}.limit))
        {
            if (nextCursor.empty() || !cursors.insert(nextCursor).second)
                throw std::runtime_error("History pagination stopped because the exchange repeated or omitted a cursor.");
            cursor = nextCursor;
            requestPending = true;
        }
        else
        {
            snapshot.loading = false;
            if (cache.size() >= 4) cache.pop_front();
            cache.push_back({*selected, snapshot, QDateTime::currentMSecsSinceEpoch() + 60000});
        }
        snapshot.revision++;
        emit snapshotChanged();
    }

    void AccountHistoryData::fail(const std::exception& error)
    {
        inFlight = false;
        snapshot.error = QString::fromUtf8(error.what());
        snapshot.loading = false;
        snapshot.revision++;
        emit snapshotChanged();
    }

    void AccountHistoryData::finishOrders()
    {
        inFlight = false;
        if (submittedGeneration != generation) return;
        try
        {
            const std::vector<trttrade::OrderData> page = orders.result();
            for (const trttrade::OrderData& row : page)
            {
                bool valid = false;
                const qint64 timestamp = QString::fromStdString(row.cTime).toLongLong(&valid);
                if (valid && timestamp >= selected->begin && timestamp <= requestEnd && orderIds.insert(row.ordId).second)
                    snapshot.orders.push_back(row);
            }
            finishPage(page.size(), page.empty() ? std::string() : page.back().ordId);
        }
        catch (const std::exception& error) { fail(error); }
    }

    void AccountHistoryData::finishPositions()
    {
        inFlight = false;
        if (submittedGeneration != generation) return;
        try
        {
            const std::vector<trttrade::PositionHistoryData> page = positions.result();
            for (const trttrade::PositionHistoryData& row : page)
            {
                bool valid = false;
                const qint64 timestamp = QString::fromStdString(row.uTime).toLongLong(&valid);
                if (valid && timestamp >= selected->begin && timestamp <= requestEnd && positionIds.emplace(row.posId, row.uTime).second)
                    snapshot.positions.push_back(row);
            }
            const std::string nextCursor = page.empty() ? std::string() : page.back().uTime;
            bool valid = false;
            const qint64 oldest = QString::fromStdString(nextCursor).toLongLong(&valid);
            finishPage(valid && oldest <= selected->begin ? 0 : page.size(), nextCursor);
        }
        catch (const std::exception& error) { fail(error); }
    }

    const AccountHistorySnapshot& AccountHistoryData::getSnapshot() const { return snapshot; }
}
