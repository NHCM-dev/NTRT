#include "main/data/WorkspaceManager.h"
#include "main/util/WorkspaceStateUtil.h"
#include "main/util/FileUtil.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try
    {
        QTemporaryDir directory;
        require(directory.isValid(), "temporary directory");
        trtdata::WorkspaceManager manager(directory.path());
        require(manager.loadConfig(), "missing configuration should load defaults");
        trtdata::WorkspaceConfig config;
        config.mainGeometry = QByteArray("binary\0geometry", 15);
        config.nextWorkspaceId = 8;
        config.currentWorkspace = 1;
        trtdata::WorkspaceState workspace;
        workspace.id = "workspace7";
        workspace.name = QString::fromUtf8("研究 \"BTC\"\n布局");
        workspace.nextChartId = 10;
        workspace.activeWindow = "chart9";
        workspace.lastChart = "chart9";
        workspace.instruments.search = "BTC";
        workspace.instruments.typeFilter = 2;
        workspace.instruments.selected = trtdata::InstIdentifier{trtdata::Exchange::OKX, trttrade::InstType::Swap, "BTC-USDT-SWAP"};
        workspace.instruments.header = QByteArray("header\0state", 12);
        workspace.instruments.scroll = 17;
        trtdata::WorkspaceWindowState chart;
        chart.id = "chart9";
        chart.geometry = QRect(-50, 80, 640, 480);
        chart.mode = trtdata::WorkspaceWindowMode::Minimized;
        chart.visible = false;
        chart.chart.instrument = workspace.instruments.selected;
        chart.chart.timeframe = trttrade::CandleTimeframe::Hour4;
        chart.chart.volume = false;
        workspace.windows.push_back(chart);
        config.workspaces.push_back(workspace);
        workspace.id = "workspace3";
        workspace.name = "Empty workspace";
        workspace.windows.clear();
        config.workspaces.push_back(workspace);
        require(manager.saveConfig(config), "atomic save");
        trtdata::WorkspaceManager restored(directory.path());
        require(restored.loadConfig(), "reload saved configuration");
        require(trtutils::serializeWorkspaceConfig(config) == trtutils::serializeWorkspaceConfig(restored.getConfig()), "complete round trip");
        require(restored.saveConfig(config), "unchanged save");
        const QString path = trtutils::appendPath(directory.path(), "workspaces.json");
        const QByteArray valid = trtutils::readAllBytes(path);
        trtdata::WorkspaceConfig parsed = config;
        require(!trtutils::parseWorkspaceConfig("{", parsed), "reject malformed JSON");
        require(parsed.workspaces.size() == 2, "parse failure preserves previous state");
        QByteArray invalid = trtutils::serializeWorkspaceConfig(config);
        invalid.replace("\"timeframe\":7", "\"timeframe\":999");
        require(!trtutils::parseWorkspaceConfig(invalid, parsed), "reject invalid enum");
        invalid = trtutils::serializeWorkspaceConfig(config);
        invalid.replace("workspace3", "workspace7");
        require(!trtutils::parseWorkspaceConfig(invalid, parsed), "reject duplicate workspace identity");
        require(trtutils::writeAllBytesAtomically(path, "{"), "write corrupt fixture");
        require(!restored.loadConfig(), "reject corrupt reload");
        require(restored.getConfig().workspaces.size() == 2, "reload failure preserves in-memory state");
        require(!restored.saveConfig(config), "failed reload disables overwrites");
        trtdata::WorkspaceManager corrupt(directory.path());
        require(!corrupt.loadConfig(), "reject corrupt initial load");
        require(!corrupt.saveConfig(config), "prevent automatic overwrite of corrupt file");
        require(trtutils::readAllBytes(path) == "{", "corrupt file preserved for recovery");
        require(trtutils::writeAllBytesAtomically(path, valid), "restore fixture");
        std::cout << "workspace persistence checks passed\n";
        return 0;
    } catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
