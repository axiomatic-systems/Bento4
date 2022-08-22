/*****************************************************************
|
|    AP4 - Stdc File Byte Stream implementation
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#define _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE64
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <string.h>
#if !defined(_WIN32_WCE)
#include <errno.h>
#include <sys/stat.h>
#endif
#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#endif
#include "Ap4FileByteStream.h"

/*----------------------------------------------------------------------
|   compatibility wrappers
+---------------------------------------------------------------------*/
#if !defined(ENOENT)
#define ENOENT 2
#endif
#if !defined(EACCES)
#define EACCES 13
#endif

#if !defined(AP4_CONFIG_HAVE_FOPEN_S)
static int fopen_s(FILE**      file,
                   const char* filename,
                   const char* mode)
{
    *file = fopen(filename, mode);
#if defined(UNDER_CE)
    if (*file == NULL) return ENOENT;
#else
    if (*file == NULL) return errno;
#endif
    return 0;
}
#endif // defined(AP4_CONFIG_HAVE_FOPEN_S

#if defined(_WIN32)

#include <windows.h>
#include <malloc.h>
#include <limits.h>
#include <assert.h>

#define AP4_WIN32_USE_CHAR_CONVERSION \
    int     _convert = 0;             \
    LPCWSTR _lpw     = NULL;          \
    LPCSTR  _lpa     = NULL

/*----------------------------------------------------------------------
|   A2WHelper
+---------------------------------------------------------------------*/
static LPWSTR
A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
    int ret;

    assert(lpa != NULL);
    assert(lpw != NULL);
    if (lpw == NULL || lpa == NULL) return NULL;

    lpw[0] = '\0';
    ret    = MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
    if (ret == 0) {
        assert(0);
        return NULL;
    }
    return lpw;
}

/*----------------------------------------------------------------------
|   W2AHelper
+---------------------------------------------------------------------*/
static LPSTR
W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
    int ret;

    assert(lpw != NULL);
    assert(lpa != NULL);
    if (lpa == NULL || lpw == NULL) return NULL;

    lpa[0] = '\0';
    ret    = WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
    if (ret == 0) {
        int error = GetLastError();
        assert(error);
        return NULL;
    }
    return lpa;
}

/*----------------------------------------------------------------------
|   conversion macros
+---------------------------------------------------------------------*/
#define AP4_WIN32_A2W(lpa)                                                            \
    (((_lpa = lpa) == NULL) ?                                                         \
    NULL :                                                                            \
    (_convert = MultiByteToWideChar(CP_UTF8, 0, lpa, -1, NULL, 0),                    \
        (INT_MAX / 2 < _convert) ?                                                    \
        NULL :                                                                        \
        A2WHelper((LPWSTR)alloca(_convert * sizeof(WCHAR)), _lpa, _convert, CP_UTF8)))

#define AP4_WIN32_W2A(lpw)                                                      \
    (((_lpw = lpw) == NULL) ?                                                   \
    NULL :                                                                      \
    ((_convert = WideCharToMultiByte(CP_UTF8, 0, lpw, -1, NULL, 0, NULL, NULL), \
       (_convert > INT_MAX / 2) ?                                               \
       NULL :                                                                   \
       W2AHelper((LPSTR)alloca(_convert), _lpw, _convert, CP_UTF8))))

/*----------------------------------------------------------------------
|   AP4_fopen_s_utf8
+---------------------------------------------------------------------*/
static errno_t
AP4_fopen_s_utf8(FILE** file, const char* path, const char* mode)
{
    AP4_WIN32_USE_CHAR_CONVERSION;
    return _wfopen_s(file, AP4_WIN32_A2W(path), AP4_WIN32_A2W(mode));
}

/*----------------------------------------------------------------------
|   remap some functions
+---------------------------------------------------------------------*/
#define fopen_s AP4_fopen_s_utf8

#endif /* _WIN32 */

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream
+---------------------------------------------------------------------*/
class AP4_StdcFileByteStream: public AP4_ByteStream
{
public:
    // class methods
    static AP4_Result Create(AP4_FileByteStream*      delegator,
                             const char*              name,
                             AP4_FileByteStream::Mode mode,
                             AP4_ByteStream*&         stream);
                      
    // methods
    AP4_StdcFileByteStream(AP4_FileByteStream* delegator,
                           FILE*               file, 
                           AP4_LargeSize       size);
    
    ~AP4_StdcFileByteStream();

    // AP4_ByteStream methods
    AP4_Result ReadPartial(void*     buffer, 
                           AP4_Size  bytesToRead, 
                           AP4_Size& bytesRead);
    AP4_Result WritePartial(const void* buffer, 
                            AP4_Size    bytesToWrite, 
                            AP4_Size&   bytesWritten);
    AP4_Result Seek(AP4_Position position);
    AP4_Result Tell(AP4_Position& position);
    AP4_Result GetSize(AP4_LargeSize& size);
    AP4_Result Flush();

