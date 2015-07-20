// Copyright (c) 2013 Alexandre Grigorovitch (alexezh@gmail.com).
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

#include "resource.h"
#include "persist.h"

class CTraceSource;

///////////////////////////////////////////////////////////////////////////////
// 
enum class ColumnId
{
    LineNumber,
    Time,
	ThreadId,
	Message,
	MaxColumn
};

///////////////////////////////////////////////////////////////////////////////
// 
#pragma pack(push, 1)
class CTraceViewConfig
{
public:
    enum
    {
        CFG_VER = 0x5,
    };

    int m_Version;

    // column sizes. 32 columns max
	int m_ColumnWidth[ColumnId::MaxColumn];
};
#pragma pack(pop)
///////////////////////////////////////////////////////////////////////////////
//
class CTraceView 
        : public CWindowImpl<CTraceView>
        , public CPersistHandler
{
public:
    CTraceView();
    ~CTraceView();
    
    void Init(CTraceApp * pApp);

    // CPersistHandler
    HRESULT SaveConfig(HKEY hKey);
    HRESULT LoadConfig(HKEY hKey);
    
public:
    void OnShowFiltered(BOOL fVal);
    void OnFilterChange();    

    // loads from file
    void LoadView();

    // update screen
    void Repaint();

    // finds line in the filtered file, positions cursor to the selected line
    void Find(LPCWSTR pszExpr, BOOL fStart);
    
    // return selected lines in global memory 
    HRESULT GetSelectedLines(HANDLE * phData);

    // returns selected line indexes
    HRESULT GetSelectedLineNumbers(DWORD * pnStart, DWORD * pnFinish);

	// reinitialize columns
    HRESULT InitColumns();

    HWND SetFocus() throw()
    {
        return m_ListView.SetFocus();
    }

    DWORD GetFocusLine() { return m_nFocusLine; }
    BOOL IsShowFiltered() { return m_fHide; }
    
private:
    
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnUpdateView(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDblClk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnCacheHint(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   
    // display dialog for a single line
    void OutputLineToConsole(DWORD nLine);

    DWORD GetFileLineNum(DWORD nItem);
    
    HRESULT InsertColumn(DWORD nIndex, DWORD nWidth, LPCWSTR pszText);
	void PopulateInfo(const char* psz, size_t cch, LV_DISPINFO* lpdi);

    // deselects all items
    void DeselectAll();

    void UpdateView(int yFocusPos);

    int GetFocusPosition();

protected:
    
    BEGIN_MSG_MAP(CTraceView)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
//		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		NOTIFY_HANDLER(ID_TRACEVIEW, LVN_GETDISPINFO, OnGetDispInfo)
        NOTIFY_HANDLER(ID_TRACEVIEW, LVN_ITEMCHANGED, OnItemChanged)
        NOTIFY_HANDLER(ID_TRACEVIEW, NM_CUSTOMDRAW, OnCustomDraw)
        NOTIFY_HANDLER(ID_TRACEVIEW, NM_DBLCLK, OnDblClk)
        NOTIFY_HANDLER(ID_TRACEVIEW, LVN_KEYDOWN, OnKeyDown)
		// NOTIFY_HANDLER(ID_TRACEVIEW, LVN_ODCACHEHINT, OnCacheHint)
    END_MSG_MAP()

private:
    CWindow m_ListView;

    CTraceApp * m_pApp;

	CTraceViewConfig m_Config;

    // for now we read the whole file
	CTraceSource * m_pColl;

    // map from filtered lines to file lines
    std::vector<DWORD> m_ActiveLines;

    // current selected line
    DWORD m_nFocusLine;
   
    BOOL m_fHide;

	// list of active columns
	std::vector<ColumnId> m_Columns;
};
