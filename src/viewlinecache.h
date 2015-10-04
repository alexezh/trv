#pragma once

#include <array>
#include "blockarray.h"

namespace v8 {
class Utf8Value;
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
	using ChangeHandler = std::function<void(DWORD idxStart, DWORD idxEnd)>;

	void StartGeneration();

	// Set can be called on any thread
	// Clear and Get should be called on a single thread or at least
	// code should not access ViewLine after ClearRange call
	void SetLine(DWORD idx, ViewLine&& line);
	void ClearRange(DWORD idxStart, DWORD idxEnd);
	const ViewLine& GetLine(DWORD idx);

	void RegisterChangeListener(const ChangeHandler& handler);

private:
	std::mutex m_Lock;

	// 
	size_t m_IdxGenStart;
	size_t m_IdxGenEnd;

	//	std::lock_guard<std::mutex> guard(Lock);
	CSparseBlockArray<std::unique_ptr<ViewLine>, 1024 * 32> m_Cache;
	ChangeHandler m_OnCachedChanged;
};
