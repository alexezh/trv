#ifndef __HANDLERS__
#define __HANDLERS__

///////////////////////////////////////////////////////////////////////////////
// 
class CLoadHandler
    : public CCommandHandler
{
public:    
    CLoadHandler(CTraceApp * pApp)
        : CCommandHandler(pApp)
    {;
    }

    HRESULT HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands);
};

///////////////////////////////////////////////////////////////////////////////
// 
class CHelpHandler
    : public CCommandHandler
{
public:    
    CHelpHandler(CTraceApp * pApp)
        : CCommandHandler(pApp)
    {;
    }

    HRESULT HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands);
};

///////////////////////////////////////////////////////////////////////////////
// 
class CAddFilterHandler
    : public CCommandHandler
{
public:
    CAddFilterHandler(CTraceApp * pApp)
        : CCommandHandler(pApp)
    {;
    }

    HRESULT HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands);
};

///////////////////////////////////////////////////////////////////////////////
// 
class CSetSubSysFilterHandler : public CCommandHandler
{
public:
    CSetSubSysFilterHandler(CTraceApp * pApp)
        : CCommandHandler(pApp)
    {;
    }

    HRESULT HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands);
};

///////////////////////////////////////////////////////////////////////////////
// 
class CShowFilteredHandler : public CCommandHandler
{
public:
    CShowFilteredHandler(CTraceApp * pApp)
        : CCommandHandler(pApp)
    {;
    }

    HRESULT HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands);
};

///////////////////////////////////////////////////////////////////////////////
// 
class CFilterRangeHandler : public CCommandHandler
{
public:
    CFilterRangeHandler(CTraceApp * pApp)
        : CCommandHandler(pApp)
    {;
    }

    HRESULT HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands);
};

///////////////////////////////////////////////////////////////////////////////
// 
class CAliasHandler 
        : public CCommandHandler
        , public CPersistHandler
{
public:
    CAliasHandler(CTraceApp * pApp);
    ~CAliasHandler();

    HRESULT HandleCommand(CCommand * pCmd, 
                                CCommandProcessorOutput * pOutput,
                                CCommandList & ExpCommands);

    // CPersistHandler
    HRESULT SaveConfig(HKEY hKey);
    HRESULT LoadConfig(HKEY hKey);

private:

    typedef CAtlMap<CAtlString, CAtlString> _CommandMap;
    _CommandMap m_Commands;
};


#endif

