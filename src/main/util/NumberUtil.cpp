#include "NumberUtil.h"
#include <algorithm>
#include <cmath>

namespace trtutils
{
    QString trimDecimalZeros(QString value)
    {
        if (value.contains('.'))
        {
            while (value.endsWith('0'))
                value.chop(1);
            if (value.endsWith('.'))
                value.chop(1);
        }
        return value;
    }

    QString formatPrice(double value)
    {
        if (value == 0)
            return "0";
        // Quote precision is independent of the distance between axis ticks.
        const int decimals = std::abs(value) < 0.000001 ? 12 : 8;
        QString result = QString::number(value, 'f', decimals);
        return trimDecimalZeros(result);
    }

    QString formatAxisPrice(double value, double pricePerPixel)
    {
        const int decimals = std::clamp(static_cast<int>(std::ceil(-std::log10(pricePerPixel))) + 1, 2, 12);
        return QString::number(value, 'f', decimals);
    }

    QString formatVolume(double value)
    {
        const double magnitude = std::abs(value);
        double divisor = 1;
        QString suffix;
        if (magnitude >= 1000000000000.0)
        {
            divisor = 1000000000000.0;
            suffix = "T";
        } else if (magnitude >= 1000000000.0)
        {
            divisor = 1000000000.0;
            suffix = "B";
        } else if (magnitude >= 1000000.0)
        {
            divisor = 1000000.0;
            suffix = "M";
        } else if (magnitude >= 10000.0)
        {
            divisor = 1000.0;
            suffix = "K";
        }
        QString result = QString::number(value / divisor, 'f', suffix.isEmpty() ? 4 : 2);
        return trimDecimalZeros(result) + suffix;
    }

    qsizetype changedDigitOffset(const QString& price, double difference)
    {
        const int changedPower = static_cast<int>(std::floor(std::log10(difference) + 1e-9));
        const qsizetype exponentPosition = price.indexOf('e', 0, Qt::CaseInsensitive);
        const qsizetype end = exponentPosition < 0 ? price.size() : exponentPosition;
        const int exponent = exponentPosition < 0 ? 0 : price.mid(exponentPosition + 1).toInt();
        const qsizetype decimal = price.indexOf('.');
        const qsizetype integerEnd = decimal < 0 ? end : decimal;
        int power = exponent - 1;
        for (qsizetype index = 0; index < integerEnd; index++)
        {
            if (price[index].isDigit())
                power++;
        }
        for (qsizetype index = 0; index < end; index++)
        {
            if (!price[index].isDigit())
                continue;
            if (power <= changedPower)
                return index;
            power--;
        }
        return price.size();
    }
}


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
