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

inline void WszToString(LPCWSTR pszFrom, std::string& to)
{
    char* pszTo = NULL;
    int cbMultiByte = 0;
    int cchWideChar = wcslen(pszFrom);

    cbMultiByte = WideCharToMultiByte(CP_UTF8, 0, pszFrom, cchWideChar, NULL, cbMultiByte, NULL, NULL);

    if (cbMultiByte > 0)
    {
        pszTo = new char[cbMultiByte+1];

        WideCharToMultiByte(CP_UTF8, 0, pszFrom, cchWideChar, pszTo, cbMultiByte, NULL, NULL);
        pszTo[cbMultiByte] = 0;
        to = pszTo;
        
        delete [] pszTo;
    }
}

inline void WStringToString(const std::wstring& from, std::string& to)
{
    char* pszTo = NULL;
    int cbMultiByte = 0;

	cbMultiByte = WideCharToMultiByte(CP_UTF8, 0, from.c_str(), from.size(), NULL, cbMultiByte, NULL, NULL);

    if (cbMultiByte > 0)
    {
        pszTo = new char[cbMultiByte+1];

		WideCharToMultiByte(CP_UTF8, 0, from.c_str(), from.size(), pszTo, cbMultiByte, NULL, NULL);
        pszTo[cbMultiByte] = 0;
        to = pszTo;
        
        delete [] pszTo;
    }
}

inline void StringToWString(const std::string& from, std::wstring& to)
{
	std::vector<WCHAR> szTextW;

	szTextW.resize(from.length()+1);
	int cchWide = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), from.length(), &szTextW[0], szTextW.size());
	szTextW[cchWide] = '\0';
	to = &szTextW[0];
}
