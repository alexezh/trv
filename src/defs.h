#pragma once

///////////////////////////////////////////////////////////////////////////////
//
typedef unsigned __int64 QWORD;

///////////////////////////////////////////////////////////////////////////////
//
#define IFC(val) { hr = (val); if(FAILED(hr)) { goto Cleanup; } }
#define IFC_WIN32(val) if((val) != 0) { hr = HRESULT_FROM_WIN32(GetLastError()); goto Cleanup; }

///////////////////////////////////////////////////////////////////////////////
//
#define WM_LOAD_BLOCK (WM_USER+1)
#define WM_LOAD_END (WM_USER+2)
#define WM_TOGGLE_SELECTED (WM_USER+3)
#define WM_QUEUE_WORK (WM_USER+5)

