#ifndef NTRT_NUMBERUTIL_H
#define NTRT_NUMBERUTIL_H

#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

namespace trtutils
{
    template<class Number>
    Number parseNumber(std::string_view value, std::string_view errorMessage)
    {
        if (value.empty())
            throw std::runtime_error(std::string(errorMessage));
        Number number{};
        const char* end = value.data() + value.size();
        const std::from_chars_result parsed = std::from_chars(value.data(), end, number);
        if (parsed.ec != std::errc{} || parsed.ptr != end || !std::isfinite(number))
            throw std::runtime_error(std::string(errorMessage));
        return number;
    }
}


#include <QString>

namespace trtutils
{
    QString trimDecimalZeros(QString value);

    QString formatPrice(double value);

    // pricePerPixel must be finite and positive.
    QString formatAxisPrice(double value, double pricePerPixel);

    QString formatVolume(double value);

    // difference is the positive finite absolute price movement.
    qsizetype changedDigitOffset(const QString& price, double difference);
}




namespace trtutils
{
    double niceAxisStep(double minimum, double maximum, double height);
}


#endif //NTRT_NUMBERUTIL_H
