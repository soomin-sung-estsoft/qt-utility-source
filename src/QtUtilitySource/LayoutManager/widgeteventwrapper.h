#pragma once

#include "eventwrapper.h"

class QWidget;

class WidgetEventWrapper : public EventWrapper
{
    Q_OBJECT

public:
    WidgetEventWrapper(QWidget* widget);
    ~WidgetEventWrapper();

public:
    inline QWidget* TargetWidget()
    {
        return reinterpret_cast<QWidget*>(target_);
    }

signals:
    void firstShown(QWidget* widget, FirstShownEventPtr e);

private slots:
    void On_target_showed();

private:
    bool isFirstShowed_;
};