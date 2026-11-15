//
// Created by NHCM on 2026/11/9.
//

#ifndef NTRT_APPEARANCEMANAGER_H
#define NTRT_APPEARANCEMANAGER_H

#include <QString>

#include "AppearanceDef.h"

namespace trtdata
{
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