    // AP4_Referenceable methods
    void AddReference();
    void Release();

private:
    // members
    AP4_ByteStream* m_Delegator;
    AP4_Cardinal    m_ReferenceCount;
    FILE*           m_File;
    AP4_Position    m_Position;
    AP4_LargeSize   m_Size;
};

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::Create(AP4_FileByteStream*      delegator,
                               const char*              name, 
                               AP4_FileByteStream::Mode mode, 
                               AP4_ByteStream*&         stream)
{
    // default value
    stream = NULL;
    
    // check arguments
    if (name == NULL) return AP4_ERROR_INVALID_PARAMETERS;
    
    // open the file
    FILE* file = NULL;
    AP4_Position size = 0;
    if (!strcmp(name, "-stdin") || !strcmp(name, "-stdin#")) {
        file = stdin;
#if defined(_WIN32)
        if (name[6] == '#') {
            _setmode(fileno(stdin), O_BINARY);
        }
#endif
    } else if (!strcmp(name, "-stdout") || !strcmp(name, "-stdout#")) {
        file = stdout;
#if defined(_WIN32)
        if (name[7] == '#') {
            _setmode(fileno(stdout), O_BINARY);
        }
#endif
    } else if (!strcmp(name, "-stderr")) {
        file = stderr;
    } else {
        int open_result;
        switch (mode) {
          case AP4_FileByteStream::STREAM_MODE_READ:
            open_result = fopen_s(&file, name, "rb");
            break;

          case AP4_FileByteStream::STREAM_MODE_WRITE:
            open_result = fopen_s(&file, name, "wb+");
            break;

          case AP4_FileByteStream::STREAM_MODE_READ_WRITE:
              open_result = fopen_s(&file, name, "r+b");
              break;                                  

          default:
            return AP4_ERROR_INVALID_PARAMETERS;
        }
    
        if (open_result != 0) {
            if (open_result == ENOENT) {
                return AP4_ERROR_NO_SUCH_FILE;
            } else if (open_result == EACCES) {
                return AP4_ERROR_PERMISSION_DENIED;
            } else {
                return AP4_ERROR_CANNOT_OPEN_FILE;
            }
        }

        // get the size
        if (AP4_fseek(file, 0, SEEK_END) >= 0) {
            size = AP4_ftell(file);
            AP4_fseek(file, 0, SEEK_SET);
        }
        
    }

    stream = new AP4_StdcFileByteStream(delegator, file, size);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::AP4_StdcFileByteStream
+---------------------------------------------------------------------*/
AP4_StdcFileByteStream::AP4_StdcFileByteStream(AP4_FileByteStream* delegator,
                                               FILE*               file,
                                               AP4_LargeSize       size) :
    m_Delegator(delegator),
    m_ReferenceCount(1),
    m_File(file),
    m_Position(0),
    m_Size(size)
{
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::~AP4_StdcFileByteStream
+---------------------------------------------------------------------*/
AP4_StdcFileByteStream::~AP4_StdcFileByteStream()
{
    if (m_File && m_File != stdin && m_File != stdout && m_File != stderr) {
        fclose(m_File);
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::AddReference
+---------------------------------------------------------------------*/
void
AP4_StdcFileByteStream::AddReference()
{
    m_ReferenceCount++;
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Release
+---------------------------------------------------------------------*/
void
AP4_StdcFileByteStream::Release()
{
    if (--m_ReferenceCount == 0) {
        if (m_Delegator) {
            delete m_Delegator;
        } else {
            delete this;
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::ReadPartial
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::ReadPartial(void*     buffer, 
                                    AP4_Size  bytesToRead, 
                                    AP4_Size& bytesRead)
{
    size_t nbRead;

    nbRead = fread(buffer, 1, bytesToRead, m_File);

    if (nbRead > 0) {
        bytesRead = (AP4_Size)nbRead;
        m_Position += nbRead;
        return AP4_SUCCESS;
    } else if (feof(m_File)) {
        bytesRead = 0;
        return AP4_ERROR_EOS;
    } else {
        bytesRead = 0;
        return AP4_ERROR_READ_FAILED;
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::WritePartial
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::WritePartial(const void* buffer, 
                                     AP4_Size    bytesToWrite, 
                                     AP4_Size&   bytesWritten)
{
    size_t nbWritten;

    if (bytesToWrite == 0) return AP4_SUCCESS;
    nbWritten = fwrite(buffer, 1, bytesToWrite, m_File);
    
    if (nbWritten > 0) {
        bytesWritten = (AP4_Size)nbWritten;
        m_Position += nbWritten;
        if (m_Position > m_Size) {
            m_Size = m_Position;
        }
        return AP4_SUCCESS;
    } else {
        bytesWritten = 0;
        return AP4_ERROR_WRITE_FAILED;
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Seek
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::Seek(AP4_Position position)
{
    // shortcut
    if (position == m_Position) return AP4_SUCCESS;
    
    size_t result;
    result = AP4_fseek(m_File, position, SEEK_SET);
    if (result == 0) {
        m_Position = position;
        return AP4_SUCCESS;
    } else {
        return AP4_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Tell
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::Tell(AP4_Position& position)
{
    position = m_Position;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::GetSize
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::GetSize(AP4_LargeSize& size)
{
    size = m_Size;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_StdcFileByteStream::Flush
+---------------------------------------------------------------------*/
AP4_Result
AP4_StdcFileByteStream::Flush()
{
    int ret_val = fflush(m_File);
    return (ret_val > 0) ? AP4_FAILURE: AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_FileByteStream::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_FileByteStream::Create(const char*              name, 
                           AP4_FileByteStream::Mode mode,
                           AP4_ByteStream*&         stream)
{
    return AP4_StdcFileByteStream::Create(NULL, name, mode, stream);
}

#if !defined(AP4_CONFIG_NO_EXCEPTIONS)
/*----------------------------------------------------------------------
|   AP4_FileByteStream::AP4_FileByteStream
+---------------------------------------------------------------------*/
AP4_FileByteStream::AP4_FileByteStream(const char*              name, 
                                       AP4_FileByteStream::Mode mode)
{
    AP4_ByteStream* stream = NULL;
    AP4_Result result = AP4_StdcFileByteStream::Create(this, name, mode, stream);
    if (AP4_FAILED(result)) throw AP4_Exception(result);
    
    m_Delegate = stream;
}
#endif











