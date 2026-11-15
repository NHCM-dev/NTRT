#ifndef NTRT_APPEARANCEDEF_H
#define NTRT_APPEARANCEDEF_H



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
}

#endif //NTRT_APPEARANCEDEF_H
