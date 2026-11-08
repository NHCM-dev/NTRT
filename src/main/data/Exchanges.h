//
// Created by NHCM on 2026/11/7.
//

#ifndef NTRT_EXCHANGES_H
#define NTRT_EXCHANGES_H
#include <string_view>

namespace trtdata
{
   enum class Exchange
   {
      Binance,
      Bybit,
      OKX
   };

   constexpr std::string_view getExchangeStr(Exchange exchange)
   {
      switch (exchange)
      {
         case Exchange::Binance: return "Binance";
         case Exchange::Bybit: return "Bybit";
         case Exchange::OKX: return "OKX";
      }

      return "";
   }
} // trtdata

#endif //NTRT_EXCHANGES_H
