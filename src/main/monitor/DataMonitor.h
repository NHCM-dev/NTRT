//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_DATAMONITOR_H
#define NTRT_DATAMONITOR_H

#include "MarketData.h"
#include <QTimer>

namespace trtdata
{
    class ConfigManager;
}

namespace trtcore
{
    class DataMonitor final
    {
        MarketData marketData;
        QTimer marketDataTimer;

    public:
        explicit DataMonitor();
        void start();
        void stop();
        const MarketData& getMarketData() const;
    };
}

#endif //NTRT_DATAMONITOR_H
