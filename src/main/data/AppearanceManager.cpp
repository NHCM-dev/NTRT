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

            setTheme(theme == "dark" ? EnumTheme::Dark : EnumTheme::Light);
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

    QPalette AppearanceManager::generatePalette(EnumTheme theme)
    {
        const bool dark = theme == EnumTheme::Dark;
        const QColor window(dark ? 0x282b30 : 0xf0f0f0);
        const QColor base(dark ? 0x202227 : 0xffffff);
        const QColor text(dark ? 0xe8eaed : 0x202124);
        const QColor button(dark ? 0x383c43 : 0xf0f0f0);
        const QColor muted(dark ? 0xa8afb9 : 0x686e75);

        QPalette palette(button, window);
        palette.setColor(QPalette::WindowText, text);
        palette.setColor(QPalette::Base, base);
        palette.setColor(QPalette::AlternateBase, window);
        palette.setColor(QPalette::Text, text);
        palette.setColor(QPalette::ButtonText, text);
        palette.setColor(QPalette::PlaceholderText, muted);
        palette.setColor(QPalette::ToolTipBase, window);
        palette.setColor(QPalette::ToolTipText, text);
        palette.setColor(QPalette::Highlight, QColor(dark ? 0x345d92 : 0x3478c0));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        if (dark)
        {
            palette.setColor(QPalette::Light, QColor(0x68717e));
            palette.setColor(QPalette::Midlight, QColor(0x555d69));
            palette.setColor(QPalette::Mid, QColor(0x4b535f));
            palette.setColor(QPalette::Dark, QColor(0x3d444f));
            palette.setColor(QPalette::Shadow, QColor(0x14161a));
            palette.setColor(QPalette::Link, QColor(0x8ab8f3));
        }
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
