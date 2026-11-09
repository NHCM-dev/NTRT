//
// Created by NHCM on 2026/11/7.
//

#include "ConfigManager.h"

#include "main/util/JsonUtil.h"
#include "main/util/FileUtil.h"
#include "main/data/market/abstract/instimpl/OKXInstrument.h"
#include "main/data/parser/OKXParser.h"
#include <algorithm>
#include <QDir>
#include <QFileInfo>

namespace trtdata
{
    Exchange ConfigManager::readExchange(std::string_view name)
    {
        for (Exchange exchange : {Exchange::OKX})
        {
            if (getExchangeStr(exchange) == name)
                return exchange;
        }
        throw std::invalid_argument("Unknown exchange: " + std::string(name));
    }

    bool ConfigManager::writeConfig(const QString& path, std::string_view json)
    {
        const std::string formatted = prettyJson(json);
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
            return false;
        return trtutils::writeAllBytes(path, QByteArray(formatted.data(), static_cast<qsizetype>(formatted.size())));
    }

    MarketConfig ConfigManager::defaultMarketConfig()
    {
        return {};
    }
    ExchangeConfig ConfigManager::defaultExchangeConfig()
    {
        return {DEFAULT_EXCHANGE_CONFIG.toStdString(), Exchange::OKX};
    }

    bool ConfigManager::loadAll()
    {
        bool success = true;
        success &= loadMarketConfig();
        success &= loadExchangeConfig();
        return success;
    }

    bool ConfigManager::saveAll()
    {
        bool success = true;
        success &= saveMarketConfig();
        success &= saveExchangeConfig();
        return success;
    }

    bool ConfigManager::tryInitOnce(
        const QString& marketFileName,
        const QString& exchangeFileName)
    {
        bool notExist = false;
        QString marketPath = trtutils::appendPath(dataPath, marketFileName);
        if (!trtutils::exists(marketPath))
        {
            notExist = true;
            this->marketConfig = defaultMarketConfig();
            saveMarketConfig(marketFileName);
        }
        QString exchangePath = trtutils::appendPath(dataPath, exchangeFileName);
        if (!trtutils::exists(exchangePath))
        {
            notExist = true;
            this->exchangeConfig = defaultExchangeConfig();
            saveExchangeConfig(exchangeFileName);
        }
        return notExist;
    }

    bool ConfigManager::loadMarketConfig(const QString& fileName)
    {
        QString path = trtutils::appendPath(dataPath, fileName);
        QByteArray bytes = trtutils::readAllBytes(path);
        if (bytes.isEmpty())
        {
            return false;
        }

        try
        {
            const elementResult config = parseElement(bytes);
            MarketConfig loaded;
            for (const jsonElement entry : readArray(config, "instruments"))
            {
                const std::string id(readString(entry, "instId"));
                Exchange exchange = readExchange(readString(entry, "exchange"));
                switch (exchange)
                {
                    //TODO: Implementation for other exchanges are pending
                case Exchange::OKX:
                default:
                    {
                        const InstType type = OKXParser::readType(readString(entry, "instType"));
                        loaded.instruments.push_back(std::make_shared<okx::OKXInstrument>(id, type));
                    }
                }
            }
            marketConfig = std::move(loaded);
            return true;
        } catch (...)
        {
            return false;
        }
        return false;
    }

    bool ConfigManager::loadExchangeConfig(const QString& fileName)
    {
        QString path = trtutils::appendPath(dataPath, fileName);
        QByteArray bytes = trtutils::readAllBytes(path);
        if (bytes.isEmpty())
        {
            return false;
        }

        try
        {
            setExchange(readExchange(readString(parseElement(bytes), "exchange")));
            return true;
        } catch (...)
        {
            return false;
        }
        return false;
    }

