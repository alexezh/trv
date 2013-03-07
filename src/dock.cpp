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

#include "dock.h"

namespace Dock {

void Table::Resize(const RECT& rc)
{
	m_Rect = rc;
	long totalSize = (m_LayoutType == Vertical) ? m_Rect.bottom - m_Rect.top : m_Rect.right - m_Rect.left;

	// first go through rows and substract fixed
	for(auto it = m_Columns.begin(); it != m_Columns.end(); ++it)
	{
		if(it->Type == SizePixel)
		{
			it->RealSize = (long)it->Size;
			totalSize -= it->RealSize;
		}
	}

	// now go through dynamic
	long combinedPercentage = 0;
	for(auto it = m_Columns.begin(); it != m_Columns.end(); ++it)
	{
		if(it->Type == SizePercentage)
		{
			it->RealSize = (long)(totalSize * it->Size);
			combinedPercentage += it->RealSize;
		}
	}

	// if there is a fill column use the rest
	for(auto it = m_Columns.begin(); it != m_Columns.end(); ++it)
	{
		if(it->Type == SizeMax)
		{
			it->RealSize = totalSize - combinedPercentage;
			break;
		}
	}

	// update all child nodes
	long pos = 0;
	for(auto it = m_Columns.begin(); it != m_Columns.end(); ++it)
	{
		RECT child;
		if(m_LayoutType == Vertical)
		{
			child.top = pos;
			child.bottom = pos + it->RealSize;
			child.left = m_Rect.left;
			child.right = m_Rect.right;
		}
		else
		{
			child.left = pos;
			child.right = pos + it->RealSize;
			child.top = m_Rect.left;
			child.bottom = m_Rect.right;
		}

		pos += it->RealSize;
		if(it->Taget)
		{
			it->Taget->Resize(child);
		}
	}

}

///////////////////////////////////////////////////////////////////////////////
//
HostWindow::HostWindow()
    : m_pRoot(nullptr)
{
}


void HostWindow::UpdateDock()
{
    RECT rect;
    GetClientRect(&rect);
    m_pRoot->Resize(rect);
}

LRESULT HostWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = TRUE;
    
    return 0;
}

LRESULT HostWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rect;
    rect.top = 0;
	rect.left = 0;
	rect.right = LOWORD(lParam);
	rect.bottom = HIWORD(lParam);

	if(m_pRoot)
	{
		m_pRoot->Resize(rect);
	}

    bHandled = TRUE;
    
    return 0;
}

} // Dock
