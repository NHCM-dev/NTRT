#include "WorkspaceStateUtil.h"
#include "JsonUtil.h"
#include <QSet>
#include <limits>
#include <cmath>

namespace trtutils
{
    void appendWorkspaceIdentifier(trtdata::jsonBuilder& builder, const std::optional<trtdata::InstIdentifier>& identifier)
    {
        builder.start_object();
        builder.append_key_value("id", identifier ? identifier->id : std::string());
        builder.append_comma();
        builder.append_key_value("exchange", identifier ? static_cast<int>(identifier->exchange) : 0);
        builder.append_comma();
        builder.append_key_value("type", identifier ? static_cast<int>(identifier->type) : 0);
        builder.end_object();
    }

    int readWorkspaceInt(const trtdata::jsonElement& json, const char* key, int minimum, int maximum)
    {
        const int64_t value = json[key].get_int64().value();
        if (value < minimum || value > maximum)
            throw simdjson::simdjson_error(simdjson::NUMBER_OUT_OF_RANGE);
        return static_cast<int>(value);
    }

    QString readWorkspaceString(const trtdata::jsonElement& json, const char* key)
    {
        const std::string_view value = trtdata::readString(json, key);
        return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
    }

    std::optional<trtdata::InstIdentifier> readWorkspaceIdentifier(const trtdata::jsonElement& json)
    {
        const std::string id(trtdata::readString(json, "id"));
        const int exchange = readWorkspaceInt(json, "exchange", 0, 0);
        const int type = readWorkspaceInt(json, "type", 0, 5);
        if (id.empty())
            return std::nullopt;
        return trtdata::InstIdentifier{static_cast<trtdata::Exchange>(exchange), static_cast<trttrade::InstType>(type), id};
    }

    QByteArray serializeWorkspaceConfig(const trtdata::WorkspaceConfig& config)
    {
        trtdata::jsonBuilder builder = trtdata::newBuilder();
        builder.start_object();
        builder.append_key_value("version", 1);
        builder.append_comma();
        builder.append_key_value("mainGeometry", config.mainGeometry.toBase64().toStdString());
        builder.append_comma();
        builder.append_key_value("currentWorkspace", config.currentWorkspace);
        builder.append_comma();
        builder.append_key_value("nextWorkspaceId", config.nextWorkspaceId);
        builder.append_comma();
        builder.escape_and_append_with_quotes("workspaces");
        builder.append_colon();
        builder.start_array();
        bool firstWorkspace = true;
        for (const trtdata::WorkspaceState& workspace : config.workspaces)
        {
            if (!firstWorkspace)
                builder.append_comma();
            firstWorkspace = false;
            builder.start_object();
            builder.append_key_value("id", workspace.id.toStdString());
            builder.append_comma();
            builder.append_key_value("name", workspace.name.toStdString());
            builder.append_comma();
            builder.append_key_value("activeWindow", workspace.activeWindow.toStdString());
            builder.append_comma();
            builder.append_key_value("lastChart", workspace.lastChart.toStdString());
            builder.append_comma();
            builder.append_key_value("nextBookDepthId", workspace.nextBookDepthId);
            builder.append_comma();
            builder.append_key_value("nextChartId", workspace.nextChartId);
            builder.append_comma();
            builder.escape_and_append_with_quotes("instruments");
            builder.append_colon();
            builder.start_object();
            builder.append_key_value("search", workspace.instruments.search.toStdString());
            builder.append_comma();
            builder.append_key_value("typeFilter", workspace.instruments.typeFilter);
            builder.append_comma();
            builder.append_key_value("header", workspace.instruments.header.toBase64().toStdString());
            builder.append_comma();
            builder.append_key_value("scroll", workspace.instruments.scroll);
            builder.append_comma();
            builder.escape_and_append_with_quotes("selected");
            builder.append_colon();
            appendWorkspaceIdentifier(builder, workspace.instruments.selected);
            builder.end_object();
            builder.append_comma();
            builder.escape_and_append_with_quotes("windows");
            builder.append_colon();
            builder.start_array();
            bool firstWindow = true;
            for (const trtdata::WorkspaceWindowState& window : workspace.windows)
            {
                if (!firstWindow)
                    builder.append_comma();
                firstWindow = false;
                builder.start_object();
                builder.append_key_value("id", window.id.toStdString());
                builder.append_comma();
                builder.append_key_value("type", static_cast<int>(window.type));
                builder.append_comma();
                builder.append_key_value("x", window.geometry.x());
                builder.append_comma();
                builder.append_key_value("y", window.geometry.y());
                builder.append_comma();
                builder.append_key_value("width", window.geometry.width());
                builder.append_comma();
                builder.append_key_value("height", window.geometry.height());
                builder.append_comma();
                builder.append_key_value("mode", static_cast<int>(window.mode));
                builder.append_comma();
                builder.append_key_value("visible", window.visible);
                builder.append_comma();
                builder.append_key_value("timeframe", static_cast<int>(window.chart.timeframe));
                builder.append_comma();
                builder.append_key_value("volume", window.chart.volume);
                builder.append_comma();
                builder.escape_and_append_with_quotes("instrument");
                builder.append_colon();
                appendWorkspaceIdentifier(builder, window.chart.instrument);
                switch (window.type)
                {
                    case trtdata::WorkspaceWindowType::AccountBalance:
                    case trtdata::WorkspaceWindowType::AccountPositions:
                    case trtdata::WorkspaceWindowType::AccountOrderHistory:
                    case trtdata::WorkspaceWindowType::AccountPositionHistory:
                    case trtdata::WorkspaceWindowType::AccountOrders:
                    case trtdata::WorkspaceWindowType::Instruments:
                    case trtdata::WorkspaceWindowType::Chart:
                        break;
                    case trtdata::WorkspaceWindowType::BookDepth:
                    {
                        builder.append_comma();
                        builder.escape_and_append_with_quotes("bookDepth");
                        builder.append_colon();
                        builder.start_object();
                        builder.append_key_value("depth", window.bookDepth.depth);
                        builder.append_comma();
                        builder.append_key_value("priceStep", window.bookDepth.priceStep);
                        builder.append_comma();
                        builder.append_key_value("rows", window.bookDepth.rows);
                        builder.append_comma();
                        builder.append_key_value("layout", static_cast<int>(window.bookDepth.layout));
                        builder.append_comma();
                        builder.append_key_value("priceAlignment", static_cast<int>(window.bookDepth.priceAlignment));
                        builder.append_comma();
                        builder.append_key_value("settingsExpanded", window.bookDepth.settingsExpanded);
                        builder.append_comma();
                        builder.append_key_value("cumulative", window.bookDepth.cumulative);
                        builder.append_comma();
                        builder.escape_and_append_with_quotes("instrument");
                        builder.append_colon();
                        appendWorkspaceIdentifier(builder, window.bookDepth.instrument);
                        builder.end_object();
                        break;
                    }
                }
                builder.end_object();
            }
            builder.end_array();
            builder.end_object();
        }
        builder.end_array();
        builder.end_object();
        const std::string_view json = builder.view();
        return QByteArray(json.data(), static_cast<qsizetype>(json.size()));
    }

