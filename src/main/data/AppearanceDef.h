#ifndef NTRT_APPEARANCEDEF_H
#define NTRT_APPEARANCEDEF_H



namespace trtdata
{
    inline constexpr int MIN_DISPLAY_SCALE_PERCENT = 25;
    inline constexpr int MAX_DISPLAY_SCALE_PERCENT = 400;

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
        int displayScalePercent = 100;
    };
}

#endif //NTRT_APPEARANCEDEF_H
