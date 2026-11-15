#ifndef NTRT_WORKSPACEMANAGER_H
#define NTRT_WORKSPACEMANAGER_H

#include <QByteArray>
#include <QRect>
#include <QString>
#include <vector>
#include "main/market/abstract/Instrument.h"
#include "main/trade/CandleTimeframe.h"

namespace trtdata
{
    struct InstrumentListState
    {
        QString search;
        int typeFilter = -1;
        std::optional<InstIdentifier> selected;
        QByteArray header;
        int scroll = 0;
    };

    struct ChartPanelState
    {
        std::optional<InstIdentifier> instrument;
        trttrade::CandleTimeframe timeframe = trttrade::CandleTimeframe::Minute15;
        bool volume = true;
    };

    enum class WorkspaceWindowType { Instruments, Chart };
    enum class WorkspaceWindowMode { Normal, Minimized, Maximized };

    struct WorkspaceWindowState
    {
        QString id;
        WorkspaceWindowType type = WorkspaceWindowType::Chart;
        QRect geometry;
        WorkspaceWindowMode mode = WorkspaceWindowMode::Normal;
        bool visible = true;
        ChartPanelState chart;
    };

    struct WorkspaceState
    {
        QString id;
        QString name;
        QString activeWindow;
        QString lastChart;
        int nextChartId = 1;
        InstrumentListState instruments;
        std::vector<WorkspaceWindowState> windows;
    };

    struct WorkspaceConfig
    {
        QByteArray mainGeometry;
        int currentWorkspace = 0;
        int nextWorkspaceId = 1;
        std::vector<WorkspaceState> workspaces;
    };

    class WorkspaceManager final
    {
        QString path;
        WorkspaceConfig config;
        QByteArray savedJson;
        bool writable = false;

    public:
        explicit WorkspaceManager(const QString& dataPath);
        bool loadConfig();
        bool saveConfig(const WorkspaceConfig& value);
        const WorkspaceConfig& getConfig() const;
    };
}
#endif
