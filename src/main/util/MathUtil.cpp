#include "MathUtil.h"
#include <algorithm>
#include <cmath>

namespace trtutils
{
    double niceAxisStep(double minimum, double maximum, double height)
    {
        const double intervals = std::max(2.0, height / 48.0);
        const double raw = (maximum - minimum) / intervals;
        const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
        const double fraction = raw / magnitude;
        double nearest = 1;
        for (double step : {2.0, 2.5, 5.0, 10.0})
        {
            if (std::abs(step - fraction) < std::abs(nearest - fraction))
                nearest = step;
        }
        return nearest * magnitude;
    }
}
