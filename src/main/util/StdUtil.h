//
// Created by NHCM on 2026/11/8.
//

#ifndef NTRT_STDUTIL_H
#define NTRT_STDUTIL_H
#include <vector>
#include <algorithm>

namespace trtutils
{
    template<typename T>
    bool contains(const std::vector<T>& vec, const T& value)
    {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }
} // trtutils

#endif //NTRT_STDUTIL_H
