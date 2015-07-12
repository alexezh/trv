#pragma once

#define TRV_CONCAT4_(x1, x2, x3, x4) x1##x2##x3##x4
#define TRV_CONCAT4(x1, x2, x3, x4) TRV_CONCAT4_(x1, x2, x3, x4)
#define LOG(format, ...) TrvTrace(TRV_CONCAT4("%d ", __FUNCTION__, format, "\n"), GetCurrentThreadId(), __VA_ARGS__);

void TrvTrace(const char * pszFormat, ...);

