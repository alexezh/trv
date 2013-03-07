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
#include "strstr.h"

///////////////////////////////////////////////////////////////////////////////
//
__inline int __NoCaseStrCmp(const CHAR * psz1, const CHAR * psz2, long cch)
{
    CHAR c1, c2;
    
    for (; cch>0; cch--)
    {
        // name can only be US-ASCII 0-127
        c1 = (char)*psz1++;
        c2 = (char)*psz2++;
        
        if( c1 >= 'a' && c1 <= 'z' )
        {
            c1 -= ('a' - 'A');
        }
        
        if( c2 >= 'a' && c2 <= 'z' )
        {
            c2 -= ('a' - 'A');
        }

        if( c1 != c2 )
        {
            return 1;
        }
    }

    return 0;
}

__inline int __CaseStrCmp(const CHAR * psz1, const CHAR * psz2, long cch)
{
    return memcmp(psz1, psz2, cch);
}

__inline CHAR __ToUpper(CHAR c)
{
    if( c >= 'a' && c <= 'z' )
    {
        c -= ('a' - 'A');
    }

    return c;
}

CStrStr::CStrStr()
    : m_fIgnoreCase(TRUE)
{
}

void CStrStr::BuildBc(LPCSTR pszExpr)
{ 
   int i;

	m_Expr = pszExpr;
	int cchExpr = m_Expr.GetLength();

    for (i = 0; i < MAX_BC_LEN; i++)
    {
        m_bc[i] = cchExpr + 1;
    }

    
    if(m_fIgnoreCase)
    {
        for (i = 0; i < cchExpr; i++)
        {
            m_bc[(BYTE)__ToUpper(pszExpr[i])] = cchExpr - i; 
        }
    }
    else
    {
        for (i = 0; i < cchExpr; i++)
        {
            m_bc[(BYTE)pszExpr[i]] = cchExpr - i; 
        }
    }
} 

LPCSTR CStrStr::Search(LPCSTR pszBuf, int cchBuf)
{ 
    int j; 
    int cchExpr = m_Expr.GetLength();
    const CHAR * pszExpr = m_Expr;

    if(m_fIgnoreCase)
    {
        // Searching  
        j = 0; 
        while (j <= cchBuf - cchExpr)
        { 
            if (__NoCaseStrCmp(pszExpr, pszBuf + j, cchExpr) == 0)
            {
                return pszBuf + j; 
            }
        
            j += m_bc[(BYTE)__ToUpper(pszBuf[j + cchExpr])];               /* shift */ 
        } 
    }
    else
    {
        // Searching  
        j = 0; 
        while (j <= cchBuf - cchExpr)
        { 
            if (__CaseStrCmp(pszExpr, pszBuf + j, cchExpr) == 0)
            {
                return pszBuf + j; 
            }
        
            j += m_bc[(BYTE)pszBuf[j + cchExpr]];               /* shift */ 
        } 
    }

    return NULL;
} 


