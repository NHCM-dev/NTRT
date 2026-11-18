#ifndef NTRT_EXCHANGECONFIGDEF_H
#define NTRT_EXCHANGECONFIGDEF_H

#include "../../core/ExchangeDef.h"
#include <string>

namespace trtdata
{
    struct ExchangeConfig
    {
        std::string fileName;
        Exchange current{};
    };
}

#endif //NTRT_EXCHANGECONFIGDEF_H
