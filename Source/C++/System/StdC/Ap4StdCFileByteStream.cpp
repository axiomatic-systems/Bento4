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
    if (!strcmp(name, "-stdin")) {
        file = stdin;
    } else if (!strcmp(name, "-stdout")) {
        file = stdout;
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











