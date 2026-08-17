#include "AppSettings.h"
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace
{
const char* kOrganization = "VisionStudio";
const char* kApplication = "VisionStudioOpenCV";

QString GetSystemFallbackDir(const QString& key)
{
    if (key == AppSettings::KeyImageDir || key == AppSettings::KeyTemplateDir)
    {
        QString picDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        if (!picDir.isEmpty() && QDir(picDir).exists())
        {
            return picDir;
        }
    }
    else if (key == AppSettings::KeyProjectDir || key == AppSettings::KeyExportDir)
    {
        QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (!docDir.isEmpty() && QDir(docDir).exists())
        {
            return docDir;
        }
    }

    return QDir::currentPath();
}
}

QString AppSettings::GetLastDirectory(const QString& key, const QString& fallbackDir)
{
    QSettings settings(kOrganization, kApplication);
    QString savedPath = settings.value(key).toString();

    // 1. 优先使用已持久化的有效目录
    if (!savedPath.isEmpty() && QDir(savedPath).exists())
    {
        return QDir::toNativeSeparators(savedPath);
    }

    // 2. 检查传入的 fallback 目录
    if (!fallbackDir.isEmpty() && QDir(fallbackDir).exists())
    {
        return QDir::toNativeSeparators(fallbackDir);
    }

    // 3. 自动回退到系统标准位置
    return QDir::toNativeSeparators(GetSystemFallbackDir(key));
}

void AppSettings::SetLastDirectory(const QString& key, const QString& pathOrFilePath)
{
    if (pathOrFilePath.isEmpty())
    {
        return;
    }

    QFileInfo fileInfo(pathOrFilePath);
    QString dirPath;

    if (fileInfo.isDir())
    {
        dirPath = fileInfo.absoluteFilePath();
    }
    else
    {
        dirPath = fileInfo.absolutePath();
    }

    if (!dirPath.isEmpty() && QDir(dirPath).exists())
    {
        QSettings settings(kOrganization, kApplication);
        settings.setValue(key, dirPath);
        settings.sync();
    }
}

QString AppSettings::GetDefaultFilePath(const QString& key, const QString& defaultFileName)
{
    QString dir = GetLastDirectory(key);
    return QDir(dir).filePath(defaultFileName);
}
