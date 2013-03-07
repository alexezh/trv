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
#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// The Quick Search algorithm uses only the bad-character shift table (see chapter Boyer-Moore algorithm). 
// After an attempt where the window is positioned on the text factor y[j .. j+m-1], the length of the shift is at least equal to one. 
// So, the character y[j+m] is necessarily involved in the next attempt, and thus can be used for the bad-character shift of the current attempt.

class CStrStr
{
public:
    CStrStr();
    
    void BuildBc(LPCSTR pszExpr);
    LPCSTR Search(LPCSTR pszBuf, int cchBuf);

    LPCSTR GetExpression() { return m_Expr; }
    
private:
    enum
    {
        MAX_BC_LEN = 256,
    };
    CHAR m_bc[MAX_BC_LEN];
    CAtlStringA m_Expr;
    BOOL m_fIgnoreCase;
};

