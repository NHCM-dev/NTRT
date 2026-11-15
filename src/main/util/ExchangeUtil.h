#ifndef NTRT_EXCHANGEUTIL_H
#define NTRT_EXCHANGEUTIL_H

#include "main/data/Exchanges.h"

namespace trtdata
{
    constexpr std::string_view getExchangeStr(Exchange exchange)
    {
        switch (exchange)
        {
            case Exchange::OKX:
                return "OKX";
        }

        return "";
    }

    Exchange readExchange(std::string_view name);
}

#endif //NTRT_EXCHANGEUTIL_H
