/*****************************************************************
|
|    AP4 - Android File Byte Stream implementation
|
|    Copyright 2002-2016 Axiomatic Systems, LLC
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "Ap4FileByteStream.h"

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream
+---------------------------------------------------------------------*/
class AP4_AndroidFileByteStream: public AP4_ByteStream
{
public:
    // class methods
    static AP4_Result Create(AP4_FileByteStream*      delegator,
                             const char*              name,
                             AP4_FileByteStream::Mode mode,
                             AP4_ByteStream*&         stream);
                      
    // methods
    AP4_AndroidFileByteStream(AP4_FileByteStream* delegator,
                              int                 fd,
                              AP4_LargeSize       size);
    
    ~AP4_AndroidFileByteStream();

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
    int             m_FD;
    AP4_Position    m_Position;
    AP4_LargeSize   m_Size;
};

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_AndroidFileByteStream::Create(AP4_FileByteStream*      delegator,
                                  const char*              name,
                                  AP4_FileByteStream::Mode mode,
                                  AP4_ByteStream*&         stream)
{
    // default value
    stream = NULL;
    
    // check arguments
    if (name == NULL) return AP4_ERROR_INVALID_PARAMETERS;
    
    // open the file
    int fd = 0;
    AP4_Position size = 0;
    if (!strcmp(name, "-stdin")) {
        fd = STDIN_FILENO;
    } else if (!strcmp(name, "-stdout")) {
        fd = STDOUT_FILENO;
    } else if (!strcmp(name, "-stderr")) {
        fd = STDERR_FILENO;
    } else {
        int open_flags = 0;
        int create_perm = 0;
        switch (mode) {
          case AP4_FileByteStream::STREAM_MODE_READ:
            open_flags = O_RDONLY;
            break;

          case AP4_FileByteStream::STREAM_MODE_WRITE:
            open_flags = O_RDWR | O_CREAT | O_TRUNC;
            break;

          case AP4_FileByteStream::STREAM_MODE_READ_WRITE:
            open_flags = O_RDWR;
            break;

          default:
            return AP4_ERROR_INVALID_PARAMETERS;
        }
    
        fd = open(name, open_flags, create_perm);
        if (fd < 0) {
            if (errno == ENOENT) {
                return AP4_ERROR_NO_SUCH_FILE;
            } else if (errno == EACCES) {
                return AP4_ERROR_PERMISSION_DENIED;
            } else {
                return AP4_ERROR_CANNOT_OPEN_FILE;
            }
        }

        // get the size
        struct stat info;
        if (stat(name, &info) == 0) {
            size = info.st_size;
        }
    }
        
    stream = new AP4_AndroidFileByteStream(delegator, fd, size);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::AP4_AndroidFileByteStream
+---------------------------------------------------------------------*/
AP4_AndroidFileByteStream::AP4_AndroidFileByteStream(AP4_FileByteStream* delegator,
                                                     int                 fd,
                                                     AP4_LargeSize       size) :
    m_Delegator(delegator),
    m_ReferenceCount(1),
    m_FD(fd),
    m_Position(0),
    m_Size(size)
{
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::~AP4_AndroidFileByteStream
+---------------------------------------------------------------------*/
AP4_AndroidFileByteStream::~AP4_AndroidFileByteStream()
{
    if (m_FD && m_FD != STDERR_FILENO && m_FD != STDOUT_FILENO && m_FD != STDERR_FILENO) {
        close(m_FD);
    }
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::AddReference
+---------------------------------------------------------------------*/
void
AP4_AndroidFileByteStream::AddReference()
{
    m_ReferenceCount++;
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::Release
+---------------------------------------------------------------------*/
void
AP4_AndroidFileByteStream::Release()
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
|   AP4_AndroidFileByteStream::ReadPartial
+---------------------------------------------------------------------*/
AP4_Result
AP4_AndroidFileByteStream::ReadPartial(void*     buffer, 
                                       AP4_Size  bytes_to_read,
                                       AP4_Size& bytes_read)
{
    ssize_t nb_read = read(m_FD, buffer, bytes_to_read);

    if (nb_read > 0) {
        bytes_read = (AP4_Size)nb_read;
        m_Position += nb_read;
        return AP4_SUCCESS;
    } else if (nb_read == 0) {
        bytes_read = 0;
        return AP4_ERROR_EOS;
    } else {
        bytes_read = 0;
        return AP4_ERROR_READ_FAILED;
    }
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::WritePartial
+---------------------------------------------------------------------*/
AP4_Result
AP4_AndroidFileByteStream::WritePartial(const void* buffer, 
                                        AP4_Size    bytes_to_write,
                                        AP4_Size&   bytes_written)
{
    if (bytes_to_write == 0) {
        bytes_written = 0;
        return AP4_SUCCESS;
    }
    ssize_t nb_written = write(m_FD, buffer, bytes_to_write);
    
    if (nb_written > 0) {
        bytes_written = (AP4_Size)nb_written;
        m_Position += nb_written;
        return AP4_SUCCESS;
    } else {
        bytes_written = 0;
        return AP4_ERROR_WRITE_FAILED;
    }
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::Seek
+---------------------------------------------------------------------*/
AP4_Result
AP4_AndroidFileByteStream::Seek(AP4_Position position)
{
    // shortcut
    if (position == m_Position) return AP4_SUCCESS;
    
    off64_t result = lseek64(m_FD, position, SEEK_SET);
    if (result >= 0) {
        m_Position = position;
        return AP4_SUCCESS;
    } else {
        return AP4_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::Tell
+---------------------------------------------------------------------*/
AP4_Result
AP4_AndroidFileByteStream::Tell(AP4_Position& position)
{
    position = m_Position;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::GetSize
+---------------------------------------------------------------------*/
AP4_Result
AP4_AndroidFileByteStream::GetSize(AP4_LargeSize& size)
{
    size = m_Size;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AndroidFileByteStream::Flush
+---------------------------------------------------------------------*/
AP4_Result
AP4_AndroidFileByteStream::Flush()
{
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_FileByteStream::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_FileByteStream::Create(const char*              name, 
                           AP4_FileByteStream::Mode mode,
                           AP4_ByteStream*&         stream)
{
    return AP4_AndroidFileByteStream::Create(NULL, name, mode, stream);
}

#if !defined(AP4_CONFIG_NO_EXCEPTIONS)
/*----------------------------------------------------------------------
|   AP4_FileByteStream::AP4_FileByteStream
+---------------------------------------------------------------------*/
AP4_FileByteStream::AP4_FileByteStream(const char*              name, 
                                       AP4_FileByteStream::Mode mode)
{
    AP4_ByteStream* stream = NULL;
    AP4_Result result = AP4_AndroidFileByteStream::Create(this, name, mode, stream);
    if (AP4_FAILED(result)) throw AP4_Exception(result);
    
    m_Delegate = stream;
}
#endif











