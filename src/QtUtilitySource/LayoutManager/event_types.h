#pragma once

#include <memory>

class QScreen;

struct ScreenChangedEvent
{
    QScreen* currentScreen;
    QScreen* newScreen;
};
using ScreenChangedEventPtr = std::shared_ptr<ScreenChangedEvent>;

struct FirstShownEvent
{
};
using FirstShownEventPtr = std::shared_ptr<FirstShownEvent>;