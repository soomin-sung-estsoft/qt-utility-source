#pragma once

#include <windows.h>
#include <gdiplus.h>

class GdiplusContext
{
private:
    GdiplusContext();

public:
    ~GdiplusContext();

    static GdiplusContext& instance();

public:
    ULONG_PTR Token() const;

private:
    ULONG_PTR gdiplusToken_;
};