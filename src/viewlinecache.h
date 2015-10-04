#pragma once

#include <array>
#include "blockarray.h"

namespace v8 {
class Utf8Value;
}

namespace Js {
class IAppHost;
}

class ViewLine
{
public:
	ViewLine()
	{

	}
	ViewLine(ViewLine&& other)
		: m_LineIndex(other.m_LineIndex)
		, m_Time(other.m_Time)
		, m_ThreadId(other.m_ThreadId)
		, m_Msg(other.m_Msg)
		, m_User(other.m_User)
		, m_Data(std::move(other.m_Data))
	{
	}

	ViewLine& operator=(ViewLine&& other)
	{
		if (this == &other)
			return *this;

		m_LineIndex = other.m_LineIndex;
		m_Time = other.m_Time;
		m_ThreadId = other.m_ThreadId;
		m_Msg = other.m_Msg;
		m_User = other.m_User;
		m_Data = std::move(other.m_Data);
			
		return *this;
	}
	void SetLineIndex(DWORD line);
	DWORD GetLineIndex() const
	{
		return m_LineIndex;
	}
	void SetTime(v8::Utf8Value& val);
	CStringRef GetTime() const
	{
		return m_Time;
	}
	void SetMsg(v8::Utf8Value& val);
	CStringRef GetMsg() const
	{
		return m_Msg;
	}
	void SetThreadId(v8::Utf8Value& val);
	CStringRef GetThreadId() const
	{
		return m_ThreadId;
	}
	void SetUser1(v8::Utf8Value& val);
	CStringRef GetUser(size_t idx) const
	{
		return m_User[idx];
	}

private:
	DWORD m_LineIndex;
	CStringRef m_Time;
	CStringRef m_ThreadId;
	CStringRef m_Msg;
	std::array<CStringRef, 4> m_User;
	std::vector<char> m_Data;
};

class ViewLineCache
{
public:
	using LiveAvailableHandler = std::function<void(DWORD idx)>;
	using LineRequestedHandler = std::function<void(DWORD idx)>;

	ViewLineCache(Js::IAppHost* host);

	std::pair<bool, const ViewLine&> GetLine(DWORD idx);

	bool HaveLineRequests()
	{
		std::lock_guard<std::mutex> guard(m_Lock);
		return m_RequestedLines.size() != 0;
	}

	static const DWORD NoLine = static_cast<DWORD>(-1);
	DWORD GetNextRequestedLine()
	{
		std::lock_guard<std::mutex> guard(m_Lock);
		if (m_RequestedLines.size() == 0)
			return NoLine;

		DWORD idx = m_RequestedLines.back();
		m_RequestedLines.pop_back();
		return idx;
	}
	void SetLine(DWORD idx, ViewLine&& line);

	void StartGeneration();

	void RegisterLineAvailableListener(const LiveAvailableHandler& handler);

private:
	std::mutex m_Lock;

	std::vector<DWORD> m_RequestedLines;
	Js::IAppHost* m_Host;

	CSparseBlockArray<std::unique_ptr<ViewLine>, 1024 * 32> m_Cache;
	LiveAvailableHandler m_OnLineAvailable;
	LineRequestedHandler m_OnLineRequested;
};
