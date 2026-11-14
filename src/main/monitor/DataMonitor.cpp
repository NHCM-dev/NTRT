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
        QObject::connect(&configManager, &trtdata::ConfigManager::tradingDataChanged, &candlesManager, [this]
        {
            candlesManager.setTimeZone(this->configManager.getTradingDataTimeZone());
        });
        QObject::connect(&configManager, &trtdata::ConfigManager::instrumentsChanged, &marketData, [this]
        {
            if (running)
                subscriptionTimer.start();
        });
        QObject::connect(&subscriptionTimer, &QTimer::timeout, &marketData, [this]
        {
            marketData.setInstruments(this->configManager.getInstruments());
            candlesManager.setInstruments(this->configManager.getInstruments());
            instrumentIconManager.setInstruments(this->configManager.getInstruments());
        });
    }

    void DataMonitor::start()
    {
        if (running)
            return;

        running = true;
        candlesManager.setTimeZone(configManager.getTradingDataTimeZone());
        marketData.setInstruments(configManager.getInstruments());
        candlesManager.setInstruments(configManager.getInstruments());
        instrumentIconManager.setInstruments(configManager.getInstruments());
    }

    void DataMonitor::stop()
    {
        running = false;
        subscriptionTimer.stop();
        marketData.stop();
        candlesManager.stop();
        instrumentIconManager.stop();
    }

    const MarketData& DataMonitor::getMarketData() const
    {
        return marketData;
    }

    const CandlesManager& DataMonitor::getCandlesManager() const
    {
        return candlesManager;
    }

    const InstrumentIconManager& DataMonitor::getInstrumentIconManager() const
    {
        return instrumentIconManager;
    }
}
