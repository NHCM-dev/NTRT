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
    {
    }

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

            appearanceConfig = {theme == "dark" ? EnumTheme::Dark : EnumTheme::Light, priceFlash};
            return true;
        }
        catch (const simdjson::simdjson_error&)
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

    QPalette AppearanceManager::generatePalette(EnumTheme theme)
    {
        QColor window(0xf0f0f0);
        QColor base(0xffffff);
        QColor text(0x202124);
        QColor muted(0x686e75);
        QPalette palette(window, window);
        palette.setColor(QPalette::Highlight, QColor(0x3478c0));

        switch (theme)
        {
        case EnumTheme::Light:
            break;
        case EnumTheme::Dark:
            window = QColor(0x282b30);
            base = QColor(0x202227);
            text = QColor(0xe8eaed);
            muted = QColor(0xa8afb9);
            palette = QPalette(QColor(0x383c43), window);
            palette.setColor(QPalette::Highlight, QColor(0x345d92));
            palette.setColor(QPalette::Light, QColor(0x68717e));
            palette.setColor(QPalette::Midlight, QColor(0x555d69));
            palette.setColor(QPalette::Mid, QColor(0x4b535f));
            palette.setColor(QPalette::Dark, QColor(0x3d444f));
            palette.setColor(QPalette::Shadow, QColor(0x14161a));
            palette.setColor(QPalette::Link, QColor(0x8ab8f3));
            break;
        }

        palette.setColor(QPalette::WindowText, text);
        palette.setColor(QPalette::Base, base);
        palette.setColor(QPalette::AlternateBase, window);
        palette.setColor(QPalette::Text, text);
        palette.setColor(QPalette::ButtonText, text);
        palette.setColor(QPalette::PlaceholderText, muted);
        palette.setColor(QPalette::ToolTipBase, window);
        palette.setColor(QPalette::ToolTipText, text);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Text, muted);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, muted);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
        return palette;
    }

    void AppearanceManager::applyPalette(EnumTheme theme)
    {
        const QPalette palette = generatePalette(theme);
        QApplication::setPalette(palette);

        QPalette inputs = palette;
        if (theme == EnumTheme::Dark)
            inputs.setColor(QPalette::Base, QColor(0x363c46));
        QApplication::setPalette(inputs, "QLineEdit");
    }
}
