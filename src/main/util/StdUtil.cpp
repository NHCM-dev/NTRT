//
// Created by NHCM on 2026/11/8.
//

#include "StdUtil.h"

#include <algorithm>
#include <vector>

namespace trtutils
{
    template <typename T>
    bool contains(const std::vector<T>& vec, const T& value)
    {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }
} // trtutils