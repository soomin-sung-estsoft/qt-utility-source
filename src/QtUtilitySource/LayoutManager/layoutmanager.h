#pragma once

#include "event_types.h"

#include <QObject>
#include <QMap>
#include <QSize>

#include <memory>

class QBoxLayout;
class QScreen;
class QWindow;
class WindowEventWrapper;

class LayoutManager : public QObject
{
    Q_OBJECT

public:
    explicit LayoutManager(QWidget* parent);
    ~LayoutManager();

private:
    struct SizeData
    {
        QSize maximumSize_;
        QSize minimumSize_;
    };

    struct LayoutData
    {
        QMargins margins_;
        int spacing_;
    };

public:
    void AdjustSize();
    void SetExcludeAdjust(QObject& widget) const;
    bool IsExcludeAdjust(QObject& widget) const;
    void Initialize();
    bool IsInitialized() const;
    qreal ScaleFactor() const;

signals:
    void adjusted();

private:
    void setParentWindow(QWidget* windowWidget);
    QWindow* getParentWindow();
    void updateScreenConnections(QScreen* currentScreen, QScreen* newScreen);
    void updateScaleFactor(qreal dpi);
    int calc(int v) const;

private slots:
    void On_screen_logicalDotsPerInchChanged(qreal dpi);
    void On_window_screenChaged(QWindow* window, ScreenChangedEventPtr e);

private:
    bool init_;
    qreal scaleFactor_;
    std::shared_ptr<WindowEventWrapper> windowEventWrapper_;
    QMap<QWidget*, SizeData> initSizes_;
    QMap<QBoxLayout*, LayoutData> initLayoutDatas_;
};