#include "OKXAccountUtil.h"
#include "main/trade/parser/OKXParser.h"
#include "main/util/NumberUtil.h"
#include <QMessageAuthenticationCode>
#include <stdexcept>
namespace trtdata::okx
{
    static std::string field(const trtdata::jsonElement& row, const char* name)
    {
        std::string_view value;
        const simdjson::error_code error = row[name].get_string().get(value);
        if (error == simdjson::NO_SUCH_FIELD)
            return {};
        if (error != simdjson::SUCCESS)
            throw simdjson::simdjson_error(error);
        return std::string(value);
    }

    static void require(bool valid, const char* message)
    {
        if (!valid)
            throw std::invalid_argument(message);
    }

    static std::string_view mode(trttrade::MarginMode value)
    {
        switch (value)
        {
            case trttrade::MarginMode::Cross: return "cross";
            case trttrade::MarginMode::Isolated: return "isolated";
            case trttrade::MarginMode::Cash: return "cash";
            case trttrade::MarginMode::SpotIsolated: return "spot_isolated";
        }

        throw std::invalid_argument("Invalid margin mode");
    }

    static std::string_view positionSide(trttrade::PositionSide value)
    {
        switch (value)
        {
            case trttrade::PositionSide::Long: return "long";
            case trttrade::PositionSide::Short: return "short";
            case trttrade::PositionSide::Net: return "net";
        }

        throw std::invalid_argument("Invalid position side");
    }

    static std::string_view orderType(trttrade::OrderType value)
    {
        switch (value)
        {
            case trttrade::OrderType::Market: return "market";
            case trttrade::OrderType::Limit: return "limit";
            case trttrade::OrderType::PostOnly: return "post_only";
            case trttrade::OrderType::FOK: return "fok";
            case trttrade::OrderType::IOC: return "ioc";
            case trttrade::OrderType::OptimalLimitIOC: return "optimal_limit_ioc";
            case trttrade::OrderType::MMP: return "mmp";
            case trttrade::OrderType::MMPAndPostOnly: return "mmp_and_post_only";
            case trttrade::OrderType::RPI: return "rpi";
        }

        throw std::invalid_argument("Invalid order type");
    }

    static void append(trtdata::jsonBuilder& builder, const char* key, std::string_view value)
    {
        if (value.empty())
            return;
        builder.append_comma();
        builder.append_key_value(key, value);
    }

    static QByteArray bytes(trtdata::jsonBuilder& builder)
    {
        builder.end_object();
        const std::string_view value = builder.view();
        return QByteArray(value.data(), static_cast<qsizetype>(value.size()));
    }

    static void positive(const std::string& value)
    {
        require(trtutils::parseNumber<double>(value, "Invalid decimal") > 0, "Decimal must be positive");
    }

    QByteArray sign(const QByteArray& secret, const QByteArray& timestamp, const QByteArray& method, const QByteArray& path, const QByteArray& body)
    {
        return QMessageAuthenticationCode::hash(timestamp + method + path + body, secret, QCryptographicHash::Sha256).toBase64();
    }

    QUrlQuery query(const trttrade::AccountQuery& value, bool paged)
    {
        QUrlQuery result;
        if (value.instType)
            result.addQueryItem("instType", QString::fromUtf8(trtdata::OKXParser::getInstType(*value.instType)));
        if (!value.instId.empty())
            result.addQueryItem("instId", QString::fromStdString(value.instId));
        if (paged)
        {
            require(value.limit >= 1 && value.limit <= 100, "Limit must be between 1 and 100");
            result.addQueryItem("limit", QString::number(value.limit));
            if (!value.after.empty()) result.addQueryItem("after", QString::fromStdString(value.after));
            if (!value.before.empty()) result.addQueryItem("before", QString::fromStdString(value.before));
        }

        return result;
    }

    QUrlQuery identifier(const trttrade::OrderIdentifier& value)
    {
        require(!value.instId.empty() && (!value.ordId.empty() || !value.clOrdId.empty()), "Instrument and order ID required");
        QUrlQuery result;
        result.addQueryItem("instId", QString::fromStdString(value.instId));
        if (!value.ordId.empty()) result.addQueryItem("ordId", QString::fromStdString(value.ordId));
        else result.addQueryItem("clOrdId", QString::fromStdString(value.clOrdId));
        return result;
    }

