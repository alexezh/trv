// trace.cpp : 
//
#include "stdafx.h"
#include <strsafe.h>
#include <stdio.h>
#include "log.h"

void TrvTrace(const char* pszFunc, DWORD dwThreadId, const char * pszFormat, ...)
{
    char outBuf[4096];
    va_list argList;
    va_start(argList, pszFormat);

	StringCchPrintfA(outBuf, sizeof(outBuf), "%d %s", dwThreadId, pszFunc);
	size_t cchUsed = strlen(outBuf);

	StringCchVPrintfA(&outBuf[cchUsed], sizeof(outBuf) - cchUsed, pszFormat, argList);
	cchUsed = strlen(outBuf);

	cchUsed = std::min<size_t>(sizeof(outBuf) - 3, cchUsed);
	strcpy(&outBuf[cchUsed], "\r\n");

	OutputDebugStringA(outBuf);
}
