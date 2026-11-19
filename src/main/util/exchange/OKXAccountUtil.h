#ifndef NTRT_OKXACCOUNTUTIL_H
#define NTRT_OKXACCOUNTUTIL_H
#include "main/trade/AccountDef.h"
#include "main/trade/AccountDataDef.h"
#include "main/trade/AccountCredentials.h"
#include "main/util/JsonUtil.h"
#include <QUrlQuery>
#include "main/trade/parser/OKXParser.h"
#include <stdexcept>
namespace trtdata::okx
{
    QByteArray sign(const QByteArray& secret, const QByteArray& timestamp, const QByteArray& method, const QByteArray& path, const QByteArray& body);
    QUrlQuery query(const trttrade::AccountQuery& value, bool paged);
    QUrlQuery identifier(const trttrade::OrderIdentifier& value);
    QByteArray orderBody(const trttrade::PlaceOrderRequest& value);
    QByteArray cancelBody(const trttrade::OrderIdentifier& value);
    QByteArray amendBody(const trttrade::AmendOrderRequest& value);
    QByteArray leverageBody(const trttrade::LeverageRequest& value);
    trttrade::BalanceDetail parseBalanceDetail(const trtdata::jsonElement& row);
    trttrade::FundingBalance parseFundingBalance(const trtdata::jsonElement& row);
    trttrade::PositionHistoryData parsePositionHistoryData(const trtdata::jsonElement& row);
    trttrade::PositionData parsePositionData(const trtdata::jsonElement& row);
    trttrade::OrderData parseOrderData(const trtdata::jsonElement& row);
    trttrade::FillData parseFillData(const trtdata::jsonElement& row);
    trttrade::OrderResult parseOrderResult(const trtdata::jsonElement& row);
    trttrade::AccountConfiguration parseAccountConfiguration(const trtdata::jsonElement& row);
    trttrade::LeverageData parseLeverageData(const trtdata::jsonElement& row);
    trttrade::AccountBalance parseAccountBalance(const trtdata::jsonElement& row);
    template<class T, class Parser>
    std::vector<T> parseList(const QByteArray& bytes, Parser parse)
    {
        const trtdata::elementResult response = trtdata::parseElement(bytes);
        trtdata::OKXParser::ensureSuccess(response);
        std::vector<T> result;
        for (const trtdata::jsonElement row : trtdata::readArray(response, "data"))
            result.push_back(parse(row));
        return result;
    }
    template<class T, class Parser>
    T parseOne(const QByteArray& bytes, Parser parse)
    {
        std::vector<T> rows = parseList<T>(bytes, parse);
        if (rows.size() != 1)
            throw std::runtime_error("Expected one OKX account result");
        return std::move(rows.front());
    }
}
#endif
