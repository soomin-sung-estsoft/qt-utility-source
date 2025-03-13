#pragma once

#include "eventwrapper.h"

class QWindow;

class WindowEventWrapper : public EventWrapper
{
    Q_OBJECT

public:
    WindowEventWrapper(QWindow* window);
    ~WindowEventWrapper();

public:
    inline QWindow* TargetWindow() {
        return reinterpret_cast<QWindow*>(target_);
    }

signals:
    void screenChanged(QWindow* sender, ScreenChangedEventPtr e);

private slots:
    void On_target_geometryChanged();

private:
    QScreen* getActualCurrentScreen(QWindow* window);
    bool isRectInScreen(QScreen* screen, const QRect& rect);
    double getPercentForIntersectInRect(QScreen* screen, const QRect& rect);
    QRect getIntersectRect(QScreen* screen, const QRect& rect);

private:
    QScreen* currentScreen_;
};