#ifndef NTRT_WORKSPACECONFIGDEF_H
#define NTRT_WORKSPACECONFIGDEF_H

#include "main/data/core/Workspace.h"
#include "main/trade/TradeDef.h"
#include <QByteArray>
#include <QRect>
#include <QString>
#include <optional>
#include <vector>

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

    struct BookDepthPanelState
    {
        std::optional<InstIdentifier> instrument;
        int depth = 400;
        double priceStep = 0;
        int rows = 20;
        BookLayout layout = BookLayout::Both;
        bool cumulative = true;
        bool settingsExpanded = true;
        PriceAlignment priceAlignment = PriceAlignment::Left;
    };

    struct WorkspaceWindowState
    {
        QString id;
        WorkspaceWindowType type = WorkspaceWindowType::Chart;
        QRect geometry;
        WorkspaceWindowMode mode = WorkspaceWindowMode::Normal;
        bool visible = true;
        ChartPanelState chart;
        BookDepthPanelState bookDepth;
    };

    struct WorkspaceState
    {
        QString id;
        QString name;
        QString activeWindow;
        QString lastChart;
        int nextChartId = 1;
        int nextBookDepthId = 1;
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
}

#endif //NTRT_WORKSPACECONFIGDEF_H
