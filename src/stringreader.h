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

/////////////////////////////////////////////////////////////////////////////
// Implements reader functionality for string
template <class T>
class CStringReaderT
{
public:
    typedef CStringReaderT<T> _Reader;

    CStringReaderT( const T * psz, long cch )
    {
        _psz = psz;
        _pszEnd = psz + cch;
        _hr = S_OK;
    }

	 CStringReaderT(const CStringReaderT& rdr)
	 {
		 _psz = rdr._psz;
		 _pszEnd = rdr._pszEnd;
		 _hr = S_OK;
	 }
	 
	 CStringReaderT()
    {
        _psz = NULL;
        _pszEnd = NULL;
        _hr = S_OK;    
    }

    void Init( const T * psz, long cch )
    {
        _psz = psz;
        _pszEnd = psz + cch;
    }

    void Init( CStringReaderT<T> & rdr )
    {
        _psz = rdr.P();
        _pszEnd = rdr.P() + rdr.Length();
    }

    BOOL IsEOL()  { return !(_psz < _pszEnd); }
    BOOL IsEmpty() { return _psz == NULL; }
    const T * P() { return _psz; }
    const long Length() { return (long)(_pszEnd - _psz); }
    HRESULT GetLastHr() { return _hr; }

    // simple char functions
    T ReadChar();
    T PeekChar();
    void SkipChars(long n);

    // checks if string matches (psz,cch), advances to cch chars
    BOOL MatchString(
                const T * psz, 
                long cch, 
                BOOL fCaseSensitive = TRUE);
    
    // reading of system types
	 static uint32_t ReadUInt32(const T* pszStart, const T* pszEnd, int ibase = 10)
	 {
		 CStringReaderA rdr(pszStart, pszEnd - pszStart);
		 unsigned long temp;
		 rdr.ReadUInt32(&temp, ibase);
		 return temp;
	 }

    BOOL ReadUInt32( 
                unsigned long *pl, 
                int ibase = 10 );
    
    BOOL ReadUInt64(
                unsigned __int64 * pi, 
                int ibase = 10);
    
    BOOL ReadInt64(
                __int64 * pi, 
                int ibase = 10);
    
    BOOL ReadFloat(
                float * pf);

    // advances to the next line (or eos). returns TRUE \r\n is found
    BOOL ReadLine(
                _Reader & Rdr);

    BOOL ReadGuid(
                GUID & Val);
    
    // scans string for specific character (or !character). returns TRUE if char is found
    BOOL ScanChar(
                const T ch,                  // character to look for
                BOOL fPositiveMatch = TRUE,  // look for char or !char
                BOOL fGreedy = FALSE);       // advance position if char is not found
    
    BOOL ScanChars( 
                const T* rch,                // set of chars to search for
                long cch,                    // number of chars in set
                BOOL fPositiveMatch = TRUE,  // look for chars or !chars
                BOOL fGreedy = FALSE );      // advance position if chars is not found
    
    BOOL ScanString( 
                const T* str,                // string to search for
                long cch,                    // number of chars in string
                BOOL fCaseSensitive = TRUE,  // case sensitive
                BOOL fGreedy = FALSE );      // advance position if string is not found

    // 
    void SkipSpace() { ScanChar(' ', FALSE, TRUE); }
   
protected:
    HRESULT _hr;
    const T * _psz;
    const T * _pszEnd;
};

template <class T>
BOOL CStringReaderT<T>::ReadUInt32( unsigned long *pl, int ibase /* = 10 */ )
{
    unsigned long res = 0;
    unsigned long digval;
    T c;
    const T* pszStart = _psz;

    _hr = S_OK;

    if( NULL == pl )
    {
        _hr = E_INVALIDARG;
        return( FALSE );
    }

    for(; _psz < _pszEnd; _psz++)
    {
        c = *_psz;
        
        if( sizeof(T) == sizeof(char))
        {
            /* convert c to value */
            if ( isdigit((unsigned char)c) )
                    digval = c - '0';
            else if ( isalpha((unsigned char)c) )
                    digval = toupper(c) - 'A' + 10;
            else
                    break;
            if (digval >= (unsigned)ibase)
                    break;          /* exit loop if bad digit found */
        }
        else
        {
            /* convert c to value */
            if (iswdigit((unsigned short)c))
                digval = c - L'0';
            else if (iswalpha((unsigned short)c))
                digval = towupper(c) - L'A' + 10;
            else
                break;
            if (digval >= (unsigned)ibase)
                break;      /* exit loop if bad digit found */
        }
        
        res = res * ibase + digval;
    }

    if( pszStart == _psz )
    {
        _hr = E_FAIL;
        return( FALSE );
    }
    else
    {
        *pl = res;
        return( TRUE );
    }
}

