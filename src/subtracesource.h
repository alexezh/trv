#pragma once
#include "file.h"
#include "bitset.h"

class SubTraceSource : CTraceSource
{
public:
	SubTraceSource(const std::shared_ptr<CTraceSource>& src, const CBitSet& set);

	// Returns the current line count. 
	// The count can change as we add more data at the end or in the beginning
	DWORD GetLineCount() override;

	const LineInfoDesc& GetDesc() override;
	const LineInfo& GetLine(DWORD nIndex) override;
	bool SetTraceFormat(const char * psz) override;

	void SetScope(const std::shared_ptr<CBitSet>& scope) override
	{
		m_Scope = scope;
	}
	const std::shared_ptr<CBitSet>& GetScope() override
	{
		return m_Scope;
	}

	// updates view with changes (if any)
	HRESULT Refresh() override;

	void SetHandler(CTraceViewNotificationHandler * pHandler) override;

private:
	std::shared_ptr<CTraceSource> m_Source;
	std::vector<DWORD> m_Lines;
	std::shared_ptr<CBitSet> m_Scope;
};
