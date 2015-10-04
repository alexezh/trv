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
#include "js/apphost.h"

ViewLineCache::ViewLineCache(IDispatchQueue* uiQueue, Js::IAppHost* host)
{
	m_UiQueue = uiQueue;
	m_Host = host;
}

void ViewLineCache::SetLine(DWORD idx, ViewLine&& line)
{
	SetLine(idx, std::unique_ptr<ViewLine>(new ViewLine(std::move(line))));
}

void ViewLineCache::SetLine(DWORD idx, std::unique_ptr<ViewLine>&& line)
{
	m_Cache.Set(idx, std::move(line));
	m_UiQueue->Post([this, idx]()
	{
		if (!m_OnLineAvailable)
			return;

		m_OnLineAvailable(idx);
	});
}

std::pair<bool, const ViewLine&> ViewLineCache::GetLine(DWORD idx)
{
	auto& line = m_Cache.GetAt(idx);
	if (line == nullptr)
	{
		m_RequestedLines.push_back(idx);
		m_Host->RequestViewLine();

		static ViewLine emptyLine;
		return std::make_pair(false, emptyLine);
	}

	return std::make_pair(true, *line);
}

void ViewLineCache::RegisterLineAvailableListener(const LiveAvailableHandler& handler)
{
	m_OnLineAvailable = handler;
}

