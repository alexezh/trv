#pragma once

#if 1
#define LOG(format, ...) TrvTrace(__FUNCTION__, GetCurrentThreadId(), format, __VA_ARGS__);
#else
#define LOG(format,...)
#endif

void TrvTrace(const char* pszFunc, DWORD dwThreadId, const char * pszFormat, ...);

