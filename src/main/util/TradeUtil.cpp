#include "TradeUtil.h"
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

#include "main/api/AbstractApi.h"
#include <stdexcept>

namespace trtdata
{
    Exchange readExchange(std::string_view name)
    {
        for (Exchange exchange : trtcore::availableExchanges())
        {
            if (getExchangeStr(exchange) == name)
                return exchange;
        }
        throw std::invalid_argument("Unknown exchange: " + std::string(name));
    }
}
