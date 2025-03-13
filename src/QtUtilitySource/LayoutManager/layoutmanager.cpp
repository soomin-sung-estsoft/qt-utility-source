#include "layoutmanager.h"
#include "windoweventwrapper.h"

#include <QBoxLayout>
#include <QDebug>
#include <QWidget>
#include <QWindow>
#include <QVariant>
#include <QScreen>
#include <QEvent>
#include <QMoveEvent>
#include <QResizeEvent>

#include <stdexcept>

namespace
{
    const char* EXCLUDE_ADJUST_KEY = "excludeAdjustWidget";
    const qreal DEFAULT_DPI_VALUE = 96.0;

    bool isWidthFixed(QWidget* widget)
    {
        return widget->minimumWidth() == widget->maximumWidth() && widget->minimumWidth() > 0;
    }

    bool isHeightFixed(QWidget* widget)
    {
        return widget->minimumHeight() == widget->maximumHeight() && widget->minimumHeight() > 0;
    }
}

LayoutManager::LayoutManager(QWidget* parent)
    : QObject(reinterpret_cast<QObject*>(parent))
    , init_(false)
    , initSizes_()
    , windowEventWrapper_()
    , scaleFactor_(1.0)
{}

LayoutManager::~LayoutManager()
{
    QWindow* parentWindow = getParentWindow();
    if (parentWindow)
    {
        QScreen* screen = parentWindow->screen();
        disconnect(screen, &QScreen::logicalDotsPerInchChanged, this, &LayoutManager::On_screen_logicalDotsPerInchChanged);
    }
}

void LayoutManager::AdjustSize()
{
    static qreal lastAppliedScaleFactor = scaleFactor_;

    QWidget* widget = reinterpret_cast<QWidget*>(parent());

    for (QWidget* child : initSizes_.keys())
    {
        if (IsExcludeAdjust(*child))
            continue;

        SizeData data = initSizes_[child];
        if (lastAppliedScaleFactor < scaleFactor_)
        {
            // 커져야할 때
            child->setMaximumSize(calc(data.maximumSize_.width()), calc(data.maximumSize_.height()));
            child->resize(
                calc(child->width() / lastAppliedScaleFactor),
                calc(child->height() / lastAppliedScaleFactor));
            child->setMinimumSize(calc(data.minimumSize_.width()), calc(data.minimumSize_.height()));
        }
        else
        {
            // 작아져야할 때
            child->setMinimumSize(calc(data.minimumSize_.width()), calc(data.minimumSize_.height()));
            child->resize(
                calc(child->width() / lastAppliedScaleFactor),
                calc(child->height() / lastAppliedScaleFactor));
            child->setMaximumSize(calc(data.maximumSize_.width()), calc(data.maximumSize_.height()));
        }
    }

    for (QBoxLayout* layout : initLayoutDatas_.keys())
    {
        if (IsExcludeAdjust(*layout))
            continue;

        LayoutData data = initLayoutDatas_[layout];
        layout->setContentsMargins(QMargins(
            calc(data.margins_.left()),
            calc(data.margins_.top()),
            calc(data.margins_.right()),
            calc(data.margins_.bottom()))
        );
        layout->setSpacing(calc(data.spacing_));
    }

    lastAppliedScaleFactor = scaleFactor_;

    // signal 호출
    emit adjusted();
}

void LayoutManager::SetExcludeAdjust(QObject& widget) const
{
    widget.setProperty(EXCLUDE_ADJUST_KEY, true);
}

bool LayoutManager::IsExcludeAdjust(QObject& widget) const
{
    return widget.property(EXCLUDE_ADJUST_KEY).toBool();
}

void LayoutManager::Initialize()
{
    if (!init_)
    {
        init_ = true;

        QWidget* widget = reinterpret_cast<QWidget*>(parent());

        // parentwindow 가져오기
        setParentWindow(widget->window());
        // parentwindow의 screen에 이벤트 연결
        updateScreenConnections(nullptr, getParentWindow()->screen());

        // 자식 위젯 및 레이아웃 사이즈 기억
        QList<QWidget*> children = widget->findChildren<QWidget*>(QString(), Qt::FindChildrenRecursively);
        for (QWidget* child : children)
        {
            if (IsExcludeAdjust(*child))
                continue;

            qDebug() << child->objectName();

            SizeData sizeData;
            sizeData.maximumSize_ = child->maximumSize();
            sizeData.minimumSize_ = child->minimumSize();
            initSizes_[child] = sizeData;

            // layout
            QBoxLayout* layout = reinterpret_cast<QBoxLayout*>(child->layout());
            if (!layout || IsExcludeAdjust(*layout))
                continue;

            LayoutData layoutData;
            layoutData.margins_ = layout->contentsMargins();
            layoutData.spacing_ = layout->spacing();

            initLayoutDatas_[layout] = layoutData;
        }

        // 사이즈 재조정
        AdjustSize();
    }
    else
    {
        throw new std::runtime_error("LayoutManager is already initialized");
    }
}

bool LayoutManager::IsInitialized() const
{
    return init_;
}

qreal LayoutManager::ScaleFactor() const
{
    return scaleFactor_;
}

void LayoutManager::setParentWindow(QWidget* windowWidget)
{
    if (windowEventWrapper_)
        throw new std::runtime_error("Parent Window is already initialized");

    if (!windowWidget)
        throw new std::runtime_error("windowWidget is null");

    QWindow* window = windowWidget->windowHandle();
    if (!window)
        throw new std::runtime_error("cannot get windowHanlde");

    windowEventWrapper_ = std::make_shared<WindowEventWrapper>(window);
    connect(windowEventWrapper_.get(), &WindowEventWrapper::screenChanged, this, &LayoutManager::On_window_screenChaged);
}

QWindow* LayoutManager::getParentWindow()
{
    if (windowEventWrapper_)
        return windowEventWrapper_->TargetWindow();

    return nullptr;
}

void LayoutManager::updateScreenConnections(QScreen* currentScreen, QScreen* newScreen)
{
    if (currentScreen == newScreen)
        return;

    // 이전 스크린에 대한 connection 해제
    if (currentScreen)
    {
        disconnect(currentScreen, &QScreen::logicalDotsPerInchChanged, this, &LayoutManager::On_screen_logicalDotsPerInchChanged);
        updateScaleFactor(DEFAULT_DPI_VALUE);
    }

    // 새로운 스크린 connect
    if (newScreen)
    {
        connect(newScreen, &QScreen::logicalDotsPerInchChanged, this, &LayoutManager::On_screen_logicalDotsPerInchChanged);
        updateScaleFactor(newScreen->logicalDotsPerInch());
    }
}

void LayoutManager::updateScaleFactor(qreal dpi)
{
    scaleFactor_ = dpi / DEFAULT_DPI_VALUE;
}

int LayoutManager::calc(int v) const
{
    return qMin(qMax(static_cast<int>(v * scaleFactor_), 0), QWIDGETSIZE_MAX);
}

void LayoutManager::On_screen_logicalDotsPerInchChanged(qreal dpi)
{
    // scaleFactor 계산
    updateScaleFactor(dpi);

    // 사이즈 재조정
    AdjustSize();
}

void LayoutManager::On_window_screenChaged(QWindow* window, ScreenChangedEventPtr e)
{
    // 스크린 변경
    updateScreenConnections(e->currentScreen, e->newScreen);

    // 사이즈 재조정
    AdjustSize();
}