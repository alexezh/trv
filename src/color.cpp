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
#include "color.h"

///////////////////////////////////////////////////////////////////////////////
//
CColor & CColor::Instance()
{
	static CColor instance;
	return instance;
}

///////////////////////////////////////////////////////////////////////////////
//
CColor::CColor()
{
    ZeroMemory(m_Palette, sizeof(m_Palette));
    
    m_Palette[DEFAULT_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[DEFAULT_TEXT].m_Fore = RGB(128, 128, 128);

    m_Palette[DEFAULT_FILTER].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[DEFAULT_FILTER].m_Fore = RGB(0, 0, 0);

    m_Palette[BLACK_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[BLACK_TEXT].m_Fore = RGB(0, 0, 0);

    m_Palette[RED_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[RED_TEXT].m_Fore = RGB(255, 0, 0);

    m_Palette[DARKRED_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[DARKRED_TEXT].m_Fore = RGB(128, 0, 0);

    m_Palette[BLUE_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[BLUE_TEXT].m_Fore = RGB(0, 0, 255);

    m_Palette[DARKBLUE_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[DARKBLUE_TEXT].m_Fore = RGB(0, 0, 128);

    m_Palette[GREEN_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[GREEN_TEXT].m_Fore = RGB(0, 255, 0);

    m_Palette[DARKGREEN_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[DARKGREEN_TEXT].m_Fore = RGB(0, 128, 0);

    m_Palette[LIGHTGRAY_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[LIGHTGRAY_TEXT].m_Fore = RGB(192, 192, 192);

    m_Palette[DARKGRAY_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[DARKGRAY_TEXT].m_Fore = RGB(128, 128, 128);

    m_Palette[YELLOW_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[YELLOW_TEXT].m_Fore = RGB(255, 255, 128);

    m_Palette[BROWN_TEXT].m_Back = GetSysColor(COLOR_WINDOW);
    m_Palette[BROWN_TEXT].m_Fore = RGB(128, 64, 64);
}

static LPCSTR ColorNames[] = 
{
	"DefaultText",
	"DefaultFilter",
	"Black",
	"DarkBlue",
	"DarkGreen",
	"DarkCyan",
	"DarkRed",
	"DarkMagenta",
	"Brown",
	"LightGray",
	"DarkGray",
	"Blue",
	"Green",
	"Cyan",
	"Red",
	"Magenta",
	"Yellow",
	"White" 
};

BYTE CColor::FromName(const char * pszName)
{
	for(size_t i = 0; i < _countof(ColorNames); i++)
	{
		if(_stricmp(pszName, ColorNames[i]) == 0)
		{
			return i;
		}
	}

    return 0;                                
}

const char* CColor::Name(BYTE c)
{
	c = (c > _countof(ColorNames)) ? 0 : c;
	return ColorNames[c];
}

