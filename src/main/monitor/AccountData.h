#ifndef NTRT_ACCOUNTDATA_H
#define NTRT_ACCOUNTDATA_H
#include "main/trade/AccountSnapshotDef.h"
#include "main/data/persistence/AccountManager.h"
#include <QObject>
#include <QFutureWatcher>
#include <QElapsedTimer>
#include "AccountHistoryData.h"
namespace trtcore
{
    class AccountData final : public QObject
    {
        Q_OBJECT
        trtdata::AccountManager manager;
        std::unique_ptr<trttrade::TradingAccount> account;
        AccountSnapshot snapshot;
        QFutureWatcher<trttrade::AccountBalance> balance;
        QFutureWatcher<std::vector<trttrade::PositionData>> positions;
        QFutureWatcher<std::vector<trttrade::OrderData>> orders;
        std::vector<trttrade::OrderData> pendingOrders;
        std::string cursor;
        QElapsedTimer lastRefresh;
        QElapsedTimer lastOrderRequest;
        QElapsedTimer failureCooldown;
        std::map<QString, std::unique_ptr<AccountHistoryData>> histories;
        QElapsedTimer historyThrottle;
        void refreshHistory();
        void requestOrders();
        void finishOrders();
    public:
        explicit AccountData(const trtdata::AccountManager& manager);
        void refresh();
        void queryHistory(const QString& id, const trttrade::AccountHistoryQuery& query, bool refresh);
        void releaseHistory(const QString& id);
        const AccountSnapshot& getSnapshot() const;
    signals:
        void snapshotChanged();
    };
}
#endif
