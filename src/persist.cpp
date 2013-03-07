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
#include "persist.h"

///////////////////////////////////////////////////////////////////////////////
//
#define TRV_KEY L"Software\\Trv"

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CPersistComponentContainer::Init()
{
    LONG lResult;

    lResult = RegCreateKey(HKEY_CURRENT_USER, TRV_KEY, &m_hKey);

    if(lResult != ERROR_SUCCESS)
    {
        return E_FAIL;
    }
    else
    {
        return S_OK;
    }
}

void CPersistComponentContainer::RegisterHandler(CPersistHandler * pHandler)
{
    m_Handlers.Add(pHandler);

    pHandler->LoadConfig(m_hKey);
}

void CPersistComponentContainer::UnregisterHandler(CPersistHandler * pHandler)
{
    size_t i;

    for(i=0; i<m_Handlers.GetCount(); i++)
    {
        if(m_Handlers[i] == pHandler)
        {
            m_Handlers.RemoveAt( i );
        }
    }
}

void CPersistComponentContainer::SaveAll()
{
    size_t i;

    for(i=0; i<m_Handlers.GetCount(); i++)
    {
        m_Handlers[i]->SaveConfig(m_hKey);
    }
}

void CPersistComponentContainer::LoadAll()
{
    size_t i;

    for(i=0; i<m_Handlers.GetCount(); i++)
    {
        m_Handlers[i]->LoadConfig(m_hKey);
    }
}

