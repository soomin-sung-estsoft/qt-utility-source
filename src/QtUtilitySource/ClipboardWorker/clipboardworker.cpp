#include "clipboardworker.h"
#include "log.h"
#include "gdipluscontext.h"

#include <QBuffer>
#include <QPixmap>
#include <QImage>
#include <QDebug>

#include <future>
#include <mutex>

using namespace Gdiplus;
using namespace std::chrono_literals;

namespace
{
    class GlobalDeleter
    {
    public:
        void operator()(HGLOBAL hGlobal) const {
            if (hGlobal)
            {
                GlobalUnlock(hGlobal);
                GlobalFree(hGlobal);
            }
        }
    };
}

ClipboardWorker::ClipboardWorker()
    : QObject()
    , copyToClipboardFuture_()
    , setPixmapFuture_()
    , pixmapData_()
{}

ClipboardWorker::~ClipboardWorker()
{
    pixmapData_.Clear();
}

ClipboardWorker& ClipboardWorker::instance()
{
    static ClipboardWorker instance;
    return instance;
}

// Public

bool ClipboardWorker::SetPixmapData(const QPixmap& pixmap, bool waitToBeforeProcess)
{
    LOG_INFO;

    if (IsRunningSetPixmapData())
    {
        if (waitToBeforeProcess)
        {
            LOG_INFO << "Wait to before process";
            setPixmapFuture_.wait();
        }
        else
        {
            LOG_WARNING << "SetPixmapData is already running";
            return false;
        }
    }

    // async 실행
    setPixmapFuture_ = std::async(std::launch::async, &ClipboardWorker::setPixmapDataImpl, this, pixmap);

    return true;
}

bool ClipboardWorker::IsRunningSetPixmapData() const
{
    if (!setPixmapFuture_.valid())
        return false;

    return setPixmapFuture_.wait_for(0ms) != std::future_status::ready;
}

bool ClipboardWorker::CopyToClipboard(bool waitSetPixmapData)
{
    LOG_INFO;

    if (IsRunningCopyToClipboard())
    {
        LOG_WARNING << "CopyToClipboard is already running";
        return false;
    }

    // async 실행
    copyToClipboardFuture_ = std::async(std::launch::async, &ClipboardWorker::copyToClipboardImpl, this, waitSetPixmapData);

    return true;
}

bool ClipboardWorker::IsRunningCopyToClipboard() const
{
    if (!copyToClipboardFuture_.valid())
        return false;

    return copyToClipboardFuture_.wait_for(0ms) != std::future_status::ready;
}

// Private

void ClipboardWorker::setPixmapDataImpl(const QPixmap& pixmap)
{
    LOG_INFO;

    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    LOG_INFO << "Start";

    QImage image = pixmap.toImage();

    pixmapData_.Clear();
    pixmapData_.bitmap = imageToBitmap(image);
    pixmapData_.hBitmap = toHBITMAP(image);
    pixmapData_.bytes = pixmapToBytes(pixmap);

    LOG_INFO << "Finished";
}

void ClipboardWorker::copyToClipboardImpl(bool waitSetPixmapData)
{
    LOG_INFO;

    if (IsRunningSetPixmapData())
    {
        if (waitSetPixmapData)
        {
            LOG_INFO << "Wait to SetPixmapData";
            setPixmapFuture_.wait();
        }
        else
        {
            LOG_WARNING << "SetPixmapData is running";
            return;
        }
    }

    if (pixmapData_.IsEmpty())
    {
        LOG_WARNING << "PixmapData is empty";
        return;
    }

    // 비트맵
    Bitmap* bitmap = pixmapData_.bitmap;
    QByteArray data = pixmapData_.bytes;

    // hGlobal 생성
    HGLOBAL hGlobal = createDIBv5Header(bitmap);
    if (hGlobal == nullptr)
        return;

    std::unique_ptr<void, GlobalDeleter> hGlobalPtr(hGlobal);

    // hGlobalPNG 생성
    HGLOBAL hGlobalPNG = createPNGClipboardData(data);
    if (hGlobalPNG == nullptr)
        return;

    std::unique_ptr<void, GlobalDeleter> hGlobalPNGPtr(hGlobalPNG);

    UINT customFormatPNG = RegisterClipboardFormatW(L"PNG");
    if (!OpenClipboard(NULL))
        return;

    EmptyClipboard();
    SetClipboardData(customFormatPNG, hGlobalPNGPtr.release());
    SetClipboardData(CF_DIBV5, hGlobalPtr.release());
    SetClipboardData(CF_BITMAP, pixmapData_.hBitmap);

    CloseClipboard();

    QMetaObject::invokeMethod(this, &ClipboardWorker::sig_clipboard_copied, Qt::QueuedConnection);
}

