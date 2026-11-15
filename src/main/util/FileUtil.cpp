//
// Created by NHCM on 2026/11/6.
//

#include "FileUtil.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace trtutils
{
    bool exists(const QString& path)
    {
        return QFileInfo::exists(path);
    }

    QByteArray readAllBytes(const QString& path)
    {
        return readAllBytes(path, {});
    }

    QByteArray readAllBytes(const QString& path, QIODeviceBase::OpenMode extra_flags)
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | extra_flags))
        {
            return file.readAll();
        }
        return {};
    }

    bool writeAllBytes(const QString& path, const QByteArray& data)
    {
        return writeAllBytes(path, data, {});
    }

    bool writeAllBytes(const QString& path, const QByteArray& data, QIODeviceBase::OpenMode flags)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | flags))
            return false;

        qint64 offset = 0;
        while (offset < data.size())
        {
            const qint64 written = file.write(data.constData() + offset, data.size() - offset);
            if (written <= 0)
                return false;
            offset += written;
        }
        return file.flush();
    }

    bool writeAllBytesAtomically(const QString& path, const QByteArray& data)
    {
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
            return false;
        QSaveFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit();
    }

    QString relativePath(const QString& path)
    {
        return QApplication::applicationDirPath() + "/" + path;
    }

    QString appendPath(const QString& base, const QString& path)
    {
        return QDir(base).filePath(path);
    }
}
