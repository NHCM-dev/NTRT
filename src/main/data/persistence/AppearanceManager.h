//
// Created by NHCM on 2026/11/9.
//

#ifndef NTRT_APPEARANCEMANAGER_H
#define NTRT_APPEARANCEMANAGER_H

#include <QString>
#include <QObject>

#include "main/data/config/AppearanceConfig.h"

namespace trtdata
{
    class AppearanceManager final : public QObject
    {
        Q_OBJECT
        QString dataPath;
        AppearanceConfig appearanceConfig;
        int appliedDisplayScalePercent = 100;

    public:
        explicit AppearanceManager(const QString& dataPath);

        bool tryInitOnce();

        bool loadConfig();

        bool saveConfig() const;

        Theme getCurrentTheme() const;

        void setTheme(Theme theme);

        bool isPriceFlashEnabled() const;

        bool setPriceFlashEnabled(bool enabled);

        const MarketColumnVisibility& getMarketColumns() const;

        bool setMarketColumns(const MarketColumnVisibility& columns);

        int getDisplayScalePercent() const;

        bool setDisplayScalePercent(int percent);

        bool isDisplayScaleRestartRequired() const;

        // Call after loading appearance settings, before constructing QApplication.
        void initializeDisplayScale();

    signals:
        void marketAppearanceChanged();
    };
}

#endif //NTRT_APPEARANCEMANAGER_H
