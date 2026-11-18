#ifndef NTRT_APPEARANCECONFIGDEF_H
#define NTRT_APPEARANCECONFIGDEF_H

#include "main/data/core/Appearance.h"

namespace trtdata
{
    inline constexpr int MIN_DISPLAY_SCALE_PERCENT = 25;
    inline constexpr int MAX_DISPLAY_SCALE_PERCENT = 400;

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
        Theme currentTheme = Theme::Dark;
        bool priceFlash = true;
        MarketColumnVisibility marketColumns{};
        int displayScalePercent = 85;
    };
}

#endif //NTRT_APPEARANCECONFIGDEF_H
