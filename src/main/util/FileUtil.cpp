//
// Created by NHCM on 2026/11/6.
//

#include "FileUtil.h"
#include <QFile>

QByteArray trtutils::readAllBytes(const char* path)
{
    return readAllBytes(path, {});
}

QByteArray trtutils::readAllBytes(const char* path, QIODeviceBase::OpenMode extra_flags)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | extra_flags))
    {
        return file.readAll();
    }
    return {};
}