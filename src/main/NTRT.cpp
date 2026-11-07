//
// Created by NHCM on 2026/11/6.
//

#include "NTRT.h"

int main(int arg_count, char* args[])
{
    for (int i = 0; i < arg_count; i++)
    {
        ntrt::args.emplace_back(args[i]);
    }
}
