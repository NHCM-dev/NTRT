#ifndef NTRT_WORKSPACEMANAGER_H
#define NTRT_WORKSPACEMANAGER_H

#include "main/data/config/WorkspaceConfig.h"

namespace trtdata
{
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
