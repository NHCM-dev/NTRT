#include "AbstractApi.h"
#include "OKXApi.h"
#include <stdexcept>

namespace trtcore
{
    const AbstractApi& exchangeApi(trtdata::Exchange exchange)
    {
        switch (exchange)
        {
        case trtdata::Exchange::OKX:
            static const trtdata::okx::OKXApi api;
            return api;
        }
        throw std::invalid_argument("Unsupported exchange");
    }

    std::vector<trtdata::Exchange> registeredExchanges()
    {
        return {trtdata::Exchange::OKX};
    }
}
