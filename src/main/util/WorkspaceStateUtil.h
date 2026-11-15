#ifndef NTRT_WORKSPACESTATEUTIL_H
#define NTRT_WORKSPACESTATEUTIL_H
#include "main/data/WorkspaceDef.h"
namespace trtutils
{
    QByteArray serializeWorkspaceConfig(const trtdata::WorkspaceConfig& config);
    bool parseWorkspaceConfig(const QByteArray& bytes, trtdata::WorkspaceConfig& config);
}
#endif