    bool ConfigManager::saveMarketConfig(const QString& fileName)
    {
        QString path = trtutils::appendPath(dataPath, fileName);
        jsonBuilder builder = newBuilder();

        builder.start_object();
        // Save instruments "instruments": [XXX]
        builder.escape_and_append_with_quotes("instruments");
        builder.append_colon();
        builder.start_array();
        bool first = true;
        try
        {
            for (const std::shared_ptr<Instrument>& instrument : marketConfig.instruments)
            {
                if (!first)
                    builder.append_comma();
                first = false;
                builder.start_object();
                builder.append_key_value("instId", instrument->instId);
                builder.append_comma();
                builder.append_key_value("instType", instrument->getInstTypeStr());
                builder.append_comma();
                builder.append_key_value("exchange", getExchangeStr(instrument->exchange));
                builder.end_object();
            }
        } catch (...)
        {
            return false;
        }
        builder.end_array();
        builder.end_object();
        return writeConfig(path, builder.view());
    }

    bool ConfigManager::saveExchangeConfig(const QString& fileName)
    {
        jsonBuilder builder = newBuilder();
        builder.start_object();
        try
        {
            builder.append_key_value("exchange", getExchangeStr(getExchange()));
        } catch (...)
        {
            return false;
        }
        builder.end_object();
        return writeConfig(trtutils::appendPath(dataPath, fileName), builder.view());
    }

    std::span<const std::shared_ptr<Instrument>> ConfigManager::getInstruments() const
    {
        return marketConfig.instruments;
    }

    std::span<const std::shared_ptr<Instrument>> ConfigManager::getInstruments(int from, int to) const
    {
        return getInstruments().subspan(from, to - from);
    }

    Instrument& ConfigManager::getInstrument(int index)
    {
        return *marketConfig.instruments[index];
    }

    const ExchangeConfig* ConfigManager::getExchangeConfig()
    {
        return &exchangeConfig;
    }

    int ConfigManager::addInstrument(const Instrument& instrument)
    {
        if (instrument.exchange != Exchange::OKX)
            return -1;
        const int index = static_cast<int>(marketConfig.instruments.size());
        marketConfig.instruments.push_back(std::make_shared<okx::OKXInstrument>(instrument.instId, instrument.instType));
        return index;
    }

    bool ConfigManager::removeInstrument(const Instrument& instrument)
    {
        const std::span<const std::shared_ptr<Instrument>> instruments = getInstruments();
        for (std::size_t index = 0; index < instruments.size(); index++)
        {
            const Instrument& candidate = *instruments[index];
            if (candidate.instId == instrument.instId && candidate.instType == instrument.instType && candidate.exchange == instrument.exchange)
                return removeInstrument(static_cast<int>(index));
        }
        return false;
    }

    bool ConfigManager::removeInstrument(const std::string& instId)
    {
        const std::span<const std::shared_ptr<Instrument>> instruments = getInstruments();
        for (std::size_t index = 0; index < instruments.size(); index++)
        {
            if (instruments[index]->instId == instId)
                return removeInstrument(static_cast<int>(index));
        }
        return false;
    }

    bool ConfigManager::removeInstrument(int index)
    {
        if (index < 0 || static_cast<std::size_t>(index) >= marketConfig.instruments.size())
            return false;
        marketConfig.instruments.erase(marketConfig.instruments.begin() + index);
        return true;
    }

    bool ConfigManager::moveInstrument(int from, int to)
    {
        std::vector<std::shared_ptr<Instrument>>& instruments = marketConfig.instruments;
        if (from < 0 || to < 0 || static_cast<std::size_t>(std::max(from, to)) >= instruments.size())
            return false;
        if (from < to)
            std::rotate(instruments.begin() + from, instruments.begin() + from + 1, instruments.begin() + to + 1);
        else if (from > to)
            std::rotate(instruments.begin() + to, instruments.begin() + from, instruments.begin() + from + 1);
        return true;
    }

    Exchange ConfigManager::getExchange()
    {
        return exchangeConfig.current;
    }

    void ConfigManager::setExchange(Exchange exchange)
    {
        exchangeConfig.current = exchange;
    }

    int ConfigManager::instrumentsCount()
    {
        return marketConfig.instruments.size();
    }
} // trtdata
