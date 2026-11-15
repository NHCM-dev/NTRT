#ifndef NTRT_PALETTEUTIL_H
#define NTRT_PALETTEUTIL_H

#include <QPalette>

namespace trtdata
{
    enum class EnumTheme;
}

namespace trtutils
{
    QPalette generatePalette(trtdata::EnumTheme theme);

    void applyPalette(trtdata::EnumTheme theme);

    QColor risingColor(const QPalette& palette);

    QColor fallingColor(const QPalette& palette);
}

#endif //NTRT_PALETTEUTIL_H
