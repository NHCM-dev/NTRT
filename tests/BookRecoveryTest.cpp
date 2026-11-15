#include "main/util/TradeUtil.h"
#include "main/util/NumberUtil.h"
#include "main/util/PaintUtil.h"
#include "main/ui/widgets/NumericWidgets.h"
#include <QApplication>
#include <QImage>
#include <QPainter>
#include "main/util/OKXBookUtil.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void requireBook(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool apply(const QByteArray& bytes, bool snapshot, trttrade::BookSnapshot& book)
{
    return trtdata::okx::applyBookMessage(trtdata::parseElement(bytes).value(), snapshot, book);
}

void verifyBooks()
{
    trttrade::BookSnapshot book;
    const QByteArray snapshot = R"({"seqId":10,"prevSeqId":-1,"ts":"1000","bids":[["100","2"],["99","3"]],"asks":[["101","4"],["102","5"]]})";
    requireBook(apply(snapshot, true, book), "initial snapshot");
    requireBook(book.bids.front().price == 100 && book.asks.front().price == 101, "best-first ordering");
    requireBook(apply(R"({"seqId":11,"prevSeqId":10,"ts":"1001","bids":[["100","0"],["99","7"]],"asks":[]})", false, book), "incremental update");
    requireBook(book.bids.size() == 1 && book.bids.front().size == 7, "zero deletes; size replaces rather than adds");
    requireBook(!apply(R"({"seqId":13,"prevSeqId":12,"ts":"1002","bids":[],"asks":[]})", false, book), "reject gap");
    requireBook(book.sequence == 11, "gap leaves previous state untouched");
    requireBook(apply(R"({"seqId":2,"prevSeqId":11,"ts":"1003","bids":[],"asks":[]})", false, book), "exchange sequence reset with matching predecessor");
    requireBook(apply(R"({"seqId":2,"prevSeqId":2,"ts":"1004","bids":[],"asks":[]})", false, book), "empty heartbeat");
    requireBook(!apply(R"({"seqId":3,"prevSeqId":2,"ts":"1005","bids":[["103","1"]],"asks":[]})", false, book), "reject crossed book");
    bool malformed = false;
    try
    {
        apply(R"({"seqId":3,"prevSeqId":2,"ts":"1005","bids":[["98","6"]],"asks":[["101","oops"]]})", false, book);
    } catch (const std::runtime_error&)
    {
        malformed = true;
    }
    requireBook(malformed && book.bids.size() == 1 && book.sequence == 2, "malformed update is atomic");
    requireBook(apply(snapshot, true, book) && book.sequence == 10, "resnapshot replaces old levels");
    const std::vector<trttrade::BookLevel> bids{{100.2, 1}, {100.1, 2}, {100, 3}, {99.9, 4}};
    const std::vector<trttrade::BookLevel> asks{{100.1, 1}, {100.2, 2}, {100.3, 3}, {101.1, 4}};
    std::vector<trttrade::BookLevel> levels = trtutils::aggregateBook(bids, true, 4, 1, 2);
    requireBook(levels.size() == 2 && levels[0].price == 100 && levels[0].size == 6 && levels[1].price == 99, "bid buckets round down");
    levels = trtutils::aggregateBook(asks, false, 4, 1, 1);
    requireBook(levels.size() == 1 && levels[0].price == 101 && levels[0].size == 6, "ask buckets round up; complete final bucket");
    levels = trtutils::aggregateBook(bids, true, 2, 1, 20);
    requireBook(levels.size() == 1 && levels[0].size == 3, "raw depth precedes aggregation");
    levels = trtutils::aggregateBook(asks, false, 4, 0.1, 10);
    requireBook(std::abs(levels[2].price - 100.3) < 1e-10, "decimal tick boundary");
    requireBook(trtutils::aggregateBook(bids, true, 4, 0, 10).size() == 4, "native prices");
    requireBook(trtutils::formatQuantity(8461.05000000001) == "8461.05", "aggregated quantity residue");
    requireBook(trtutils::formatQuantity(4737.80000000001) == "4737.8", "cumulative quantity residue");
    requireBook(trtutils::formatDecimal(8417.780000000001) == "8417.78", "decimal tooltip hides binary residue");
    requireBook(trtutils::formatDecimal(65000.0123456789) == "65000.0123456789", "preserve meaningful quote precision");
    requireBook(trtutils::formatDecimal(10.0) == "10", "step omits trailing zeros");
    requireBook(trtutils::formatVolume(0.00000001) == "0.00000001", "small nonzero sizes stay visible");
    requireBook(trtutils::formatDecimal(-0.00000000000001) == "0", "no negative zero");
    requireBook(trtutils::roundToStep(65000.01, 0.01, true) == trtutils::roundToStep(65000.01, 0.01, false), "large tick index stays on its decimal boundary");
    requireBook(std::abs(trtutils::roundToStep(1.2341, 0.01, false) - 1.23) < 1e-12, "bid rounds down off boundary");
    requireBook(std::abs(trtutils::roundToStep(1.2341, 0.01, true) - 1.24) < 1e-12, "ask rounds up off boundary");
    requireBook(trtutils::priceFlashOffset("123.45", 0.01, 100, true) == 0, "full flash phase");
    requireBook(trtutils::priceFlashOffset("123.45", -0.01, 700, true) == 5, "changed suffix phase");
    requireBook(trtutils::priceFlashOffset("123.45", 0.01, 1500, true) == -1, "flash expires");
    requireBook(trtutils::priceFlashOffset("123.45", 0.01, 100, false) == -1, "global flash disable");
    requireBook(trtutils::priceFlashOffset("123.45", 0, 100, true) == -1, "unchanged price does not flash");
    requireBook(std::abs(*trtutils::bookBuyShare({{100, 3}}, {{101, 1}}, 1) - 0.75) < 1e-12, "buy sell volume share");
    requireBook(!trtutils::bookBuyShare({}, {}, 100), "empty book has no percentage");
    requireBook(*trtutils::bookBuyShare({{100, 3}, {99, 99}}, {{101, 1}}, 1) == 0.75, "share respects raw depth");
    requireBook(*trtutils::bookBuyShare({{100, 3}}, {}, 1) == 1, "one-sided book percentage");
    std::cout << "book recovery and aggregation checks passed\n";
}

