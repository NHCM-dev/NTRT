#include "WorkspaceManager.h"
#include "main/util/FileUtil.h"
#include "main/util/JsonUtil.h"
#include "main/util/WorkspaceStateUtil.h"

namespace trtdata
{
    WorkspaceManager::WorkspaceManager(const QString& dataPath) : path(trtutils::appendPath(dataPath, "workspaces.json"))
    {}

    bool WorkspaceManager::loadConfig()
    {
        writable = false;
        if (!trtutils::exists(path))
        {
            writable = true;
            return true;
        }
        WorkspaceConfig loaded;
        const QByteArray bytes = trtutils::readAllBytes(path);
        if (!trtutils::parseWorkspaceConfig(bytes, loaded))
            return false;
        config = std::move(loaded);
        savedJson = trtutils::serializeWorkspaceConfig(config);
        writable = true;
        return true;
    }

    bool WorkspaceManager::saveConfig(const WorkspaceConfig& value)
    {
        if (!writable)
            return false;
        const QByteArray bytes = trtutils::serializeWorkspaceConfig(value);
        if (bytes != savedJson && !writeJsonFile(path, std::string_view(bytes.constData(), bytes.size())))
            return false;
        config = value;
        savedJson = bytes;
        return true;
    }

    const WorkspaceConfig& WorkspaceManager::getConfig() const
    {
        return config;
    }
}
