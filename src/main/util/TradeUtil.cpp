#include "TradeUtil.h"
#include "ExchangeUtil.h"
#include "main/market/abstract/Instrument.h"

namespace trtutils
{
    QString instrumentLabel(const trtdata::InstIdentifier& identifier)
    {
        return QString::fromStdString(identifier.id) + " · " + QString::fromUtf8(
                trtdata::getExchangeStr(identifier.exchange)) +
            " · " + QString::fromUtf8(trttrade::instDisplayStr(identifier.type));
    }
}
