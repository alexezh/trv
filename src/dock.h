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

namespace Dock {

///////////////////////////////////////////////////////////////////////////////
// 
class Element
{
public:
	virtual void Resize(const RECT& rc) = 0;
};

class Window : public Element
{
public:
	Window(CWindow* p)
		: m_pWindow(p)
	{
	}

	void Resize(const RECT& rc)
	{
        m_pWindow->SetWindowPos(NULL, 
                        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
                        SWP_NOZORDER);
	}

private:
    // optional window associated with
    CWindow * m_pWindow;
};

class Table : public Element
{
public:
	enum LayoutType
	{
		Horizontal,
		Vertical,
	};
	
	Table(LayoutType type)
		: m_LayoutType(type)
	{
	}

	enum SizeType
	{
		SizeMax,
		SizePixel,
		SizePercentage,
	};

	void Resize(const RECT& rc);

	size_t AddColumn(SizeType t, double sz, Element* pTarget)
	{
		Column c;
		c.Type = t;
		c.Size = sz;
		c.Taget = pTarget;
		m_Columns.push_back(c);
		return m_Columns.size()-1;
	}

	void ResizeColumn(size_t id, double sz)
	{
		m_Columns[id].Size = sz;
	}

private:
	struct Column
	{
		SizeType Type;
		double Size;
		long RealSize;
		Element * Taget;
	};

	LayoutType m_LayoutType;
	RECT m_Rect;
	std::vector<Column> m_Columns;
};

///////////////////////////////////////////////////////////////////////////////
// 
class HostWindow : public CWindowImpl<HostWindow>
{
public:
    HostWindow();
    
    void UpdateDock();
	void SetRootElement(Element* p)
	{
		assert(m_pRoot == nullptr);
		m_pRoot = p;
	}

private:    
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    BEGIN_MSG_MAP(CDock)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
    END_MSG_MAP()

private:    
    Element* m_pRoot;

    LONG m_nHeight;
    LONG m_nWidth;
};

} // Dock
