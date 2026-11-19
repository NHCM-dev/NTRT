#ifndef NTRT_ACCOUNTSNAPSHOTDEF_H
#define NTRT_ACCOUNTSNAPSHOTDEF_H
#include "AccountDataDef.h"
#include <QString>
#include <QtGlobal>
#include <map>
namespace trtcore
{
    template<class T> struct AccountReadSnapshot
    {
        T data;
        QString error;
        qint64 timestamp = 0;
        bool loading = false;
    };
    struct AccountHistorySnapshot
    {
        std::vector<trttrade::OrderData> orders;
        std::vector<trttrade::PositionHistoryData> positions;
        QString error;
        qint64 revision = 0;
        bool loading = false;
        bool hasMore = false;
        std::string cursor;
    };
    struct AccountSnapshot
    {
        static constexpr int refreshIntervalMs = 500;
        std::map<QString, AccountHistorySnapshot> histories;
        AccountReadSnapshot<trttrade::AccountBalance> balance;
        AccountReadSnapshot<std::vector<trttrade::PositionData>> positions;
        AccountReadSnapshot<std::vector<trttrade::OrderData>> orders;
    };
}
#endif
