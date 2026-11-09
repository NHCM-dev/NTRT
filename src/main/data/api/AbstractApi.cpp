#include "AbstractApi.h"
#include "OKXApi.h"
#include <stdexcept>

namespace trtcore
{
    static const trtdata::okx::OKXApi okxApi;

    const AbstractApi& exchangeApi(trtdata::Exchange exchange)
    {
        switch (exchange)
        {
        case trtdata::Exchange::OKX:
            return okxApi;
        }
        throw std::invalid_argument("Unsupported exchange");
    }

    std::vector<trtdata::Exchange> availableExchanges()
    {
        return {trtdata::Exchange::OKX};
    }
}
