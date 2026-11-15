//
// Created by NHCM on 2026/11/9.
//

#ifndef NTRT_APPEARANCEMANAGER_H
#define NTRT_APPEARANCEMANAGER_H

#include <QPalette>
#include <QString>

namespace trtdata
{
    enum class EnumTheme
    {
        Light,
        Dark
    };

    struct MarketColumnVisibility
    {
        bool exchange = false;
        bool type = false;
        bool volume24h = false;
        bool icon = true;
        bool changeAmount = true;
    };

    struct AppearanceConfig
    {
        EnumTheme currentTheme = EnumTheme::Dark;
        bool priceFlash = true;
        MarketColumnVisibility marketColumns{};
    };

    class AppearanceManager final
    {
        QString dataPath;
        AppearanceConfig appearanceConfig;

    public:
        explicit AppearanceManager(const QString& dataPath);

        bool tryInitOnce();

        bool loadConfig();

        bool saveConfig() const;

        EnumTheme getCurrentTheme() const;

        void setTheme(EnumTheme theme);

        bool isPriceFlashEnabled() const;

        bool setPriceFlashEnabled(bool enabled);

        const MarketColumnVisibility& getMarketColumns() const;

        bool setMarketColumns(const MarketColumnVisibility& columns);
    };
}

#endif //NTRT_APPEARANCEMANAGER_H
