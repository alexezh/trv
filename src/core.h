#pragma once

#include "resource.h"

#include "strstr.h"
#include "file.h"
#include "stringreader.h"
#include "bitset.h"

///////////////////////////////////////////////////////////////////////////////
//
class CTraceExtendedInfo;

///////////////////////////////////////////////////////////////////////////////
// stores tree of filters
// OR
//    A
//    AND
//        B
//        C
// during compute operation each filter can update color field in the CTraceExtendedInfo
class CTraceFilter
{
public:
    enum OP
    {
        OP_OR,
        OP_AND,
        OP_EXPR,
        OP_THREAD,
        OP_SUBSYS,
        OP_RANGE,
    };

    CTraceFilter(OP Op, DWORD cbColorOffset, BYTE bColor = 0);
    virtual ~CTraceFilter();

    // return text to output
    virtual void GetText(CAtlStringW & szText) = 0;

    // builds bitmask with selected lines
    virtual HRESULT ProcessFile(CTraceFile * pFile, BOOL fRecursive) = 0;
    
    // get/set operation
    void SetOperation(OP Op) { m_Op = Op; }
    OP GetOperation() { return m_Op; }

    // enable/disable filter
    void SetEnable(BOOL fEnable) { m_fEnable = fEnable; }
    BOOL GetEnable() { return m_fEnable; }

    // recompute 
    virtual void ComputeSelf(CTraceExtendedInfo * pExtInfo);
    CBitSet & GetCurrentSet() { return m_Set; }

    // add/remove
    HRESULT AddChild( CTraceFilter * pFilter);
    void RemoveAllChildren();
    BOOL RemoveChild(CTraceFilter * pFilter);

    // enumarate support
    DWORD GetChildCount();
    CTraceFilter * GetChild(DWORD dwIndex);

    // view state
    void SetViewState(DWORD dwVal);
    DWORD GetViewState();

    // color
    void SetColor(BYTE bColor) { m_bColor = bColor; }
    BYTE GetColor() { return m_bColor; }

protected:

    void Compute(CTraceExtendedInfo * pExtInfo, CBitSet & Set, OP ParentOp);

   
protected:
    BOOL m_fEnable;

    OP m_Op;
    CBitSet m_Set;
    CAtlArray<CTraceFilter*> m_Child;
    DWORD m_dwViewState;

    // offset for updating color during Compute operation
    DWORD m_cbColorOffset;

    // color for this filter. 0 - undefined
    BYTE m_bColor;
};

///////////////////////////////////////////////////////////////////////////////
//
class CLogicalTraceFilter : public CTraceFilter
{
public:    
    CLogicalTraceFilter(OP Op, DWORD cbColorOffset, BYTE bColor = 0);
    
    void GetText(CAtlStringW & szText);
    
    HRESULT ProcessFile(CTraceFile * pFile, BOOL fRecursive);
};

///////////////////////////////////////////////////////////////////////////////
//
class CRootTraceFilter : public CLogicalTraceFilter
{
public:
    CRootTraceFilter();
    
    void ComputeSelf(CTraceExtendedInfo * pExtInfo);

    // removes filters except for top level
    void RemoveFilter(CTraceFilter * pFilter);
};

