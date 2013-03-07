#include "stdafx.h"

#include "traceapp.h"
#include "traceview.h"
#include "handlers.h"

HRESULT CLoadHandler::HandleCommand(CCommand * pCmd, 
                                        CCommandProcessorOutput * pOutput,
                                        CCommandList & ExpCommands)
{
    HRESULT hr = S_OK;
    FILE * pFile = NULL;
    CAtlStringW szFile;
    char szBuf[256];
    long cch;
    char * p;

    if(!pCmd->Match(L"ld"))
    {
        return S_FALSE;
    }
    
    szFile = pCmd->GetString(L"%");

    pFile = _wfopen(szFile, L"r");
    if(pFile == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Cleanup;
    }

    for(;!feof(pFile);)
    {
        p = fgets(szBuf, sizeof(szBuf), pFile);
        if(p == NULL)
        {
            break;
        }

        // remore /r/n
        cch = strlen(szBuf);
        if(cch > 1)
        {
            cch = (szBuf[cch - 1] == '\n') ? cch - 1 : cch;
        }

        if(cch > 1)
        {
            cch = (szBuf[cch - 1] == '\r') ? cch - 1 : cch;
        }

        CAtlStringW szLine(szBuf, cch);

        {
            CCommandList lst;
            CCommandParser prs;

            prs.Parse(szLine, lst);

            for(;lst.GetCount();)
            {
                ExpCommands.AddHead(lst.RemoveTail());
            }
        }
    }

Cleanup:

    if(pFile)
    {
        fclose(pFile);
    }
    
    return hr;
}