template <class T>
BOOL CStringReaderT<T>::ReadUInt64(unsigned __int64 * pi, int ibase)
{
    unsigned __int64 res = 0;
    unsigned long digval;
    T c;
    const T* pszStart = _psz;

    _hr = S_OK;

    if( NULL == pi )
    {
        _hr = E_INVALIDARG;
        return( FALSE );
    }

    for(; _psz < _pszEnd; _psz++)
    {
        c = *_psz;
        
        if( sizeof(T) == sizeof(char))
        {
            /* convert c to value */
            if ( isdigit((unsigned char)c) )
                    digval = c - '0';
            else if ( isalpha( (unsigned char)c ) )
                    digval = toupper(c) - 'A' + 10;
            else
                    break;
            if (digval >= (unsigned)ibase)
                    break;          /* exit loop if bad digit found */
        }
        else
        {
            /* convert c to value */
            if (iswdigit(c))
                digval = c - L'0';
            else if (iswalpha(c))
                digval = towupper(c) - L'A' + 10;
            else
                break;
            if (digval >= (unsigned)ibase)
                break;      /* exit loop if bad digit found */
        }
        
        res = res * ibase + digval;
    }

    if( pszStart == _psz )
    {
        _hr = E_FAIL;
        return( FALSE );
    }
    else
    {
        *pi = res;
        return( TRUE );
    }
}

template <class T>
BOOL CStringReaderT<T>::ReadInt64(__int64 * pi, int ibase)
{
    int sign = 1;

    _hr = S_OK;

    if( NULL == pi )
    {
        _hr = E_INVALIDARG;
        return( FALSE );
    }

    if(_psz < _pszEnd)
    {
        if(*_psz == '-')
        {
            sign = -1;
        }

        ReadUInt64((unsigned __int64 *)pi, ibase);

        if(sign < 0 )
        {
            *pi = -*pi;
        }
    }

    return TRUE;    
}

template <class T>
BOOL CStringReaderT<T>::ReadFloat( float *pfloat )
{
    float res = 0.0;
    float decimal = 0.0;
    unsigned long digval;
    T c;
    const T* pszStart = _psz;
    float decimalDigitWeight = 1.0;
    BOOL fAfterDecimal = FALSE;
    BOOL fNegative = FALSE;
    _hr = S_OK;

    if( NULL == pfloat )
    {
        _hr = E_INVALIDARG;
        return( FALSE );
    }

    for(; _psz < _pszEnd; _psz++)
    {
        c = *_psz;

        if( sizeof(T) == sizeof(char))
        {
            /* convert c to value */
            if ( isdigit( (unsigned char)c) )
            {
                digval = c - '0';
            }                    
            else if( '.' == (unsigned char)c && 
                    ! fAfterDecimal )
            {
                fAfterDecimal = TRUE;
                continue;
            }
            else if( _psz == pszStart &&
                        '-' == c )
            {
                fNegative = TRUE;
                continue;
            }
            else
            {
                break;
            }                
        }
        else
        {
            /* convert c to value */
            if (iswdigit(c))
            {
                digval = c - L'0';
            }
            else if( L'.' == c && 
                ! fAfterDecimal )
            {
                fAfterDecimal = TRUE;
                continue;
            }
            else if( _psz == pszStart &&
                        '-' == c )
            {
                fNegative = TRUE;
                continue;
            }
            else
            {
                break;
            }                
        }

        if( fAfterDecimal )
        {
            decimal = decimal * 10 + digval;
            decimalDigitWeight *= 10;
        }
        else
        {
            res = res * 10 + digval;
        }
    }

    if( pszStart == _psz )
    {
        _hr = E_FAIL;
        return( FALSE );
    }
    else
    {
        if( 0.0 != decimalDigitWeight )
        {
            res += ( decimal / decimalDigitWeight );
        }            

        if( fNegative )
        {
            res *= -1.0;
        }
        
        *pfloat = res;
        return( TRUE );
    }
}

template <class T>
inline T CStringReaderT<T>::ReadChar()
{
    if( _psz < _pszEnd )
    {
        return *_psz++;
    }
    else
    {
        _hr = E_FAIL;
        return '\0';
    }
}

template <class T>
BOOL CStringReaderT<T>::MatchString(
    const T * psz,
    long cch,
    BOOL fCaseSensitive /* = TRUE */
    )
{
    if( _psz + cch - 1 < _pszEnd )
    {
        int iRet;
        
        if(sizeof(T) == sizeof(WCHAR))
        {
            if( fCaseSensitive )
            {
                iRet = wcsncmp((const WCHAR*)_psz, (const WCHAR*)psz, cch);
            }
            else
            {
                iRet = _wcsnicmp((const WCHAR*)_psz, (const WCHAR*)psz, cch);
            }
        }
        else
        {
            if( fCaseSensitive )
            {
                iRet = strncmp((const char*)_psz, (const char*)psz, cch);
            }
            else
            {
                iRet = _strnicmp((const char*)_psz, (const char*)psz, cch);
            }
        }

        if(iRet == 0)
        {
            _psz += cch;
            return TRUE;
        }
    }

    return FALSE;
}

template <class T>
BOOL CStringReaderT<T>::ReadLine(_Reader & Rdr)
{
    BOOL fRet;
    const T szLineBreak[] = { (T)'\r', (T)'\n', (T)'\0' };
    T * pszStart = _psz;

    fRet = ScanString( szLineBreak, 2, TRUE );
    if(!fRet)
    {
        return FALSE;
    }

    SkipChars(2);

    Rdr.Init(pszStart, _psz);

    return TRUE;
}

