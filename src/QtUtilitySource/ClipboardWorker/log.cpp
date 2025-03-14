#include "log.h"

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QThread>
#include <QDateTime>
#include <QDir>
#include <QTextStream>

#include <windows.h>

namespace
{
    const quint64 MAX_LOG_FILE_SIZE = 1 * 1024 * 1024; // 1MB
    QString appId_;
    QString appDataRootPath_;

    // https://doc.qt.io/qt-5/qtglobal.html#qInstallMessageHandler
    void logOutputHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
    {
        Q_UNUSED(context);

        QString keyBase = QString("HKEY_CURRENT_USER\\Software\\ESTsoft\\%0\\Trace").arg(appId_);
        QSettings setting(keyBase, QSettings::NativeFormat, QCoreApplication::instance());
        int traceLevel = setting.value("trace").toInt();
        if (traceLevel == 0)
        {
            traceLevel = 3; // default log
            setting.setValue("trace", traceLevel);
        }

#ifdef _DEBUG
        traceLevel = 4;
#endif

        if (traceLevel < 1 && type == QtCriticalMsg)
            return;
        if (traceLevel < 2 && type == QtWarningMsg)
            return;
        if (traceLevel < 3 && type == QtInfoMsg)
            return;
        if (traceLevel < 4 && type == QtDebugMsg)
            return;

        QString dt = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString logText = QString("[%0][%1] ").arg(dt).arg(QCoreApplication::applicationPid());
        switch (type) {
            case QtDebugMsg:
                logText += QString("[Debug] %0").arg(msg);
                break;
            case QtInfoMsg:
                logText += QString("[Info] %0").arg(msg);
                break;
            case QtWarningMsg:
                logText += QString("[Warning] %0").arg(msg);
                break;
            case QtCriticalMsg:
                logText += QString("[Critical] %0").arg(msg);
                break;
            case QtFatalMsg:
                logText += QString("[Fatal] %0").arg(msg);
                //abort();
                break;
        }

        fprintf(stderr, "%s\n", qUtf8Printable(logText));
        fflush(stderr);

#ifdef _DEBUG
#ifdef Q_OS_WIN // https://forum.qt.io/topic/136623/qinstallmessagehandler-not-showing-anything-when-in-debug-mode/5
        ::OutputDebugStringA(qUtf8Printable(logText + "\n"));
#endif
#endif

        QString logFilePath = QString("%0/%1.log").arg(appDataRootPath_).arg(appId_);
        if (QDir(QFileInfo(logFilePath).absolutePath()).exists() == false)
            QDir().mkpath(QFileInfo(logFilePath).absolutePath());
        qint64 fileSize = QFile(logFilePath).size();
        if (fileSize > MAX_LOG_FILE_SIZE)
        {
            QFile::remove(logFilePath + ".old");
            QFile(logFilePath).rename(logFilePath + ".old");
        }

        static int retryCount = 0;
        do
        {
            QFile logFile(logFilePath);
            if (logFile.open(QIODevice::WriteOnly | QIODevice::Append))
            {
                QTextStream textStream(&logFile);
                textStream << logText << "\n";
                break;
            }
            else
            {
                Q_ASSERT(false);
                ::OutputDebugStringW(L"[Lemon] Log write failed.");
                ++retryCount;
                QThread::msleep(100);
            }
        } while (retryCount % 3 && retryCount < 30);
    }
}

namespace Log
{
    void InstallLogHandler(const QString& appId, const QString& appDataRootPath)
    {
        static bool isInstalled = false;

        if (appId.isEmpty() || appDataRootPath.isEmpty())
            return;

        if (!isInstalled)
        {
            isInstalled = true;
            appId_ = appId;
            appDataRootPath_ = appDataRootPath;
            qInstallMessageHandler(logOutputHandler);

            LOG_INFO << "INSTALLED LOG HANDLER";
        }
    }
}