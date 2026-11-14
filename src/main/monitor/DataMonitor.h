//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_DATAMONITOR_H
#define NTRT_DATAMONITOR_H

#include "MarketData.h"
#include "CandlesManager.h"
#include "InstrumentIconManager.h"
#include <QTimer>

namespace trtdata
{
    class ConfigManager;
}

namespace trtcore
{
    class DataMonitor final
    {
        const trtdata::ConfigManager& configManager;
        MarketData marketData;
        CandlesManager candlesManager;
        InstrumentIconManager instrumentIconManager;
        QTimer subscriptionTimer;
        bool running = false;

    public:
        explicit DataMonitor(const trtdata::ConfigManager& configManager);
        void start();
        void stop();
        const MarketData& getMarketData() const;
        const CandlesManager& getCandlesManager() const;
        const InstrumentIconManager& getInstrumentIconManager() const;
    };
}

#endif //NTRT_DATAMONITOR_H