template <class T>
BOOL CStringReaderT<T>::ReadGuid(GUID & Val)
{
    int i;
    const T* pszStart = _psz;

    Val.Data1 = ReadLong(16);
    if(ReadChar() != '-')
    {
        _hr = E_INVALIDARG;
        _psz = pszStart;
        return FALSE;
    }
    Val.Data2 = (unsigned short)ReadLong(16);
    if(ReadChar() != '-')
    {
        _hr = E_INVALIDARG;
        _psz = pszStart;
        return FALSE;
    }
    Val.Data3 = (unsigned short)ReadLong(16);
    if(ReadChar() != '-')
    {
        _hr = E_INVALIDARG;
        _psz = pszStart;
        return FALSE;
    }

    for(i=0; i<8; i++)
    {
        CStringReaderT<T> rdr(P(), 2);
        SkipChars(2);
        Val.Data4[i] = (unsigned char)rdr.ReadLong(16);
        if(i == 1)
        {
            if(ReadChar() != '-')
            {
                _hr = E_INVALIDARG;
                _psz = pszStart;
                return FALSE;
            }
        }
    }

    return TRUE;
}

template <class T>
BOOL CStringReaderT<T>::ScanChar(
    const T ch, 
    BOOL fPositiveMatch /* = TRUE */, 
    BOOL fGreedy /* = FALSE */ )
{
    if( _psz < _pszEnd )
    {
        const T * psz2 = _psz;

        if( fPositiveMatch )
        {
            // stop once a match is found
            for(;psz2 < _pszEnd; psz2++)
            {
                if(*psz2 == ch)
                {
                    _psz = psz2;
                    return TRUE;
                }
            }
        }
        else
        {
            // stop once the current char is not a match
            for(;psz2 < _pszEnd; psz2++)
            {
                if( *psz2 != ch )
                {
                    _psz = psz2;
                    return TRUE;
                }
            }
        }
    }

    if(fGreedy)
    {
        _psz = _pszEnd;
    }

    return FALSE;
}

template <class T>
BOOL CStringReaderT<T>::ScanChars( 
    const T* rch, 
    long cch, 
    BOOL fPositiveMatch /* = TRUE */,
    BOOL fGreedy /* = FALSE */ )
{
    if( _psz < _pszEnd )
    {
        const T * psz1 = _psz;
        const T * psz2 = _psz;

        if( fPositiveMatch )
        {
            // stop once a match is found
            for( ;psz2 < _pszEnd; psz2++ )
            {
                for( int i = 0; i < cch; i++ )
                {
                    if( *psz2 == rch[i] )
                    {
                        _psz = psz2;
                        return TRUE;
                    }
                }
            }
        }
        else
        {
            // stop once the current char is not a match
            for( ;psz2 < _pszEnd; psz2++ )
            {
                int i;
                for( i = 0; i < cch; i++ )
                {
                    if( *psz2 == rch[i] )
                    {
                        break;
                    }
                }

                if( i == cch )
                {
                    _psz = psz2;
                    return TRUE;
                }
            }
        }
    }

    if(fGreedy)
    {
        _psz = _pszEnd;
    }

    return FALSE;
}

template <class T>
BOOL CStringReaderT<T>::ScanString( 
    const T* str, 
    long cch, 
    BOOL fCaseSensitive /* = TRUE */,
    BOOL fGreedy /* = FALSE */ )
{
    if( 0 == cch )
    {
        return NULL;
    }
    
    if( _psz < _pszEnd )
    {
        const T * psz1 = _psz;
        const T * psz2 = _psz;

        if( fCaseSensitive )
        {
            for( ;psz2 < _pszEnd - cch + 1; psz2++ )
            {
                int i;
                for( i = 0; i < cch; i++ )
                {
                    if( psz2[i] != str[i] )
                    {
                        break;
                    }
                }

                if( i == cch )
                {
                    _psz = psz2;
                    return TRUE;
                }
            }
        }
        else
        {
            for( ;psz2 < _pszEnd - cch + 1; psz2++ )
            {
                int i;
                for( i = 0; i < cch; i++ )
                {
                    if( wms_tolower( psz2[i] ) != wms_tolower( str[i] ) )
                    {
                        break;
                    }
                }

                if( i == cch )
                {
                    _psz = psz2;
                    return TRUE;
                }
            }
        
        }
    }

    if(fGreedy)
    {
        _psz = _pszEnd;
    }

    return FALSE;
}


template <class T>
T CStringReaderT<T>::PeekChar()
{
    if( _psz < _pszEnd )
    {
        return *_psz;
    }
    else
    {
        _hr = E_FAIL;
        return '\0';
    }
}

template <class T>
void CStringReaderT<T>::SkipChars(long n)
{
    if( _psz + n <= _pszEnd )
    {
        _psz += n;
    }
    else
    {
        _psz = _pszEnd;
    }
}


typedef CStringReaderT<wchar_t> CStringReaderW;
typedef CStringReaderT<char> CStringReaderA;
