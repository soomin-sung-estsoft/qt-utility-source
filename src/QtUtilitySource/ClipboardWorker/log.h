#pragma once

#include <QDebug>
#include <QString>

#define LOG_DEBUG       qDebug() << __FUNCTION__
#define LOG_INFO        qInfo() << __FUNCTION__
#define LOG_WARNING     qWarning() << __FUNCTION__
#define LOG_CRITICAL  qCritical() << __FUNCTION__

namespace Log
{
    void InstallLogHandler(const QString& appId, const QString& appDataRootPath);
}