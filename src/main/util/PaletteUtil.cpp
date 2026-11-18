#include "PaletteUtil.h"
#include "main/data/core/Appearance.h"
#include <QApplication>

namespace trtutils
{
    QPalette generatePalette(trtdata::Theme theme)
    {
        QColor window(0xf0f0f0);
        QColor base(0xffffff);
        QColor text(0x202124);
        QColor muted(0x686e75);
        QPalette palette(window, window);
        palette.setColor(QPalette::Highlight, QColor(0x3478c0));

        switch (theme)
        {
            case trtdata::Theme::Light:
                palette.setColor(QPalette::Light, Qt::white);
                palette.setColor(QPalette::Midlight, QColor(0xe0e3e8));
                palette.setColor(QPalette::Mid, QColor(0xbcc2ca));
                palette.setColor(QPalette::Dark, QColor(0x9aa2ac));
                palette.setColor(QPalette::Shadow, QColor(0xd9dde3));
                palette.setColor(QPalette::Link, QColor(0x1460aa));
                break;
            case trtdata::Theme::Dark:
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

    void applyPalette(trtdata::Theme theme)
    {
        // Remove resolved QSS palettes before changing the application palette. Otherwise
        // palette(...) rules retain colors from the previous theme on existing widgets.
        const QString stylesheet = qApp->styleSheet();
        if (!stylesheet.isEmpty())
            qApp->setStyleSheet(QString());
        const QPalette palette = generatePalette(theme);
        QApplication::setPalette(palette);

        QPalette inputs = palette;
        switch (theme)
        {
            case trtdata::Theme::Light:
                break;
            case trtdata::Theme::Dark:
                inputs.setColor(QPalette::Base, QColor(0x363c46));
                break;
        }
        QApplication::setPalette(inputs, "QLineEdit");
        if (!stylesheet.isEmpty())
            qApp->setStyleSheet(stylesheet);
    }

    QColor risingColor(const QPalette& palette)
    {
        return QColor(palette.color(QPalette::Base).lightness() < 128 ? 0x4ade80 : 0x15803d);
    }

    QColor fallingColor(const QPalette& palette)
    {
        return QColor(palette.color(QPalette::Base).lightness() < 128 ? 0xfb7185 : 0xb91c1c);
    }
}
