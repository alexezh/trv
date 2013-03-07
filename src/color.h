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
class CColor
{
public:    
    struct PAL_ELEM
    {
        COLORREF m_Back;
        COLORREF m_Fore;
    };
/*    
    enum COLOR
    {
        BLACK               =0,
        DARKBLUE            =1,
        DARKGREEN           =2,
        DARKCYAN            =3,
        DARKRED             =4,
        DARKMAGENTA         =5,
        BROWN, DARKYELLOW       =6,
        LIGHTGRAY           =7,
        DARKGRAY            =8,
        BLUE                =9, 
        GREEN               =10, 
        CYAN                =11, 
        RED                 =12, 
        MAGENTA             =13, 
        YELLOW              =14, 
        WHITE               =15,
    };
*/
    enum COLOR
    {
        DEFAULT_TEXT = 0,
        DEFAULT_FILTER = 1,
        BLACK_TEXT = 2,
        DARKBLUE_TEXT,
        DARKGREEN_TEXT,
        DARKCYAN_TEXT,
        DARKRED_TEXT,
        DARKMAGENTA_TEXT,
        BROWN_TEXT,
        LIGHTGRAY_TEXT,
        DARKGRAY_TEXT,
        BLUE_TEXT, 
        GREEN_TEXT, 
        CYAN_TEXT, 
        RED_TEXT, 
        MAGENTA_TEXT, 
        YELLOW_TEXT, 
        WHITE_TEXT,
        MAX_COLOR,
    };

    CColor();

	static CColor & Instance();
	static BYTE FromName(const char * name);
	static const char* Name(BYTE c);

    COLORREF GetBackColor(BYTE bColor) { return m_Palette[bColor].m_Back; }
    COLORREF GetForeColor(BYTE bColor) { return m_Palette[bColor].m_Fore; }

private:
    PAL_ELEM m_Palette[MAX_COLOR];
};
