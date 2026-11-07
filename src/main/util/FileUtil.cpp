//
// Created by NHCM on 2026/11/6.
//

#include "FileUtil.h"

#include <QApplication>
#include <QFile>

namespace trtutils
{
    QByteArray readAllBytes(const char* path)
    {
        return readAllBytes(path, {});
    }

    QByteArray readAllBytes(const char* path, QIODeviceBase::OpenMode extra_flags)
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | extra_flags))
        {
            return file.readAll();
        }
        return {};
    }

    QString relativePath(const QString& path)
    {
        return QApplication::applicationDirPath() + "/" + path;
    }
}