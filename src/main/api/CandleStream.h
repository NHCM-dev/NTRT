#ifndef NTRT_CANDLESTREAM_H
#define NTRT_CANDLESTREAM_H
#include "main/market/abstract/Instrument.h"
#include "main/trade/CandleTimeframe.h"
#include "main/trade/TradeDataDef.h"
#include "main/data/TradingDataConfig.h"
#include <QObject>
#include <vector>

namespace trtcore
{
    class CandleStream : public QObject
    {
        Q_OBJECT

    public:
        virtual void start(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe,
                           trtdata::TradingDataTimeZone zone) = 0;

        virtual void stop() = 0;

        signals :


        void candlesReceived(const std::vector<trttrade::CandleData>& candles);

        void connectionChanged(const QString& error);
    };
}
#endif