    bool parseWorkspaceConfig(const QByteArray& bytes, trtdata::WorkspaceConfig& config)
    {
        try
        {
            const trtdata::jsonElement json = trtdata::parseElement(bytes).value();
            readWorkspaceInt(json, "version", 1, 1);
            trtdata::WorkspaceConfig loaded;
            loaded.mainGeometry = QByteArray::fromBase64(readWorkspaceString(json, "mainGeometry").toLatin1());
            loaded.currentWorkspace = readWorkspaceInt(json, "currentWorkspace", 0, 1000);
            loaded.nextWorkspaceId = readWorkspaceInt(json, "nextWorkspaceId", 1, 1000000);
            QSet<QString> workspaceIds;
            for (const trtdata::jsonElement entry : trtdata::readArray(json, "workspaces"))
            {
                trtdata::WorkspaceState workspace;
                workspace.id = readWorkspaceString(entry, "id");
                workspace.name = readWorkspaceString(entry, "name");
                if (!workspace.id.startsWith("workspace") || workspace.id.mid(9).toInt() <= 0 ||
                    workspace.id.mid(9).toInt() >= 1000000 || workspaceIds.contains(workspace.id))
                    return false;
                workspaceIds.insert(workspace.id);
                workspace.activeWindow = readWorkspaceString(entry, "activeWindow");
                workspace.lastChart = readWorkspaceString(entry, "lastChart");
                workspace.nextChartId = readWorkspaceInt(entry, "nextChartId", 1, 1000000);
                trtdata::jsonElement bookCounter;
                if (entry["nextBookDepthId"].get(bookCounter) != simdjson::NO_SUCH_FIELD)
                    workspace.nextBookDepthId = readWorkspaceInt(entry, "nextBookDepthId", 1, 1000000);
                const trtdata::jsonElement list = entry["instruments"].value();
                workspace.instruments.search = readWorkspaceString(list, "search");
                workspace.instruments.typeFilter = readWorkspaceInt(list, "typeFilter", -1, 5);
                workspace.instruments.header = QByteArray::fromBase64(readWorkspaceString(list, "header").toLatin1());
                workspace.instruments.scroll = readWorkspaceInt(list, "scroll", 0, std::numeric_limits<int>::max());
                workspace.instruments.selected = readWorkspaceIdentifier(list["selected"].value());
                QSet<QString> windowIds;
                bool hasInstruments = false;
                for (const trtdata::jsonElement item : trtdata::readArray(entry, "windows"))
                {
                    trtdata::WorkspaceWindowState window;
                    window.id = readWorkspaceString(item, "id");
                    if (window.id.isEmpty() || windowIds.contains(window.id))
                        return false;
                    windowIds.insert(window.id);
                    window.type = static_cast<trtdata::WorkspaceWindowType>(readWorkspaceInt(item, "type", 0, 7));
                    switch (window.type)
                    {
                        case trtdata::WorkspaceWindowType::AccountBalance:
                        case trtdata::WorkspaceWindowType::AccountPositions:
                        case trtdata::WorkspaceWindowType::AccountOrderHistory:
                        case trtdata::WorkspaceWindowType::AccountPositionHistory:
                        case trtdata::WorkspaceWindowType::AccountOrders:
                            if (!window.id.startsWith("account")) return false;
                            break;
                        case trtdata::WorkspaceWindowType::Instruments:
                            if (hasInstruments)
                                return false;
                            if (window.id != "instrumentList")
                                return false;
                            hasInstruments = true;
                            break;
                        case trtdata::WorkspaceWindowType::Chart:
                            if (!window.id.startsWith("chart") || window.id.mid(5).toInt() <= 0 || window.id.mid(5).toInt() >= 1000000)
                                return false;
                            break;
                        case trtdata::WorkspaceWindowType::BookDepth:
                        {
                            if (!window.id.startsWith("bookDepth") || window.id.mid(9).toInt() <= 0 || window.id.mid(9).toInt() >= 1000000)
                                return false;
                            const trtdata::jsonElement book = item["bookDepth"].value();
                            window.bookDepth.instrument = readWorkspaceIdentifier(book["instrument"].value());
                            window.bookDepth.depth = readWorkspaceInt(book, "depth", 1, 400);
                            window.bookDepth.rows = readWorkspaceInt(book, "rows", 1, 100);
                            window.bookDepth.layout = static_cast<trtdata::BookLayout>(readWorkspaceInt(book, "layout", 0, 2));
                            window.bookDepth.priceStep = book["priceStep"].get_double().value();
                            if (!std::isfinite(window.bookDepth.priceStep) || window.bookDepth.priceStep < 0 || window.bookDepth.priceStep > 1e12)
                                return false;
                            window.bookDepth.cumulative = book["cumulative"].get_bool().value();
                            trtdata::jsonElement alignment;
                            if (book["priceAlignment"].get(alignment) != simdjson::NO_SUCH_FIELD)
                                window.bookDepth.priceAlignment = static_cast<trtdata::PriceAlignment>(readWorkspaceInt(book, "priceAlignment", 0, 2));
                            trtdata::jsonElement expanded;
                            if (book["settingsExpanded"].get(expanded) != simdjson::NO_SUCH_FIELD)
                                window.bookDepth.settingsExpanded = expanded.get_bool().value();
                            break;
                        }
                    }
                    window.geometry = QRect(readWorkspaceInt(item, "x", -1000000, 1000000),
                        readWorkspaceInt(item, "y", -1000000, 1000000), readWorkspaceInt(item, "width", 1, 100000),
                        readWorkspaceInt(item, "height", 1, 100000));
                    window.mode = static_cast<trtdata::WorkspaceWindowMode>(readWorkspaceInt(item, "mode", 0, 2));
                    window.visible = item["visible"].get_bool().value();
                    window.chart.timeframe = static_cast<trttrade::CandleTimeframe>(readWorkspaceInt(item, "timeframe", 0, 10));
                    window.chart.volume = item["volume"].get_bool().value();
                    window.chart.instrument = readWorkspaceIdentifier(item["instrument"].value());
                    workspace.windows.push_back(std::move(window));
                }
                loaded.workspaces.push_back(std::move(workspace));
            }
            if (!loaded.workspaces.empty() && loaded.currentWorkspace >= static_cast<int>(loaded.workspaces.size()))
                return false;
            config = std::move(loaded);
            return true;
        } catch (const simdjson::simdjson_error&)
        {
            return false;
        }
    }
}
