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

    struct AppearanceConfig
    {
        EnumTheme currentTheme = EnumTheme::Light;
        bool priceFlash = true;
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
        static QPalette generatePalette(EnumTheme theme);
        static void applyPalette(EnumTheme theme);
    };
}

#endif //NTRT_APPEARANCEMANAGER_H
