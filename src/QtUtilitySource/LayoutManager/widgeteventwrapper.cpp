#include "widgeteventwrapper.h"

#include <QShowEvent>

WidgetEventWrapper::WidgetEventWrapper(QWidget* widget)
    : EventWrapper(widget)
    , isFirstShowed_(true)
{
    connect(this, &EventWrapper::showed, this, &WidgetEventWrapper::On_target_showed);
}

WidgetEventWrapper::~WidgetEventWrapper()
{}

void WidgetEventWrapper::On_target_showed()
{
    if (isFirstShowed_)
    {
        isFirstShowed_ = false;

        QWidget* widget = TargetWidget();
        FirstShownEventPtr e = std::make_shared<FirstShownEvent>();

        emit firstShown(widget, e);
    }
}