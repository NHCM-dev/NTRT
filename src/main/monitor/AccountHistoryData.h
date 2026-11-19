#ifndef NTRT_ACCOUNTHISTORYDATA_H
#define NTRT_ACCOUNTHISTORYDATA_H
#include "main/data/persistence/AccountManager.h"
#include "main/trade/AccountSnapshotDef.h"
#include <QObject>
#include <QFutureWatcher>
#include <optional>

namespace trtcore
{
    class AccountHistoryData final : public QObject
    {
        trtdata::AccountManager manager;
        std::unique_ptr<trttrade::TradingAccount> account;
        QFutureWatcher<std::vector<trttrade::OrderData>> orders;
        QFutureWatcher<std::vector<trttrade::PositionHistoryData>> positions;
        AccountHistorySnapshot snapshot;
        std::optional<trttrade::AccountHistoryQuery> pending;
        trttrade::AccountHistoryQuery active;
        qint64 generation = 0;
        qint64 submittedGeneration = 0;
        bool appendPage = false;
        void finishOrders();
        void finishPositions();
    public:
        explicit AccountHistoryData(const trtdata::AccountManager& manager);
        void setQuery(const trttrade::AccountHistoryQuery& query, bool append);
        bool submit();
        const AccountHistorySnapshot& getSnapshot() const;
    };
}
#endif
