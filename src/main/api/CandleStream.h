#ifndef NTRT_CANDLESTREAM_H
#define NTRT_CANDLESTREAM_H
#include "main/trade/TradeDef.h"
#include "main/trade/TradeDataDef.h"
#include "main/data/core/TradingTimeDef.h"
#include <QObject>
#include <QString>
#include <vector>

namespace trtcore
{
    class CandleStream : public QObject
    {
        Q_OBJECT

    public:
        virtual void start(const trtdata::InstIdentifier& instrument, trttrade::CandleTimeframe timeframe,
                           trtdata::TradingTimeZone zone) = 0;

        virtual void stop() = 0;

        signals :


        void candlesReceived(const std::vector<trttrade::CandleData>& candles);

        void connectionChanged(const QString& error);
    };
}
#endif
