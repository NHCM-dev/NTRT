#ifndef NTRT_FORMATUTIL_H
#define NTRT_FORMATUTIL_H

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

#endif //NTRT_FORMATUTIL_H
