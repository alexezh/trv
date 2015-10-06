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
#include "stdafx.h"

#include <strsafe.h>
#include "traceapp.h"
#include "traceview.h"
#include "outputview.h"
#include "jshost.h"
#include "stringref.h"
#include "file.h"
#include "strstr.h"
#include "color.h"
#include "bitset.h"
#include "log.h"

///////////////////////////////////////////////////////////////////////////////
//
static DWORD FindIndex(std::vector<DWORD> & coll, DWORD line)
{
	size_t mn = coll.size();

	for (size_t n = 0; n < mn; n++)
	{
		if (coll[n] >= line)
		{
			return n;
		}
	}

	return (coll.size() > 0) ? 0 : coll.size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT TraceListView::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = false;

	if (wParam == 'C' && GetAsyncKeyState(VK_CONTROL))
	{
		if (m_pOwner)
			m_pOwner->OnCopy();

		bHandled = true;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
CTraceView::CTraceView()
	: m_nFocusLine(0)
	, m_fHide(FALSE)
{
}

CTraceView::~CTraceView()
{
}

void CTraceView::Init(CTraceApp * pApp)
{
	m_pApp = pApp;

	m_pApp->PPersist()->RegisterHandler(this);
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CTraceView::SaveConfig(HKEY hKey)
{
	return S_OK;
}

HRESULT CTraceView::LoadConfig(HKEY hKey)
{
	LONG lResult;
	DWORD n;
	DWORD cb = 0;

	ZeroMemory(&m_Config, sizeof(m_Config));

	lResult = RegQueryValueEx(hKey, L"Config", NULL, NULL, NULL, &cb);

	if (lResult == ERROR_SUCCESS && cb == sizeof(m_Config))
	{
		lResult = RegQueryValueEx(hKey, L"Config", NULL, NULL, (BYTE*) &m_Config, &cb);

		if (lResult == ERROR_SUCCESS)
		{
			if (m_Config.m_Version == CTraceViewConfig::CFG_VER)
			{
				goto Cleanup;
			}
		}
	}

	// otherwise reinit the structure
	m_Config.m_Version = CTraceViewConfig::CFG_VER;

	// set default column width. 
	for (n = 0; n < (size_t) ColumnId::MaxColumn; n++)
	{
		m_Config.m_ColumnWidth[n] = 60;
	}

	// last column is bigger
	m_Config.m_ColumnWidth[(size_t) ColumnId::Message] = 4096;

Cleanup:

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DWORD dwStyle;
	HRESULT hr = S_OK;
	RECT rect;

	dwStyle = WS_CHILD |
		WS_BORDER |
		WS_VISIBLE |
		LVS_REPORT |
		LVS_SHOWSELALWAYS |
		LVS_OWNERDATA |
		LVS_NOSORTHEADER; //  | LVS_NOCOLUMNHEADER;

	ZeroMemory(&rect, sizeof(rect));

	CWindow listView;
	if (listView.Create(WC_LISTVIEW, m_hWnd, &rect, NULL, dwStyle, 0, ID_TRACEVIEW) == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

	m_ListView.SubclassWindow(listView.m_hWnd);
	m_ListView.SetOwner(this);

	SetColumns({ ColumnId::Message });

	//set the number of items in the list
	ListView_SetExtendedListViewStyle(m_ListView.m_hWnd, LVS_EX_FULLROWSELECT);
	ListView_SetItemCount(m_ListView.m_hWnd, 0);

Cleanup:

	return 0;
}

LRESULT CTraceView::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	RECT rect;

	rect.top = 0;
	rect.left = 0;
	rect.right = LOWORD(lParam);
	rect.bottom = HIWORD(lParam);

	m_ListView.SetWindowPos(NULL,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		SWP_NOZORDER | SWP_NOREDRAW);

	bHandled = TRUE;

	return 0;
}

LRESULT CTraceView::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pApp->SetClipboardHandler(this);
	bHandled = true;
	return 0;
}

void CTraceView::OnCopy()
{
	HANDLE hMem;
	HRESULT hr;

	OpenClipboard();

	EmptyClipboard();

	IFC(GetSelectedLines(&hMem));

	SetClipboardData(CF_TEXT, hMem);

Cleanup:

	CloseClipboard();
}
void CTraceView::SetColumns(const std::vector<ColumnId>& ids)
{
	// remove all current columns
	for (size_t i = m_Columns.size(); i > 0; i--)
	{
		ListView_DeleteColumn(m_ListView.m_hWnd, i - 1);
	}

	HRESULT hr = S_OK;
	m_Columns.clear();
	auto lineInfoDesc = m_pSource->GetDesc();
	int columnIdx = 0;
	for (auto id : ids)
	{
		if (id == ColumnId::LineNumber)
		{
			IFC(InsertColumn(columnIdx, m_Config.m_ColumnWidth[static_cast<uint32_t>(id)], L"Line"));
		}
		else if (id == ColumnId::ThreadId)
		{
			IFC(InsertColumn(columnIdx, m_Config.m_ColumnWidth[static_cast<uint32_t>(id)], L"Tid"));
		}
		else if (id == ColumnId::Time)
		{
			IFC(InsertColumn(columnIdx, m_Config.m_ColumnWidth[static_cast<uint32_t>(id)], L"Time"));
		}
		else if (id == ColumnId::User1)
		{
			IFC(InsertColumn(columnIdx, m_Config.m_ColumnWidth[static_cast<uint32_t>(id)], L"User1"));
		}
		else if (id == ColumnId::User2)
		{
			IFC(InsertColumn(columnIdx, m_Config.m_ColumnWidth[static_cast<uint32_t>(id)], L"User2"));
		}
		else if (id == ColumnId::User3)
		{
			IFC(InsertColumn(columnIdx, m_Config.m_ColumnWidth[static_cast<uint32_t>(id)], L"User3"));
		}
		else if (id == ColumnId::User4)
		{
			IFC(InsertColumn(columnIdx, m_Config.m_ColumnWidth[static_cast<uint32_t>(id)], L"User4"));
		}
		else if (id == ColumnId::Message)
		{
			IFC(InsertColumn(columnIdx, 2048, L"Message"));
		}
		else
		{
			continue;
		}

		m_Columns.push_back((ColumnId) id);
		columnIdx++;
	}

Cleanup:
	;
}

DWORD CTraceView::GetFileLineNum(DWORD nItem)
{
	if (m_fHide)
	{
		return m_ActiveLines[nItem];
	}
	else
	{
		return nItem;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceView::LoadView()
{
	UpdateView(0);
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceView::OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	HRESULT hr = S_OK;
	LV_DISPINFO *lpdi = (LV_DISPINFO *) pnmh;
	DWORD nLine;

	nLine = GetFileLineNum(lpdi->item.iItem);

	if (lpdi->item.mask & LVIF_TEXT)
	{
		auto& line = m_LineCache->GetLine(nLine);
		if (!line.first)
			return 0;

		auto desc = m_pSource->GetDesc();
		ColumnId id = m_Columns[lpdi->item.iSubItem];
		switch (id)
		{
			case ColumnId::LineNumber:
			{
				char lineA[32];
				_itoa(line.second.GetLineIndex(), lineA, 10);
				PopulateInfo(lineA, strlen(lineA), lpdi);
			}
			break;
			case ColumnId::ThreadId:
				if (desc.Tid)
				{
					char tidA[32];
					_itoa(line.second.GetThreadId(), tidA, 10);
					PopulateInfo(tidA, strlen(tidA), lpdi);
				}
				break;
			case ColumnId::Time:
				if (desc.Time)
					PopulateInfo(line.second.GetTime(), lpdi);
				break;
			case ColumnId::User1:
				if (desc.GetUser(0))
					PopulateInfo(line.second.GetUser(0), lpdi);
				break;
			case ColumnId::User2:
				if (desc.GetUser(1))
					PopulateInfo(line.second.GetUser(1), lpdi);
				break;
			case ColumnId::User3:
				if (desc.GetUser(2))
					PopulateInfo(line.second.GetUser(2), lpdi);
				break;
			case ColumnId::User4:
				if (desc.GetUser(3))
					PopulateInfo(line.second.GetUser(3), lpdi);
				break;
			case ColumnId::Message:
				PopulateInfo(line.second.GetMsg(), lpdi);
				break;
		}
	}

	return 0;
}

void CTraceView::PopulateInfo(const char * psz, size_t cch, LV_DISPINFO *lpdi)
{
	LPCSTR pszSrc;
	LPCSTR pszEnd;
	LPWSTR pszDst = lpdi->item.pszText;
	int dstLeft = lpdi->item.cchTextMax;
	WCHAR c;

	if (cch > 0 && dstLeft > 1)
	{
		pszSrc = psz;
		pszEnd = ((DWORD) lpdi->item.cchTextMax > cch) ? psz + cch : psz + lpdi->item.cchTextMax - 1;
		for (; pszSrc < pszEnd && dstLeft > 1; )
		{
			c = (WCHAR) *pszSrc++;
			if (c == '\r' || c == '\n')
			{
				break;
			}

			*pszDst++ = c;
			dstLeft--;
		}
	}

	*pszDst = '\0';
}
///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceView::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	LPNMLISTVIEW pnmv = (LPNMLISTVIEW) pnmh;

	if ((pnmv->uNewState & LVIS_FOCUSED) != 0)
	{
		m_nFocusLine = GetFileLineNum(pnmv->iItem);
	}

	bHandled = TRUE;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceView::OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	LRESULT lRet = 0;
	LPNMLVCUSTOMDRAW pCD = (LPNMLVCUSTOMDRAW) pnmh;

	switch (pCD->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
			lRet = CDRF_NOTIFYITEMDRAW; // (CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW);
			break;

		case CDDS_ITEMPREPAINT:
		{
			DWORD nLine;
			BYTE bColor;

			nLine = GetFileLineNum(pCD->nmcd.dwItemSpec);

			bColor = m_pApp->PJsHost()->GetLineColor(nLine);

			pCD->clrText = CColor::Instance().GetForeColor(bColor);
			pCD->clrTextBk = CColor::Instance().GetBackColor(bColor);

			(void) SelectObject(pCD->nmcd.hdc, GetStockObject(ANSI_FIXED_FONT));

			lRet = CDRF_DODEFAULT; // (CDRF_NOTIFYPOSTPAINT | CDRF_NEWFONT);
			break;
		}
		case CDDS_ITEMPOSTPAINT:
		{

			lRet = CDRF_DODEFAULT;
			break;
		}
		default:
			break;
	}

	bHandled = TRUE;

	return lRet;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceView::OnDblClk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	OutputLineToConsole(m_nFocusLine);

	bHandled = TRUE;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceView::OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	NMTVKEYDOWN *lpkd = (NMTVKEYDOWN *) pnmh;

	if (lpkd->wVKey == VK_RETURN)
	{
		OutputLineToConsole(m_nFocusLine);
	}

	bHandled = TRUE;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
LRESULT CTraceView::OnCacheHint(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	NMLVCACHEHINT* pCachehint = (NMLVCACHEHINT*) pnmh;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceView::OnShowFiltered(BOOL fVal)
{
	int yFocusPos = GetFocusPosition();

	m_fHide = fVal;

	UpdateView(yFocusPos);
}

///////////////////////////////////////////////////////////////////////////////
//

void CTraceView::SetTraceSource(const std::shared_ptr<CTraceSource>& src)
{
	m_pSource = src;
	m_ActiveLines.clear();

	m_LineCache = std::make_shared<ViewLineCache>(m_pApp, m_pApp->PJsHost());
	m_LineCache->Resize(m_pSource->GetLineCount());
	m_LineCache->RegisterLineAvailableListener([this](DWORD idx)
	{
		// translate file line indexes to view indexes
		if (m_ActiveLines.size() > 0)
		{
			// for (DWORD i = idxStart; i < idxEnd; i++)
			assert(false);
		}
		else
		{
			// view index is the same as line index
			ListView_Update(m_ListView.m_hWnd, idx);
		}
	});

	UpdateView(0);
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceView::Repaint()
{
	// update view    
	if (m_fHide)
	{
		ListView_RedrawItems(m_ListView.m_hWnd, 0, m_ActiveLines.size());
	}
	else
	{
		ListView_RedrawItems(m_ListView.m_hWnd, 0, m_pSource->GetLineCount());
	}

	m_ListView.Invalidate(TRUE);
	UpdateView(0);
}

void CTraceView::SetViewSource(const std::shared_ptr<CBitSet>& lines)
{
	int yFocusPos = GetFocusPosition();
	if (lines == nullptr)
	{
		m_ActiveLines.resize(0);
		m_fHide = false;
	}
	else
	{
		m_ActiveLines.resize(lines->GetSetBitCount());

		LOG("@%p lines=%d", lines->GetSetBitCount());

		for (size_t idx = 0, active = 0; idx < m_pSource->GetLineCount(); idx++)
		{
			if (lines->GetBit(idx))
			{
				m_ActiveLines[active++] = idx;
			}
		}
		m_fHide = true;
	}
	UpdateView(yFocusPos);
}

void CTraceView::OutputLineToConsole(DWORD nLine)
{
	const LineInfo& line = m_pSource->GetLine(nLine);
	std::string lineA;
	char lineNumA[32];
	lineA = _itoa(nLine, lineNumA, 10);
	lineA += " ";
	lineA.append(line.Content.psz, line.Content.cch);
	m_pApp->POutputView()->OutputLineA(lineA.c_str(), lineA.size());
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceView::SetFocusLine(DWORD nLine)
{
	DeselectAll();

	// translate line to index
	DWORD n = 0;
	if (m_ActiveLines.size() == 0)
	{
		n = nLine;
	}
	else
	{
		auto it = std::lower_bound(m_ActiveLines.begin(), m_ActiveLines.end(), nLine);
		n = (it != m_ActiveLines.end()) ? *it : m_ActiveLines.back();
	}

	ListView_SetItemState(m_ListView.m_hWnd, n, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(m_ListView.m_hWnd, n, FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CTraceView::GetSelectedLines(HANDLE * phData)
{
	HRESULT hr = S_OK;
	CStringRef szLine;
	DWORD nTotal;
	DWORD n;
	int iItem = -1;
	DWORD nFileLine;

	HANDLE hData = NULL;
	char * pszData = NULL;

	HANDLE hCopy = NULL;
	char * pszCopy = NULL;

	DWORD cbCurSize = 0;
	DWORD cbMaxSize = 0;

	*phData = NULL;

	nTotal = ListView_GetSelectedCount(m_ListView.m_hWnd);

	for (n = 0; n < nTotal; n++)
	{
		iItem = ListView_GetNextItem(m_ListView.m_hWnd, iItem, LVIS_SELECTED);

		if (iItem == -1)
		{
			break;
		}

		nFileLine = GetFileLineNum(iItem);

		szLine = m_pSource->GetLine(nFileLine).Content;

		if (cbCurSize + szLine.cch > cbMaxSize)
		{
			cbMaxSize = (cbCurSize + szLine.cch) * 2 + 1;

			hCopy = GlobalAlloc(GMEM_MOVEABLE, cbMaxSize + 1);
			if (hCopy == NULL)
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				goto Cleanup;
			}

			pszCopy = (char*) GlobalLock(hCopy);

			if (pszData != NULL)
			{
				CopyMemory(pszCopy, pszData, cbCurSize);
			}

			GlobalUnlock(hData);
			GlobalFree(hData);

			hData = hCopy;
			pszData = pszCopy;
			hCopy = NULL;
			pszCopy = NULL;
		}

		CopyMemory(pszData + cbCurSize, szLine.psz, szLine.cch);

		cbCurSize += szLine.cch;
	}

	if (pszData)
	{
		pszData[cbCurSize] = '\0';

		GlobalUnlock(hData);

		*phData = hData;
		hData = NULL;
		pszData = NULL;
	}

Cleanup:

	if (pszCopy)
	{
		GlobalUnlock(hCopy);
	}

	if (pszData)
	{
		GlobalUnlock(hData);
	}

	if (hCopy)
	{
		GlobalFree(hCopy);
	}

	if (hData)
	{
		GlobalFree(hData);
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CTraceView::GetSelectedLineNumbers(DWORD * pnStart, DWORD * pnFinish)
{
	HRESULT hr = S_OK;
	DWORD nTotal;
	DWORD n;
	int iItem = -1;

	nTotal = ListView_GetSelectedCount(m_ListView.m_hWnd);

	if (nTotal == 0)
	{
		*pnStart = *pnFinish = 0;
	}
	else
	{
		iItem = ListView_GetNextItem(m_ListView.m_hWnd, iItem, LVIS_SELECTED);
		*pnStart = GetFileLineNum(iItem);

		for (n = 1; n < nTotal; n++)
		{
			iItem = ListView_GetNextItem(m_ListView.m_hWnd, iItem, LVIS_SELECTED);

			if (iItem == -1)
			{
				break;
			}

			*pnFinish = GetFileLineNum(iItem);
		}
	}

	//Cleanup:

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
int CTraceView::GetFocusPosition()
{
	DWORD nTop;
	DWORD nFocus = 0;
	RECT rcTop;
	RECT rcFocus;

	ZeroMemory(&rcFocus, sizeof(rcFocus));

	//
	// to keep focus item on the same place, record topindex and then restore topindex base on new position
	//
	nTop = ListView_GetTopIndex(m_ListView.m_hWnd);

	ListView_GetItemRect(m_ListView.m_hWnd, nTop, &rcTop, LVIR_BOUNDS);

	//
	// find closest item in the new 
	//
	if (m_nFocusLine != -1)
	{
		if (m_fHide)
		{
			nFocus = FindIndex(m_ActiveLines, m_nFocusLine);
		}
		else
		{
			nFocus = m_nFocusLine;
		}

		ListView_GetItemRect(m_ListView.m_hWnd, nFocus, &rcFocus, LVIR_BOUNDS);
	}

	return rcFocus.top - rcTop.top;
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceView::UpdateView(int yFocusPos)
{
	if (m_ListView == nullptr)
		return;

	// resize cache just in case
	m_LineCache->Resize(m_pSource->GetLineCount());

	// update view    
	if (m_fHide)
	{
		ListView_SetItemCount(m_ListView.m_hWnd, m_ActiveLines.size());
	}
	else
	{
		ListView_SetItemCount(m_ListView.m_hWnd, m_pSource->GetLineCount());
	}

	DeselectAll();

	// find closest item in the new 
	if (m_nFocusLine != -1)
	{
		DWORD nTop;
		DWORD nFocus;
		RECT rcFocus;
		RECT rcTop;

		if (m_fHide)
		{
			nFocus = FindIndex(m_ActiveLines, m_nFocusLine);
		}
		else
		{
			nFocus = m_nFocusLine;
		}

		ListView_SetItemState(m_ListView.m_hWnd, nFocus, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

		// get absolute position of the top element
		nTop = ListView_GetTopIndex(m_ListView.m_hWnd);

		ListView_GetItemRect(m_ListView.m_hWnd, nTop, &rcTop, LVIR_BOUNDS);

		// get absolute position of item
		ListView_GetItemRect(m_ListView.m_hWnd, nFocus, &rcFocus, LVIR_BOUNDS);

		//        ListView_EnsureVisible(m_ListView.m_hWnd, nItem, FALSE);
		ListView_Scroll(m_ListView.m_hWnd, 0, rcFocus.top - rcTop.top - yFocusPos);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CTraceView::InsertColumn(DWORD nIndex, DWORD nWidth, LPCWSTR pszText)
{
	LVCOLUMN col;
	HRESULT hr = S_OK;
	ZeroMemory(&col, sizeof(col));
	col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	col.fmt = LVCFMT_LEFT;
	col.pszText = (LPWSTR) pszText;
	col.cx = nWidth;

	if (ListView_InsertColumn(m_ListView.m_hWnd, nIndex, &col) == -1)
	{
		ATLASSERT(false);
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}

Cleanup:

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
void CTraceView::DeselectAll()
{
	int iItem = -1;
	DWORD nTotal;
	DWORD n;

	nTotal = ListView_GetSelectedCount(m_ListView.m_hWnd);

	for (n = 0; n < nTotal; n++)
	{
		iItem = ListView_GetNextItem(m_ListView.m_hWnd, iItem, LVIS_SELECTED);

		if (iItem == -1)
		{
			break;
		}

		ListView_SetItemState(m_ListView.m_hWnd, iItem, 0, LVIS_FOCUSED | LVIS_SELECTED);
	}
}
