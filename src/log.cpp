// trace.cpp : 
//
#include "stdafx.h"
#include <strsafe.h>
#include <stdio.h>
#include "log.h"

void TrvTrace(const char * pszFormat, ...)
{
    char outBuf[4096];
    va_list argList;
    va_start(argList, pszFormat);

    StringCchVPrintfA(outBuf, sizeof(outBuf), pszFormat, argList);
    OutputDebugStringA(outBuf);
}
