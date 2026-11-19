#ifndef NTRT_ACCOUNTDATA_H
#define NTRT_ACCOUNTDATA_H
#include "main/trade/AccountSnapshotDef.h"
#include "main/data/persistence/AccountManager.h"
#include <QObject>
#include <QFutureWatcher>
namespace trtcore
{
    class AccountData final : public QObject
    {
        trtdata::AccountManager manager;
        std::unique_ptr<trttrade::TradingAccount> account;
        AccountSnapshot snapshot;
        QFutureWatcher<trttrade::AccountBalance> balance;
        QFutureWatcher<std::vector<trttrade::PositionData>> positions;
        QFutureWatcher<std::vector<trttrade::OrderData>> orders;
        std::vector<trttrade::OrderData> pendingOrders;
        std::string cursor;
        void requestOrders();
        void finishOrders();
    public:
        explicit AccountData(const trtdata::AccountManager& manager);
        void refresh();
        const AccountSnapshot& getSnapshot() const;
    };
}
#endif
