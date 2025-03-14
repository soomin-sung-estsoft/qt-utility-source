#pragma once

#include <QObject>

#include <future>
#include <memory>
#include <windows.h>

#define g_Clipboard ClipboardWorker::instance()

namespace Gdiplus
{
    class Bitmap;
}

class QImage;
class QPixmap;
class QByteArray;
class GdiplusContext;

class ClipboardWorker : public QObject
{
    Q_OBJECT

private:
    ClipboardWorker();

public:
    ~ClipboardWorker();

    static ClipboardWorker& instance();

private:
    struct PixmapData
    {
        HBITMAP hBitmap;
        Gdiplus::Bitmap* bitmap;
        QByteArray bytes;

        void Clear();
        bool IsEmpty() const;
    };

public:
    bool SetPixmapData(const QPixmap& pixmap, bool waitToBeforeProcess = true);
    bool IsRunningSetPixmapData() const;
    bool CopyToClipboard(bool waitSetPixmapData = true);
    bool IsRunningCopyToClipboard() const;

signals:
    void sig_clipboard_copied();

private:
    void setPixmapDataImpl(const QPixmap& pixmap);
    void copyToClipboardImpl(bool waitSetPixmapData);
    Gdiplus::Bitmap* imageToBitmap(const QImage& image);
    QByteArray pixmapToBytes(const QPixmap& pixmap);
    HGLOBAL createDIBv5Header(Gdiplus::Bitmap* image);
    HGLOBAL createPNGClipboardData(QByteArray bytes);
    HBITMAP toHBITMAP(const QImage& image);

private:
    std::future<void> copyToClipboardFuture_;
    std::future<void> setPixmapFuture_;
    PixmapData pixmapData_;
};