    QByteArray cancelBody(const trttrade::OrderIdentifier& value)
    {
        const QUrlQuery parameters = identifier(value);
        trtdata::jsonBuilder builder = trtdata::newBuilder();
        builder.start_object();
        builder.append_key_value("instId", std::string_view(value.instId));
        append(builder, "ordId", value.ordId);
        if (value.ordId.empty()) append(builder, "clOrdId", value.clOrdId);
        return bytes(builder);
    }

    QByteArray orderBody(const trttrade::PlaceOrderRequest& value)
    {
        require(!value.instId.empty(), "Instrument required");
        positive(value.sz);
        switch (value.ordType)
        {
            case trttrade::OrderType::Market:
            case trttrade::OrderType::OptimalLimitIOC: break;
            default: positive(value.px); break;
        }

        require(value.tgtCcy.empty() || value.tgtCcy == "base_ccy" || value.tgtCcy == "quote_ccy", "Invalid target currency");
        require(value.clOrdId.size() <= 32 && value.clOrdId.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") == std::string::npos, "Invalid client order ID");
        trtdata::jsonBuilder builder = trtdata::newBuilder();
        builder.start_object();
        builder.append_key_value("instId", std::string_view(value.instId));
        append(builder, "tdMode", mode(value.tdMode));
        switch (value.side)
        {
            case trttrade::OrderSide::Buy: append(builder, "side", "buy"); break;
            case trttrade::OrderSide::Sell: append(builder, "side", "sell"); break;
            default: throw std::invalid_argument("Invalid order side");
        }

        append(builder, "ordType", orderType(value.ordType));
        append(builder, "sz", value.sz);
        append(builder, "px", value.px);
        append(builder, "clOrdId", value.clOrdId);
        append(builder, "ccy", value.ccy);
        append(builder, "tgtCcy", value.tgtCcy);
        if (value.posSide) append(builder, "posSide", positionSide(*value.posSide));
        if (value.reduceOnly)
        {
            builder.append_comma();
            builder.append_key_value("reduceOnly", *value.reduceOnly);
        }

        return bytes(builder);
    }

    QByteArray amendBody(const trttrade::AmendOrderRequest& value)
    {
        identifier(value.order);
        require(!value.newSz.empty() || !value.newPx.empty(), "New size or price required");
        if (!value.newSz.empty()) positive(value.newSz);
        if (!value.newPx.empty()) positive(value.newPx);
        trtdata::jsonBuilder builder = trtdata::newBuilder();
        builder.start_object();
        builder.append_key_value("instId", std::string_view(value.order.instId));
        append(builder, "ordId", value.order.ordId);
        if (value.order.ordId.empty()) append(builder, "clOrdId", value.order.clOrdId);
        append(builder, "newSz", value.newSz);
        append(builder, "newPx", value.newPx);
        builder.append_comma();
        builder.append_key_value("cxlOnFail", value.cxlOnFail);
        return bytes(builder);
    }

    QByteArray leverageBody(const trttrade::LeverageRequest& value)
    {
        require(!value.instId.empty() || !value.ccy.empty(), "Instrument or currency required");
        positive(value.lever);
        trtdata::jsonBuilder builder = trtdata::newBuilder();
        builder.start_object();
        builder.append_key_value("lever", std::string_view(value.lever));
        switch (value.mgnMode)
        {
            case trttrade::TradeMode::Cross: append(builder, "mgnMode", "cross"); break;
            case trttrade::TradeMode::Isolated: append(builder, "mgnMode", "isolated"); break;
            default: throw std::invalid_argument("Invalid leverage mode");
        }

        append(builder, "instId", value.instId);
        append(builder, "ccy", value.ccy);
        if (value.posSide) append(builder, "posSide", positionSide(*value.posSide));
        return bytes(builder);
    }

    trttrade::BalanceDetail parseBalanceDetail(const trtdata::jsonElement& row)
    {
        trttrade::BalanceDetail value;
        value.ccy = field(row, "ccy");
        value.eq = field(row, "eq");
        value.cashBal = field(row, "cashBal");
        value.availBal = field(row, "availBal");
        value.availEq = field(row, "availEq");
        value.frozenBal = field(row, "frozenBal");
        value.upl = field(row, "upl");
        value.liab = field(row, "liab");
        value.eqUsd = field(row, "eqUsd");
        return value;
    }

    trttrade::FundingBalance parseFundingBalance(const trtdata::jsonElement& row)
    {
        trttrade::FundingBalance value;
        value.ccy = field(row, "ccy");
        value.bal = field(row, "bal");
        value.availBal = field(row, "availBal");
        value.frozenBal = field(row, "frozenBal");
        return value;
    }

    trttrade::PositionData parsePositionData(const trtdata::jsonElement& row)
    {
        trttrade::PositionData value;
        value.instType = field(row, "instType");
        value.instId = field(row, "instId");
        value.posId = field(row, "posId");
        value.posSide = field(row, "posSide");
        value.mgnMode = field(row, "mgnMode");
        value.pos = field(row, "pos");
        value.availPos = field(row, "availPos");
        value.avgPx = field(row, "avgPx");
        value.markPx = field(row, "markPx");
        value.upl = field(row, "upl");
        value.uplRatio = field(row, "uplRatio");
        value.lever = field(row, "lever");
        value.liqPx = field(row, "liqPx");
        value.margin = field(row, "margin");
        value.ccy = field(row, "ccy");
        value.cTime = field(row, "cTime");
        value.uTime = field(row, "uTime");
        require(!value.instId.empty(), "Missing response instrument");
        return value;
    }

    trttrade::OrderData parseOrderData(const trtdata::jsonElement& row)
    {
        trttrade::OrderData value;
        value.instType = field(row, "instType");
        value.instId = field(row, "instId");
        value.ordId = field(row, "ordId");
        value.clOrdId = field(row, "clOrdId");
        value.tdMode = field(row, "tdMode");
        value.side = field(row, "side");
        value.posSide = field(row, "posSide");
        value.ordType = field(row, "ordType");
        value.state = field(row, "state");
        value.px = field(row, "px");
        value.sz = field(row, "sz");
        value.accFillSz = field(row, "accFillSz");
        value.avgPx = field(row, "avgPx");
        value.fee = field(row, "fee");
        value.feeCcy = field(row, "feeCcy");
        value.pnl = field(row, "pnl");
        value.cTime = field(row, "cTime");
        value.uTime = field(row, "uTime");
        require(!value.instId.empty(), "Missing response instrument");
        return value;
    }

    trttrade::FillData parseFillData(const trtdata::jsonElement& row)
    {
        trttrade::FillData value;
        value.instType = field(row, "instType");
        value.instId = field(row, "instId");
        value.ordId = field(row, "ordId");
        value.clOrdId = field(row, "clOrdId");
        value.tradeId = field(row, "tradeId");
        value.billId = field(row, "billId");
        value.side = field(row, "side");
        value.posSide = field(row, "posSide");
        value.fillPx = field(row, "fillPx");
        value.fillSz = field(row, "fillSz");
        value.fee = field(row, "fee");
        value.feeCcy = field(row, "feeCcy");
        value.ts = field(row, "ts");
        require(!value.instId.empty(), "Missing response instrument");
        return value;
    }

    trttrade::OrderResult parseOrderResult(const trtdata::jsonElement& row)
    {
        trttrade::OrderResult value;
        value.ordId = field(row, "ordId");
        value.clOrdId = field(row, "clOrdId");
        value.sCode = field(row, "sCode");
        value.sMsg = field(row, "sMsg");
        value.reqId = field(row, "reqId");
        if (value.sCode.empty() || value.sCode != "0")
            throw std::runtime_error("OKX order error " + value.sCode + ": " + value.sMsg);
        return value;
    }

    trttrade::AccountConfiguration parseAccountConfiguration(const trtdata::jsonElement& row)
    {
        trttrade::AccountConfiguration value;
        value.uid = field(row, "uid");
        value.acctLv = field(row, "acctLv");
        value.posMode = field(row, "posMode");
        const simdjson::error_code error = row["autoLoan"].get_bool().get(value.autoLoan);
        if (error != simdjson::SUCCESS && error != simdjson::NO_SUCH_FIELD)
            throw simdjson::simdjson_error(error);
        value.perm = field(row, "perm");
        return value;
    }

    trttrade::LeverageData parseLeverageData(const trtdata::jsonElement& row)
    {
        trttrade::LeverageData value;
        value.instId = field(row, "instId");
        value.ccy = field(row, "ccy");
        value.mgnMode = field(row, "mgnMode");
        value.posSide = field(row, "posSide");
        value.lever = field(row, "lever");
        return value;
    }

    trttrade::AccountBalance parseAccountBalance(const trtdata::jsonElement& row)
    {
        trttrade::AccountBalance value;
        value.totalEq = field(row, "totalEq");
        value.adjEq = field(row, "adjEq");
        value.imr = field(row, "imr");
        value.mmr = field(row, "mmr");
        value.uTime = field(row, "uTime");
        for (const trtdata::jsonElement detail : trtdata::readArray(row, "details"))
            value.details.push_back(parseBalanceDetail(detail));
        return value;
    }
}
