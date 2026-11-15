#include "ExchangeUtil.h"
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
