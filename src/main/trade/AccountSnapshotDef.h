#ifndef NTRT_ACCOUNTSNAPSHOTDEF_H
#define NTRT_ACCOUNTSNAPSHOTDEF_H
#include "AccountDataDef.h"
#include <QString>
#include <QtGlobal>
namespace trtcore
{
    template<class T> struct AccountReadSnapshot
    {
        T data;
        QString error;
        qint64 timestamp = 0;
        bool loading = false;
    };
    struct AccountSnapshot
    {
        static constexpr int refreshIntervalMs = 500;
        AccountReadSnapshot<trttrade::AccountBalance> balance;
        AccountReadSnapshot<std::vector<trttrade::PositionData>> positions;
        AccountReadSnapshot<std::vector<trttrade::OrderData>> orders;
    };
}
#endif
