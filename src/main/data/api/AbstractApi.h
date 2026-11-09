//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_ABSTRACTAPI_H
#define NTRT_ABSTRACTAPI_H

namespace trtcore
{
    class AbstractApi
    {
    public:
        virtual ~AbstractApi() = default;

        virtual int getRequestInterval() = 0;
    };
}

#endif //NTRT_ABSTRACTAPI_H
