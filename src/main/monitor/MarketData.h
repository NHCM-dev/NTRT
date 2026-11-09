//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_MARKETDATA_H
#define NTRT_MARKETDATA_H

#include "main/data/market/abstract/Instrument.h"
#include <QElapsedTimer>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace trtcore
{
    struct MarketSnapshot
    {
        trtdata::Exchange exchange;
        std::optional<trtdata::TickerData> ticker;
        QString error;
    };

    class MarketData final : public QObject
    {
        using InstrumentPtr = std::shared_ptr<trtdata::Instrument>;

        std::vector<InstrumentPtr> instruments;
        std::map<InstrumentPtr, MarketSnapshot> snapshots;
        QElapsedTimer lastRequest;
        std::size_t nextInstrument = 0;
        int requestInterval = 200;
        bool pending = false;

    public:
        void setInstruments(std::span<const InstrumentPtr> instruments);
        void updateNext();
        const MarketSnapshot* getSnapshot(const InstrumentPtr& instrument) const;
    };
}

#endif //NTRT_MARKETDATA_H
