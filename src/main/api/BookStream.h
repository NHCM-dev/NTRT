#ifndef NTRT_BOOKSTREAM_H
#define NTRT_BOOKSTREAM_H

#include "main/trade/TradeSnapshotDef.h"
#include <QObject>
#include <set>

namespace trtcore
{
    class BookStream : public QObject
    {
        Q_OBJECT
    public:
        using Instruments = std::set<trtdata::InstIdentifier>;
        virtual void setInstruments(const Instruments& instruments) = 0;
        virtual void stop() = 0;

    signals:
        void bookReceived(const trtdata::InstIdentifier& instrument, const trttrade::BookSnapshot& book);
    };
}
#endif
