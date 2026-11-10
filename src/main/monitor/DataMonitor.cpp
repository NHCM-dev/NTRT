//
// Created by NHCM on 2026/11/8.
//

#include "DataMonitor.h"
#include "main/data/ConfigManager.h"

namespace trtcore
{
    DataMonitor::DataMonitor(const trtdata::ConfigManager& configManager)
        : configManager(configManager)
    {
        subscriptionTimer.setSingleShot(true);
        subscriptionTimer.setInterval(150);
        QObject::connect(&configManager, &trtdata::ConfigManager::instrumentsChanged, &marketData, [this]
        {
            if (running)
                subscriptionTimer.start();
        });
        QObject::connect(&subscriptionTimer, &QTimer::timeout, &marketData, [this]
        {
            marketData.setInstruments(this->configManager.getInstruments());
        });
    }

    void DataMonitor::start()
    {
        if (running)
            return;

        running = true;
        marketData.setInstruments(configManager.getInstruments());
    }

    void DataMonitor::stop()
    {
        running = false;
        subscriptionTimer.stop();
        marketData.stop();
    }

    const MarketData& DataMonitor::getMarketData() const
    {
        return marketData;
    }
}
