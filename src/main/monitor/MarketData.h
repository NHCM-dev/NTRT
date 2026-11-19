//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_MARKETDATA_H
#define NTRT_MARKETDATA_H

#include "main/api/TickerStream.h"
#include "main/market/abstract/Instrument.h"
#include "main/api/BookStream.h"
#include "main/trade/TradeSnapshotDef.h"
#include <QElapsedTimer>
#include <map>
#include <memory>
#include <optional>
#include <span>

namespace trtcore
{
    class MarketData final : public QObject
    {
        Q_OBJECT
    public:
        using Identifier2SnapshotMap = MarketDataSnapshot::Identifier2SnapshotMap;

        MarketData();
        void requestCatalog(trtdata::Exchange exchange, trttrade::InstType type);

        void setInstruments(std::span<const std::shared_ptr<trtdata::Instrument>> instruments);

        void setBookInstruments(const BookStream::Instruments& instruments);

        void stop();

        MarketDataSnapshot getSnapshot() const;

    private:
        MarketDataSnapshot snapshot;
        std::map<trtdata::Exchange, std::unique_ptr<TickerStream>> streams;

        std::map<trtdata::Exchange, std::unique_ptr<BookStream>> bookStreams;

        void initStream(trtdata::Exchange exchange);

        void updateTicker(trtdata::Exchange exchange, trtdata::TickerData ticker);

        void updateConnection(trtdata::Exchange exchange, const QString& error);

        void updateError(const trtdata::InstIdentifier& identifier, const QString& error);
    signals:
        void snapshotChanged();
    };
}

#endif //NTRT_MARKETDATA_H
