#ifndef NTRT_TICKERSTREAM_H
#define NTRT_TICKERSTREAM_H

#include "main/data/market/abstract/Instrument.h"
#include <set>

namespace trtcore
{
    class TickerStream : public QObject
    {
        Q_OBJECT

    public:
        using Instruments = std::set<trtdata::InstIdentifier>;

        virtual void setInstruments(const Instruments& instruments) = 0;
        virtual void stop() = 0;

    signals:
        void tickerReceived(const trtdata::TickerData& ticker);
        void connectionChanged(const QString& error);
        void subscriptionFailed(const trtdata::InstIdentifier& identifier, const QString& error);
    };
}

#endif //NTRT_TICKERSTREAM_H
