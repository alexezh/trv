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
#include "viewlinecache.h"

void ViewLineCache::SetLine(DWORD idx, ViewLine&& line)
{
	m_Cache.Set(idx, std::unique_ptr<ViewLine>(new ViewLine(std::move(line))));
}

void ViewLineCache::ClearRange(DWORD idxStart, DWORD idxEnd)
{

}

const ViewLine& ViewLineCache::GetLine(DWORD idx)
{
	auto& line = m_Cache.GetAt(idx);
	if (line == nullptr)
	{
		static ViewLine emptyLine;
		return emptyLine;
	}

	return *line;
}

void ViewLineCache::RegisterChangeListener(const ChangeHandler& handler)
{
	m_OnCachedChanged = handler;
}