Bitmap* ClipboardWorker::imageToBitmap(const QImage& image)
{
    LOG_INFO;
    LOG_INFO << "Image size: " << image.size();

    int imageWidth = image.width();
    int imageHeight = image.height();

    Bitmap* bitmap = new Bitmap(imageWidth, imageHeight, PixelFormat32bppARGB);

    // QImage의 데이터를 Bitmap으로 복사
    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            QColor color = image.pixelColor(x, y);
            Color gpColor(color.alpha(), color.red(), color.green(), color.blue());
            bitmap->SetPixel(x, y, gpColor.GetValue());
        }
    }

    return bitmap;
}

QByteArray ClipboardWorker::pixmapToBytes(const QPixmap& pixmap)
{
    QByteArray data;
    QBuffer buffer(&data);
    pixmap.save(&buffer, "PNG");

    return data;
}

HGLOBAL ClipboardWorker::createDIBv5Header(Bitmap* image)
{
    int width = image->GetWidth();
    int height = image->GetHeight();

    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPV5HEADER) + width * height * 4);
    if (!hGlobal)
        return nullptr;

    BITMAPV5HEADER* dibv5Data = (BITMAPV5HEADER*)GlobalLock(hGlobal);
    if (!dibv5Data)
        return nullptr;

    dibv5Data->bV5Size = sizeof(BITMAPV5HEADER);
    dibv5Data->bV5Width = width;
    dibv5Data->bV5Height = -height;
    dibv5Data->bV5Planes = 1;
    dibv5Data->bV5BitCount = 32;
    dibv5Data->bV5Compression = BI_BITFIELDS;
    dibv5Data->bV5SizeImage = 0;
    dibv5Data->bV5XPelsPerMeter = 0;
    dibv5Data->bV5YPelsPerMeter = 0;
    dibv5Data->bV5ClrUsed = 0;
    dibv5Data->bV5ClrImportant = 0;
    dibv5Data->bV5RedMask = 0x00FF0000;
    dibv5Data->bV5GreenMask = 0x0000FF00;
    dibv5Data->bV5BlueMask = 0x000000FF;
    dibv5Data->bV5AlphaMask = 0xFF000000;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Gdiplus::Color pixelColor;
            image->GetPixel(x, y, &pixelColor);

            ARGB* destPixel = (ARGB*)((BYTE*)dibv5Data + sizeof(BITMAPV5HEADER)) + (y * width) + x;
            *destPixel = pixelColor.GetValue();
        }
    }

    GlobalUnlock(hGlobal);
    return hGlobal;
}

HGLOBAL ClipboardWorker::createPNGClipboardData(QByteArray bytes)
{
    std::vector<BYTE> pngData(bytes.begin(), bytes.end());

    HGLOBAL hGlobalPNG = GlobalAlloc(GMEM_MOVEABLE, pngData.size());
    if (!hGlobalPNG)
        return nullptr;

    BYTE* pDataPNG = static_cast<BYTE*>(GlobalLock(hGlobalPNG));
    if (!pDataPNG)
        return nullptr;

    memcpy(pDataPNG, pngData.data(), pngData.size());
    GlobalUnlock(hGlobalPNG);
    return hGlobalPNG;
}

HBITMAP ClipboardWorker::toHBITMAP(const QImage& image)
{
    // QImage를 BMP 형식으로 QByteArray에 저장
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "BMP");

    // BITMAPFILEHEADER와 BITMAPINFOHEADER 추출
    BITMAPFILEHEADER* bmpFileHeader = (BITMAPFILEHEADER*)byteArray.constData();
    BITMAPINFOHEADER* bmpInfoHeader = (BITMAPINFOHEADER*)(byteArray.constData() + sizeof(BITMAPFILEHEADER));

    // HBITMAP 생성 및 픽셀 데이터 설정
    HBITMAP hBitmap = CreateCompatibleBitmap(GetDC(NULL), bmpInfoHeader->biWidth, bmpInfoHeader->biHeight);
    SetDIBits(GetDC(NULL), hBitmap, 0, bmpInfoHeader->biHeight, byteArray.constData() + bmpFileHeader->bfOffBits, (BITMAPINFO*)bmpInfoHeader, DIB_RGB_COLORS);

    return hBitmap;
}

// PixmapData struct

void ClipboardWorker::PixmapData::Clear()
{
    if (hBitmap)
        DeleteObject(hBitmap);

    if (bitmap)
        delete bitmap;

    hBitmap = nullptr;
    bitmap = nullptr;
    bytes = QByteArray();
}

bool ClipboardWorker::PixmapData::IsEmpty() const
{
    return hBitmap == nullptr && bitmap == nullptr && bytes.isEmpty();
}