#include "windoweventwrapper.h"

#include <QApplication>
#include <QWindow>
#include <QMoveEvent>
#include <QRect>
#include <QResizeEvent>
#include <QScreen>

#include <memory>

WindowEventWrapper::WindowEventWrapper(QWindow* window)
    : EventWrapper(reinterpret_cast<QObject*>(window))
    , currentScreen_(getActualCurrentScreen(window))
{
    connect(this, &EventWrapper::moved, this, &WindowEventWrapper::On_target_geometryChanged);
    connect(this, &EventWrapper::resized, this, &WindowEventWrapper::On_target_geometryChanged);
}

WindowEventWrapper::~WindowEventWrapper()
{}

void WindowEventWrapper::On_target_geometryChanged()
{
    QWindow* target = TargetWindow();
    QScreen* newScreen = getActualCurrentScreen(target);

    if (currentScreen_ != newScreen)
    {
        ScreenChangedEventPtr e = std::make_shared<ScreenChangedEvent>();
        e->currentScreen = currentScreen_;
        e->newScreen = newScreen;

        emit screenChanged(target, e);

        currentScreen_ = newScreen;
    }
}

QScreen* WindowEventWrapper::getActualCurrentScreen(QWindow* window)
{
    QRect windowRect = window->geometry();
    QScreen* windowScreen = window->screen();
    if (isRectInScreen(windowScreen, windowRect))
        return windowScreen;

    // 겹침 비율이 가장 큰 스크린을 구함
    double maxPercent = -1.0;
    QScreen* maxScreen = nullptr;

    QList<QScreen*> screens = QApplication::screens();
    for (auto screen : screens)
    {
        if (screen == windowScreen)
            continue;

        double percent = getPercentForIntersectInRect(screen, windowRect);
        if (percent > 0)
        {
            if (maxScreen == nullptr || maxPercent < percent)
                maxScreen = screen;
        }
    }

    return maxScreen;
}

bool WindowEventWrapper::isRectInScreen(QScreen* screen, const QRect& rect)
{
    // 겹친 영역이 50%보다 많다면 true, 아니면 false
    return getPercentForIntersectInRect(screen, rect);
}

double WindowEventWrapper::getPercentForIntersectInRect(QScreen* screen, const QRect& rect)
{
    QRect intersect = getIntersectRect(screen, rect);

    int rectArea = rect.width() * rect.height();
    int intersectArea = intersect.width() * intersect.height();

    // 겹친영역의 비율 구함
    return static_cast<double>(intersectArea) / rectArea;
}

QRect WindowEventWrapper::getIntersectRect(QScreen* screen, const QRect& rect)
{
    return screen->geometry().intersected(rect);
}