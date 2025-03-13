#include "eventwrapper.h"

EventWrapper::EventWrapper(QObject* target)
    : QObject()
    , target_(target)
{
    target_->installEventFilter(this);
}

EventWrapper::~EventWrapper()
{
    target_->removeEventFilter(this);
}

bool EventWrapper::eventFilter(QObject* obj, QEvent* e)
{
    // 대상 아닐경우
    if (target_ == obj)
    {
        // event 타입에 따라 시그널 호출
        switch (e->type())
        {
            case QEvent::Move:
                emit moved(target_, reinterpret_cast<QMoveEvent*>(e));
                break;

            case QEvent::Resize:
                emit resized(target_, reinterpret_cast<QResizeEvent*>(e));
                break;
            case::QEvent::Show:
                emit showed(target_, reinterpret_cast<QShowEvent*>(e));
                break;
        }
    }

    return QObject::eventFilter(obj, e);
}