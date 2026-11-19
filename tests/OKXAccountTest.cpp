#include "main/util/exchange/OKXAccountUtil.h"
#include "main/util/NetworkUtil.h"
#include "main/network/RestAPITemplate.h"
#include "main/util/WorkspaceStateUtil.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <iostream>
#include <stdexcept>

static void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try
    {
        using namespace trtdata::okx;
        check(sign("test-secret", "2020-12-08T09:08:57.715Z", "GET", "/api/v5/account/balance?ccy=BTC", {}) == "5KlCItRxE039QKll2OJlbYeUcSiPGR/z10UR7bbl68o=", "Signature mismatch");
        trttrade::PlaceOrderRequest order;
        order.instId = "BTC-USDT";
        order.sz = "0.00000001";
        order.px = "123456.123456789";
        order.clOrdId = "test123";
        order.reduceOnly = false;
        const QByteArray body = orderBody(order);
        const trtdata::elementResult json = trtdata::parseElement(body);
        check(trtdata::readString(json, "sz") == order.sz && trtdata::readString(json, "px") == order.px, "Decimal precision lost");
        check(!json["reduceOnly"].get_bool().value(), "Boolean serialization failed");
        order.sz = "0";
        bool rejected = false;
        try { orderBody(order); } catch (const std::exception&) { rejected = true; }
        check(rejected, "Invalid size accepted");
        const trttrade::AccountBalance balance = parseOne<trttrade::AccountBalance>(R"({"code":"0","data":[{"totalEq":"1.123456789123456789","details":[{"ccy":"BTC","eq":"0.00000001","availEq":""}]}]})", parseAccountBalance);
        check(balance.totalEq == "1.123456789123456789" && balance.details[0].availEq.empty(), "Balance precision/empty field lost");
        check(parseList<trttrade::PositionData>(R"({"code":"0","data":[]})", parsePositionData).empty(), "Empty positions failed");
        const trttrade::PositionData position = parseOne<trttrade::PositionData>(R"({"code":"0","data":[{"instId":"BTC-USDT-SWAP","pos":"-2","posSide":"net","liqPx":"","upl":"-64.30","uplRatio":"-0.2956","bePx":"113.18","mgnRatio":"20.0367"}]})", parsePositionData);
        check(position.pos == "-2" && position.liqPx.empty() && position.upl == "-64.30" && position.uplRatio == "-0.2956" && position.bePx == "113.18", "Signed position/P&L fields lost");
        rejected = false;
        try { parseOne<trttrade::OrderResult>(R"({"code":"0","data":[{"sCode":"51000","sMsg":"invalid order"}]})", parseOrderResult); }
        catch (const std::exception&) { rejected = true; }
        check(rejected, "Per-order error accepted");
        rejected = false;
        try { parseList<trttrade::OrderData>(R"({"code":"50101","msg":"invalid key","data":[]})", parseOrderData); }
        catch (const std::exception&) { rejected = true; }
        check(rejected, "API error accepted");
        rejected = false;
        try { parseOne<trttrade::OrderResult>(R"({"code":"0","data":[]})", parseOrderResult); }
        catch (const std::exception&) { rejected = true; }
        check(rejected, "Missing order result accepted");
        const trttrade::AccountConfiguration config = parseOne<trttrade::AccountConfiguration>(R"({"code":"0","data":[{"uid":"123","autoLoan":true,"perm":"read_only"}]})", parseAccountConfiguration);
        check(config.autoLoan && config.uid == "123", "Account configuration boolean failed");
        trtdata::WorkspaceConfig layout;
        trtdata::WorkspaceState workspace;
        workspace.id = "workspace1";
        workspace.name = "Test";
        int accountId = 1;
        for (trtdata::WorkspaceWindowType type : {trtdata::WorkspaceWindowType::AccountBalance,
            trtdata::WorkspaceWindowType::AccountPositions, trtdata::WorkspaceWindowType::AccountOrders})
        {
            trtdata::WorkspaceWindowState window;
            window.id = QString("account%1").arg(accountId++);
            window.type = type;
            window.geometry = QRect(40, 40, 800, 400);
            workspace.windows.push_back(window);
        }
        layout.workspaces.push_back(workspace);
        trtdata::WorkspaceConfig restored;
        check(trtutils::parseWorkspaceConfig(trtutils::serializeWorkspaceConfig(layout), restored), "Account window layout parse failed");
        check(restored.workspaces[0].windows.size() == 3 && restored.workspaces[0].windows[2].type == trtdata::WorkspaceWindowType::AccountOrders,
            "Account window layout roundtrip failed");
        QTcpServer server;
        check(server.listen(QHostAddress::LocalHost), "Local test server failed");
        QByteArray received;
        QObject::connect(&server, &QTcpServer::newConnection, &server, [&server, &received]
        {
            QTcpSocket* socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, &received]
            {
                received += socket->readAll();
                if (!received.contains("\r\n\r\n")) return;
                const qsizetype split = received.indexOf("\r\n\r\n") + 4;
                if (received.size() - split < 14) return;
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}");
                socket->disconnectFromHost();
            });
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
        const trtnet::RestAPITemplate endpoint{trtnet::HTTPMethod::POST, QUrl("http://127.0.0.1:" + QString::number(server.serverPort())), "/test", {}};
        QFuture<QByteArray> future = trtnet::replyToFuture(endpoint.request({}, "{\"sz\":\"0.001\"}", {{"X-Test", "signed"}}), [](const QByteArray& bytes) { return bytes; });
        QEventLoop loop;
        QFutureWatcher<QByteArray> watcher;
        QObject::connect(&watcher, &QFutureWatcher<QByteArray>::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(3000, &loop, &QEventLoop::quit);
        watcher.setFuture(future);
        loop.exec();
        check(future.isFinished() && future.result() == "{}", "Local request failed");
        check(received.startsWith("POST /test HTTP/1.1") && received.toLower().contains("x-test: signed") && received.endsWith("{\"sz\":\"0.001\"}"), "POST bytes/headers mismatch");
        std::cout << "OKX signing, serialization, parsing, errors and HTTP transport passed\n";
        return 0;
    } catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
