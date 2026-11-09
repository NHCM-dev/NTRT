//
// Created by NHCM on 2026/11/8.
//

#include "DataMonitor.h"

#include "main/NTRT.h"
#include "main/data/ConfigManager.h"

namespace trtcore
{
    DataMonitor::DataMonitor()
    {
        QObject::connect(&marketDataTimer, &QTimer::timeout, &marketData, [this]
        {
            marketData.setInstruments(ntrt::configManager->getInstruments());
            marketData.updateNext();
        });
    }

    void DataMonitor::start()
    {
        marketData.setInstruments(ntrt::configManager->getInstruments());
        marketData.updateNext();
        marketDataTimer.start(200);
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
