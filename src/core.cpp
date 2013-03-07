#include "stdafx.h"

#include "traceapp.h"
#include "core.h"

#include "stringreader.h"

///////////////////////////////////////////////////////////////////////////////
//
CTraceFilter::CTraceFilter(OP Op, DWORD cbColorOffset, BYTE bColor)
    : m_fEnable(FALSE)
    , m_dwViewState(0)
    , m_cbColorOffset(cbColorOffset)
    , m_bColor(bColor)
{
    m_Op = Op;
    m_dwViewState = TVIS_EXPANDED;
}

CTraceFilter::~CTraceFilter()
{
    RemoveAllChildren();
}

void CTraceFilter::SetViewState(DWORD dwVal)
{
    m_dwViewState = (dwVal & (~TVIS_BOLD));
}

DWORD CTraceFilter::GetViewState()
{
    return m_dwViewState | ((m_fEnable) ? TVIS_BOLD : 0);
}

HRESULT CTraceFilter::AddChild( CTraceFilter * pFilter)
{
    m_Child.Add(pFilter);
    return S_OK;
}

BOOL CTraceFilter::RemoveChild(CTraceFilter * pFilter)
{
    CTraceFilter * pChild;
    DWORD n;
    BOOL fRet = FALSE;

    for(n=0; n<m_Child.GetCount(); n++)
    {
        pChild = m_Child.GetAt(n);
        if(pChild == pFilter)
        {
            m_Child.RemoveAt(n);
            delete pChild;
            fRet = TRUE;
            break;
        }

        fRet = pChild->RemoveChild(pFilter);
        if(fRet)
        {
            break;
        }
    }
    
    return fRet;
}

void CTraceFilter::RemoveAllChildren()
{
    CTraceFilter * pChild;
    DWORD n;

    for(n=0; n<m_Child.GetCount(); n++)
    {
        pChild = m_Child.GetAt(n);
        delete pChild;
    }
    
    m_Child.RemoveAll();
}

DWORD CTraceFilter::GetChildCount()
{
    return m_Child.GetCount();
}

CTraceFilter * CTraceFilter::GetChild(DWORD dwIndex)
{
    if(dwIndex >= m_Child.GetCount())
    {
        return NULL;
    }

    return m_Child.GetAt(dwIndex);
}

void CTraceFilter::ComputeSelf(CTraceExtendedInfo * pInfo)
{
    DWORD n;

    if(!m_fEnable)
    {
        return;
    }

    if(m_Op == OP_OR || m_Op == OP_AND)
    {
        CTraceFilter * pChild;

        m_Set.Clear((m_Op == OP_AND));

        for(n=0; n<m_Child.GetCount();n++)
        {
            pChild = m_Child.GetAt(n);
            pChild->Compute(pInfo, m_Set, m_Op);
        }
    }

    // go through selected bits and set color
    if(m_bColor != 0)
    {
        for(n=0; n<m_Set.GetTotalBitCount(); n++)
        {
            if(m_Set.GetBit(n))
            {
                pInfo->CompareAndSetByteField(n, m_cbColorOffset, 0, m_bColor);                
            }
        }
    }
}

DWORD s_BitCount[256] = 
{
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
};

void CTraceFilter::Compute(CTraceExtendedInfo * pInfo, CBitSet & Set, OP ParentOp)
{
    DWORD n;
    DWORD dwBits;
    DWORD nSet = 0;

    if(!m_fEnable)
    {
        return;
    }

    ComputeSelf(pInfo);
    
    DWORD * pTo = Set.GetBuf();
    DWORD * pFrom = m_Set.GetBuf();

    // combine parent set with leaf child set
    if(ParentOp == OP_OR)
    {
        for(n = 0; n<Set.GetBufSize(); n++)
        {
            dwBits = (*pTo) | (*pFrom);
            nSet += s_BitCount[dwBits & 0x000000ff] + 
                    s_BitCount[(dwBits >> 8) & 0x000000ff] +
                    s_BitCount[(dwBits >> 16) & 0x000000ff] +
                    s_BitCount[(dwBits >> 24) & 0x000000ff];
            *pTo = dwBits;
            pTo++;
            pFrom++;
        }
    }
    else
    {
        for(n = 0; n<Set.GetBufSize(); n++)
        {
            dwBits = (*pTo) & (*pFrom);
            nSet += s_BitCount[dwBits & 0x000000ff] + 
                    s_BitCount[(dwBits >> 8) & 0x000000ff] +
                    s_BitCount[(dwBits >> 16) & 0x000000ff] +
                    s_BitCount[(dwBits >> 24) & 0x000000ff];
            *pTo = dwBits;
            pTo++;
            pFrom++;
        }
    }

    Set.SetSetBitCount(nSet);
}


///////////////////////////////////////////////////////////////////////////////
//
CRootTraceFilter::CRootTraceFilter()
    : CLogicalTraceFilter(OP_AND, 0)
{
}

void CRootTraceFilter::ComputeSelf(CTraceExtendedInfo * pExtInfo)
{
    DWORD n;
    BYTE bColor = 0;
    
    // cleanup colors
    for(n=0; n<m_Set.GetTotalBitCount(); n++)
    {
        pExtInfo->SetField(n, m_cbColorOffset, &bColor, sizeof(bColor));                
    }
    
    CTraceFilter::ComputeSelf(pExtInfo);
}

void CRootTraceFilter::RemoveFilter(CTraceFilter * pFilter)
{
    CTraceFilter * pChild;
    DWORD n;
    BOOL fRet = FALSE;

    for(n=0; n<m_Child.GetCount(); n++)
    {
        pChild = m_Child.GetAt(n);

        fRet = pChild->RemoveChild(pFilter);
        if(fRet)
        {
            break;
        }
    }
}



