#ifndef NTRT_NETWORKDEF_H
#define NTRT_NETWORKDEF_H

#include <cstdint>

namespace trtnet
{
    enum class HTTPMethod : uint8_t
    {
        GET,
        POST,
        PUT,
        PATCH,
        DELETE,
        HEAD,
        OPTIONS,
        TRACE
    };
}

#endif //NTRT_NETWORKDEF_H
