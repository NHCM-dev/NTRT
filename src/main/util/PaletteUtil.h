#ifndef NTRT_PALETTEUTIL_H
#define NTRT_PALETTEUTIL_H

#include <QPalette>
#include "main/data/core/Appearance.h"

namespace trtutils
{
    QPalette generatePalette(trtdata::Theme theme);

    void applyPalette(trtdata::Theme theme);

    QColor risingColor(const QPalette& palette);

    QColor fallingColor(const QPalette& palette);
}

#endif //NTRT_PALETTEUTIL_H
