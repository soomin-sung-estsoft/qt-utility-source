#include "gdipluscontext.h"

#pragma comment(lib, "gdiplus.lib")

namespace
{
    static GdiplusContext& initObj = GdiplusContext::instance();
}

using namespace Gdiplus;

GdiplusContext::GdiplusContext()
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, NULL);
}

GdiplusContext::~GdiplusContext()
{
    GdiplusShutdown(gdiplusToken_);
}

GdiplusContext& GdiplusContext::instance()
{
    static GdiplusContext inst;
    return inst;
}

ULONG_PTR GdiplusContext::Token() const
{
    return gdiplusToken_;
}