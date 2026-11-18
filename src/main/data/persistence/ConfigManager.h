//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_CONFIGMANAGER_H
#define NTRT_CONFIGMANAGER_H
#include <memory>
#include <span>
#include <vector>
#include <QObject>

#include "main/data/persistence/AppearanceManager.h"
#include "main/data/persistence/WorkspaceManager.h"
#include "../../core/TradingTimeDef.h"
#include "main/util/FileUtil.h"
#include "main/market/abstract/Instrument.h"

#include "main/data/config/MarketConfig.h"
#include "main/data/config/ExchangeConfig.h"

namespace trtdata
{
    inline static const QString DEFAULT_MARKET_CONFIG = "market.json";
    inline static const QString DEFAULT_EXCHANGE_CONFIG = "exchange.json";
    inline static const QString DEFAULT_DATA_PATH = "data/";

    class ConfigManager final : public QObject
    {
        Q_OBJECT

        QString dataPath;

        MarketConfig marketConfig;
        ExchangeConfig exchangeConfig;
        AppearanceManager appearanceManager;
        WorkspaceManager workspaceManager;


        MarketConfig defaultMarketConfig();

        ExchangeConfig defaultExchangeConfig();

    public:
        ConfigManager(QString dataPath = trtutils::relativePath(DEFAULT_DATA_PATH))
        : dataPath(dataPath), appearanceManager(dataPath), workspaceManager(dataPath)
        {}

        /**
         * Need to be manually called after ConfigManager initialized
         */
        bool loadAll();

        bool saveAll();

        /**
         * this function should be called before loadAll() to create default config if there none exist
         * @return true if there is at least one config not exists yet
         */
        bool tryInitOnce(
            const QString& marketFileName = DEFAULT_MARKET_CONFIG,
            const QString& exchangeFileName = DEFAULT_EXCHANGE_CONFIG);

        bool loadMarketConfig(const QString& fileName = DEFAULT_MARKET_CONFIG);

        bool loadExchangeConfig(const QString& fileName = DEFAULT_EXCHANGE_CONFIG);

        bool saveMarketConfig(const QString& fileName = DEFAULT_MARKET_CONFIG);

        bool saveExchangeConfig(const QString& fileName = DEFAULT_EXCHANGE_CONFIG);

        std::span<const std::shared_ptr<Instrument>> getInstruments() const;

        std::span<const std::shared_ptr<Instrument>> getInstruments(int from, int to) const;

        Instrument& getInstrument(int index);

        const ExchangeConfig* getExchangeConfig();

        /**
         * @return index of instrument in list
         */
        int addInstrument(const Instrument& instrument);

        bool removeInstrument(const Instrument& instrument);

        bool removeInstrument(const std::string& instId);

        bool removeInstrument(int index);

        bool replaceInstruments(std::vector<std::shared_ptr<Instrument>> instruments);

        bool moveInstrument(int from, int to);

        Exchange getExchange();

        void setExchange(Exchange exchange);

        AppearanceManager* getAppearanceManager();

        WorkspaceManager* getWorkspaceManager();

        TradingTimeZone getTradingDataTimeZone() const;

        bool setTradingDataTimeZone(TradingTimeZone timeZone);

        const std::string& getDisplayTimeZone() const;

        bool setDisplayTimeZone(const std::string& zone);

        int instrumentsCount();

        signals :


        void instrumentsChanged();

        void tradingDataChanged();
    };
} // trtdata

#endif //NTRT_CONFIGMANAGER_H