void verifyNumericWidgets()
{
    trtui::DecimalSpinBox step;
    step.setDecimals(12);
    step.setRange(0, 1e12);
    step.setValue(10);
    requireBook(step.text() == "10", "compact step control");
    step.setValue(0.00000001);
    requireBook(step.text().contains("00000001"), "small step precision");
    trtui::PriceLabel label;
    label.resize(260, 40);
    label.show();
    label.setPriceText("Latest ", "123.45", {}, 0.01, 100, true);
    const QImage full = label.grab().toImage();
    label.setPriceText("Latest ", "123.45", {}, 0.01, 700, true);
    const QImage suffix = label.grab().toImage();
    label.setPriceText("Latest ", "123.45", {}, 0.01, 1600, true);
    const QImage expired = label.grab().toImage();
    requireBook(full != suffix && suffix != expired, "actual flash phases render differently");
    label.setPriceText("Latest ", "123.45", {}, 0.01, 100, false);
    requireBook(label.grab().toImage() == expired, "disabled flash renders plain price");
    label.hide();
    QImage bars(100, 40, QImage::Format_ARGB32);
    bars.fill(Qt::black);
    QPainter painter(&bars);
    trtutils::drawValueBar(painter, QRectF(0, 0, 100, 20), 0.25, Qt::green);
    trtutils::drawValueBar(painter, QRectF(0, 20, 100, 20), 0.75, Qt::green);
    painter.end();
    requireBook(bars.pixelColor(10, 10) == QColor(Qt::black) && bars.pixelColor(80, 10).green() > 0, "quarter bar uses relative length");
    requireBook(bars.pixelColor(30, 30) == bars.pixelColor(80, 10), "different lengths retain the same opacity");
    trtui::RatioBar balance("B", "S");
    balance.setShare(0.7678);
    requireBook(balance.getShare() && *balance.getShare() == 0.7678, "percentage bar accepts share");
    balance.setShare(std::nullopt);
    requireBook(!balance.getShare(), "percentage bar clears on missing feed");
}
