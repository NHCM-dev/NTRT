#ifndef NTRT_ACCOUNTHISTORYDATA_H
#define NTRT_ACCOUNTHISTORYDATA_H
#include "main/data/persistence/AccountManager.h"
#include "main/trade/AccountSnapshotDef.h"
#include <QObject>
#include <QFutureWatcher>
#include <optional>
#include <deque>
#include <exception>
#include <set>
#include <string>
#include <utility>

namespace trtcore
{
    class AccountHistoryData final : public QObject
    {
        Q_OBJECT
        trtdata::AccountManager manager;
        std::unique_ptr<trttrade::TradingAccount> account;
        QFutureWatcher<std::vector<trttrade::OrderData>> orders;
        QFutureWatcher<std::vector<trttrade::PositionHistoryData>> positions;
        AccountHistorySnapshot snapshot;
        struct CachedQuery
        {
            trttrade::AccountHistoryQuery query;
            AccountHistorySnapshot result;
            qint64 expiresAt;
        };
        std::deque<CachedQuery> cache;
        std::optional<trttrade::AccountHistoryQuery> selected;
        std::set<std::string> cursors;
        std::set<std::string> orderIds;
        std::set<std::pair<std::string, std::string>> positionIds;
        std::string cursor;
        qint64 requestEnd = 0;
        qint64 generation = 0;
        qint64 submittedGeneration = 0;
        bool requestPending = false;
        bool inFlight = false;
        void finishPage(std::size_t size, const std::string& nextCursor);
        void fail(const std::exception& error);
        void finishOrders();
        void finishPositions();
    public:
        explicit AccountHistoryData(const trtdata::AccountManager& manager);
        void setQuery(const trttrade::AccountHistoryQuery& query);
        bool submit();
        const AccountHistorySnapshot& getSnapshot() const;
    signals:
        void snapshotChanged();
    };
}
#endif