HRESULT CHelpHandler::HandleCommand(CCommand * pCmd, 
                                    CCommandProcessorOutput * pOutput,
                                    CCommandList & ExpCommands)
{
    if(!pCmd->Match(L"help"))
    {
        return S_FALSE;
    }
    
    pOutput->OutputLine(L"af <expression> [<-c color>] [-o <comment>] [-g <group>] - add filter");
    pOutput->OutputLine(L"au <expression> - add sub system filter. expression format is [all:level] [[-][+]subsys:level]");
    pOutput->OutputLine(L"la name command - add alias for the command");
    pOutput->OutputLine(L"ll - list aliases");
    pOutput->OutputLine(L"lr name - remove alias");
    pOutput->OutputLine(L"hide - show filtered only");
    pOutput->OutputLine(L"show - show all traces");
    pOutput->OutputLine(L"ld <file> - load commands from the file");
    pOutput->OutputLine(L"help - this help");

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CShowFilteredHandler::HandleCommand(CCommand * pCmd, 
                            CCommandProcessorOutput * pOutput,
                            CCommandList & ExpCommands)
{
    if(!pCmd->Match(L"show"))
    {
        return S_FALSE;
    }

    m_pApp->PTraceView()->OnShowFiltered(pCmd->GetBool(L"%", FALSE));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CAddFilterHandler::HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands)
{
    HRESULT hr = S_OK;
    CExpressionTraceFilter * pChild;

    if(!pCmd->Match(L"af"))
    {
        return S_FALSE;
    }

    pChild = new CExpressionTraceFilter(m_pApp->PExtInfoOffsets()->m_cbColorOffs);

    pChild->SetExpression(pCmd->GetString(L"%"));
    pChild->SetComment(pCmd->GetString(L"o"));
    pChild->SetColor(pCmd->GetByte(L"c", 0));

    pChild->SetEnable(TRUE);

    // tell child filter to cmpute itself
    pChild->ProcessFile(m_pApp->PFile(), TRUE);
    
    // add child filter to parent
    m_pApp->PUserFilter()->AddChild(pChild);

    // enable root user filter
    if(!m_pApp->PUserFilter()->GetEnable())
    {
        m_pApp->PUserFilter()->SetEnable(TRUE);
    }

    m_pApp->OnFilterUpdated(TRUE);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CFilterRangeHandler::HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands)
{
    HRESULT hr = S_OK;
    DWORD nStart, nFinish;

    if(!pCmd->Match(L"fr"))
    {
        return S_FALSE;
    }

    if(m_pApp->PRangeFilter()->GetEnable())
    {
        m_pApp->PRangeFilter()->SetEnable(FALSE);
    }
    else
    {
        m_pApp->PRangeFilter()->SetEnable(TRUE);

        IFC(m_pApp->PTraceView()->GetSelectedLineNumbers(&nStart, &nFinish));

        m_pApp->PRangeFilter()->SetRange(nStart, nFinish);
    }

    IFC(m_pApp->PRangeFilter()->ProcessFile(m_pApp->PFile(), FALSE));

    m_pApp->OnFilterUpdated(FALSE);

Cleanup:

    return hr;    
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CSetSubSysFilterHandler::HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands)
{
    HRESULT hr;
    CAtlStringA szTextA;

    if(!pCmd->Match(L"au"))
    {
        return S_FALSE;
    }
    
    szTextA = pCmd->GetString(L"%");

    m_pApp->PSubSysFilter()->SetEnable(TRUE);
    
    IFC(m_pApp->PSubSysFilter()->ParseSubSysText(szTextA));

    IFC(m_pApp->PSubSysFilter()->ProcessFile(m_pApp->PFile(), FALSE));

    m_pApp->OnFilterUpdated(FALSE);

Cleanup:

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
CAliasHandler::CAliasHandler(CTraceApp * pApp)
    : CCommandHandler(pApp)
{
    m_pApp->PPersist()->RegisterHandler(this);
}

CAliasHandler::~CAliasHandler()
{
    m_pApp->PPersist()->UnregisterHandler(this);
}

HRESULT CAliasHandler::HandleCommand(CCommand * pCmd, 
                            CCommandProcessorOutput * pOutput,
                            CCommandList & ExpCommands)
{
    if(pCmd->Match(L"la"))
    {
        CAtlString szAlias;
        CAtlString szCmd;
        
        szAlias = pCmd->GetString(L"%");
        szCmd = pCmd->GetString(L"%1");
        
        m_Commands.SetAt(szAlias, szCmd);
    }
    else if(pCmd->Match(L"ll"))
    {
        POSITION pos;
        _CommandMap::CPair * pPair;
        CAtlString szOut;
        
        for(pos = m_Commands.GetStartPosition(); pos; )
        {
            pPair = m_Commands.GetNext( pos );

            szOut.Empty();
            szOut.Append(pPair->m_key);
            szOut.Append(L" =");
            szOut.Append(pPair->m_value);

            pOutput->OutputLine(szOut);
        }
    }
    else if(pCmd->Match(L"lr"))
    {
    }
    else
    {
        CAtlString szCmd;

        if(!m_Commands.Lookup(pCmd->GetCmd(), szCmd))
        {
            return S_FALSE;
        }

        m_pApp->PProcessor()->ExecuteCommand(szCmd, pOutput);
        
        return S_FALSE;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CAliasHandler::SaveConfig(HKEY hKey)
{
    POSITION pos;
    _CommandMap::CPair * pPair;
    CByteArray arReg;
    DWORD dw;
    
    for(pos = m_Commands.GetStartPosition(); pos; )
    {
        pPair = m_Commands.GetNext( pos );

        dw = pPair->m_key.GetLength() * sizeof(WCHAR);
        arReg.Append((BYTE*)&dw, sizeof(dw));
        arReg.Append((BYTE*)(LPCWSTR)pPair->m_key, dw);

        dw = pPair->m_value.GetLength() * sizeof(WCHAR);
        arReg.Append((BYTE*)&dw, sizeof(dw));
        arReg.Append((BYTE*)(LPCWSTR)pPair->m_value, dw);
    }

    RegSetValueEx(hKey, L"Alias", 0, REG_BINARY, arReg.GetData(), arReg.GetCount());

    return S_OK;
}

HRESULT CAliasHandler::LoadConfig(HKEY hKey)
{
    LRESULT lResult;
    DWORD cb;
    CByteArray arReg;
    
    lResult = RegQueryValueEx(hKey, L"Alias", NULL, NULL, NULL, &cb);
    
    if(lResult != ERROR_SUCCESS || cb < 1024*1024)
    {
        goto Cleanup;
    }

    if(FAILED(arReg.SetCount(cb)))
    {
        goto Cleanup;
    }
    
    lResult = RegQueryValueEx(hKey, L"Alias", NULL, NULL, arReg.GetData(), &cb);

    if(lResult != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
/*
    for(;;)
    {
        arReg.Get
    }
*/

Cleanup:

    return S_OK;
}

