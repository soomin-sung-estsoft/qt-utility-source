#include "log.h"
#include "clipboardworker.h"

#include <QApplication>
#include <QFile>
#include <QPixmap>

#include <thread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Log::InstallLogHandler("TestApp", "C:\\Test");

    // clipboard 복사 완료 시
    QObject::connect(&g_Clipboard, &ClipboardWorker::sig_clipboard_copied, [&a]()
    {
        LOG_INFO << "Copy to Clipboard Completed!";

        // 종료
        a.exit(0);
    });

    // test.jpg 불러오기
    QPixmap pixmap(".\\test.jpg");

    // 미리 pixmap 데이터를 가공
    g_Clipboard.SetPixmapData(pixmap);

    // 필요할 때마다 clipboard에 복사
    g_Clipboard.CopyToClipboard();

    return a.exec();
}