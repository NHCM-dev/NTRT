//
// Created by NHCM on 2026/11/9.
//

#include "AppearanceManager.h"
#include <QApplication>
#include "main/util/FileUtil.h"
#include "main/util/JsonUtil.h"

namespace trtdata
{
    AppearanceManager::AppearanceManager(const QString& dataPath) : dataPath(dataPath)
    {}

    bool AppearanceManager::tryInitOnce()
    {
        return !trtutils::exists(trtutils::appendPath(dataPath, "appearance.json")) && saveConfig();
    }

    bool AppearanceManager::loadConfig()
    {
        const QByteArray bytes = trtutils::readAllBytes(trtutils::appendPath(dataPath, "appearance.json"));
        if (bytes.isEmpty())
            return false;

        try
        {
            const elementResult json = parseElement(bytes);
            const std::string_view theme = readString(json, "theme");
            if (theme != "light" && theme != "dark")
                return false;

            bool priceFlash = true;
            const simdjson::error_code error = json["priceFlash"].get_bool().get(priceFlash);
            if (error != simdjson::SUCCESS && error != simdjson::NO_SUCH_FIELD)
                return false;

            int64_t displayScalePercent = 100;
            const simdjson::error_code scaleError = json["displayScalePercent"].get_int64().get(displayScalePercent);
            if ((scaleError != simdjson::SUCCESS && scaleError != simdjson::NO_SUCH_FIELD) ||
                displayScalePercent < MIN_DISPLAY_SCALE_PERCENT || displayScalePercent > MAX_DISPLAY_SCALE_PERCENT)
                return false;

            MarketColumnVisibility columns;
            elementResult marketColumns = json["marketColumns"];
            if (marketColumns.error() != simdjson::NO_SUCH_FIELD)
            {
                const jsonElement saved = marketColumns.value();
                const simdjson::error_code amountError = saved["changeAmount"].get_bool().get(columns.changeAmount);
                if (amountError != simdjson::SUCCESS && amountError != simdjson::NO_SUCH_FIELD)
                    return false;
                columns.exchange = saved["exchange"].get_bool().value();
                columns.type = saved["type"].get_bool().value();
                columns.volume24h = saved["volume24h"].get_bool().value();
                const simdjson::error_code iconError = saved["icon"].get_bool().get(columns.icon);
                if (iconError != simdjson::SUCCESS && iconError != simdjson::NO_SUCH_FIELD)
                    return false;
            }

            appearanceConfig = {theme == "dark" ? EnumTheme::Dark : EnumTheme::Light, priceFlash, columns,
                static_cast<int>(displayScalePercent)};
            return true;
        } catch (const simdjson::simdjson_error&)
        {
            return false;
        }
    }

    bool AppearanceManager::saveConfig() const
    {
        jsonBuilder builder = newBuilder();
        builder.start_object();
        builder.append_key_value("theme", appearanceConfig.currentTheme == EnumTheme::Dark ? "dark" : "light");
        builder.append_comma();
        builder.append_key_value("priceFlash", appearanceConfig.priceFlash);
        builder.append_comma();
        builder.append_key_value("displayScalePercent", appearanceConfig.displayScalePercent);
        builder.append_comma();
        builder.escape_and_append_with_quotes("marketColumns");
        builder.append_colon();
        builder.start_object();
        builder.append_key_value("exchange", appearanceConfig.marketColumns.exchange);
        builder.append_comma();
        builder.append_key_value("type", appearanceConfig.marketColumns.type);
        builder.append_comma();
        builder.append_key_value("volume24h", appearanceConfig.marketColumns.volume24h);
        builder.append_comma();
        builder.append_key_value("icon", appearanceConfig.marketColumns.icon);
        builder.append_comma();
        builder.append_key_value("changeAmount", appearanceConfig.marketColumns.changeAmount);
        builder.end_object();
        builder.end_object();
        return writeJsonFile(trtutils::appendPath(dataPath, "appearance.json"), builder.view());
    }

    EnumTheme AppearanceManager::getCurrentTheme() const
    {
        return appearanceConfig.currentTheme;
    }

    void AppearanceManager::setTheme(EnumTheme theme)
    {
        appearanceConfig.currentTheme = theme;
    }

    bool AppearanceManager::isPriceFlashEnabled() const
    {
        return appearanceConfig.priceFlash;
    }

    bool AppearanceManager::setPriceFlashEnabled(bool enabled)
    {
        const bool previous = appearanceConfig.priceFlash;
        appearanceConfig.priceFlash = enabled;
        if (saveConfig())
            return true;
        appearanceConfig.priceFlash = previous;
        return false;
    }

    const MarketColumnVisibility& AppearanceManager::getMarketColumns() const
    {
        return appearanceConfig.marketColumns;
    }

    bool AppearanceManager::setMarketColumns(const MarketColumnVisibility& columns)
    {
        const MarketColumnVisibility previous = appearanceConfig.marketColumns;
        appearanceConfig.marketColumns = columns;
        if (saveConfig())
            return true;
        appearanceConfig.marketColumns = previous;
        return false;
    }

    int AppearanceManager::getDisplayScalePercent() const
    {
        return appearanceConfig.displayScalePercent;
    }

    bool AppearanceManager::setDisplayScalePercent(int percent)
    {
        if (percent < MIN_DISPLAY_SCALE_PERCENT || percent > MAX_DISPLAY_SCALE_PERCENT)
            return false;
        const int previous = appearanceConfig.displayScalePercent;
        appearanceConfig.displayScalePercent = percent;
        if (saveConfig())
            return true;
        appearanceConfig.displayScalePercent = previous;
        return false;
    }

    bool AppearanceManager::isDisplayScaleRestartRequired() const
    {
        return appliedDisplayScalePercent != appearanceConfig.displayScalePercent;
    }

    void AppearanceManager::initializeDisplayScale()
    {
        appliedDisplayScalePercent = appearanceConfig.displayScalePercent;
        // Application zoom multiplies the OS per-monitor scale; never rescale persisted logical geometry.
        qputenv("QT_SCALE_FACTOR", QByteArray::number(appliedDisplayScalePercent / 100.0, 'f', 2));
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    }

}
