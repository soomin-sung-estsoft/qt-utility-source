#pragma once

#include "event_types.h"

#include <QWidget>
#include <QEvent>

class EventWrapper : public QObject
{
    Q_OBJECT

public:
    explicit EventWrapper(QObject* target);
    ~EventWrapper();

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

public:
    inline QObject* Target() {
        return target_;
    }

signals:
    void moved(QObject* sender, QMoveEvent* e);
    void resized(QObject* sender, QResizeEvent* e);
    void showed(QObject* ender, QShowEvent* e);

protected:
    QObject* target_;
};