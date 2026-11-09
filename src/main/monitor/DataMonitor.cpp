//
// Created by NHCM on 2026/11/8.
//

#include "DataMonitor.h"

namespace trtcore
{
    DataMonitor::DataMonitor()
    {
        QObject::connect(&marketDataTimer, &QTimer::timeout, &marketData, [this]
        {
            marketData.updateNext();
        });
    }

    void DataMonitor::start()
    {
        marketData.updateNext();
        marketDataTimer.start(MarketData::requestInterval);
    }

    void DataMonitor::stop()
    {
        marketDataTimer.stop();
    }

    const MarketData& DataMonitor::getMarketData() const
    {
        return marketData;
    }
}
