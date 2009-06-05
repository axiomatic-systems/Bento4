/*****************************************************************
|
|    AP4 - Buffered Stream Test
|
|    Copyright 2002-2009 Axiomatic Systems, LLC
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
#include <stdlib.h>

#include "Ap4.h"

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define CHECK(x) do { \
    if (!(x)) { fprintf(stderr, "ERROR line %d", __LINE__); return DebugHook(); }\
} while (0)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "Buffered Stream Test - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2009 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   DebugHook
+---------------------------------------------------------------------*/
static int
DebugHook()
{
    return -1;
}

/*----------------------------------------------------------------------
|   TestStream
+---------------------------------------------------------------------*/
class TestStream : public AP4_ByteStream
{
public:
    TestStream(AP4_Size size, bool partial);

    // AP4_ByteStream methods
    AP4_Result ReadPartial(void*     buffer, 
                           AP4_Size  bytes_to_read, 
                           AP4_Size& bytes_read);
    AP4_Result WritePartial(const void* buffer, 
                            AP4_Size    bytes_to_write, 
                            AP4_Size&   bytes_written);
    AP4_Result Seek(AP4_Position position);
    AP4_Result Tell(AP4_Position& position) {
        position = m_Position;
        return AP4_SUCCESS;
    }
    AP4_Result GetSize(AP4_LargeSize& size) {
        size = m_Buffer.GetDataSize();
        return AP4_SUCCESS;
    }

    // AP4_Referenceable methods
    void AddReference();
    void Release();

private:
    AP4_DataBuffer m_Buffer;
    bool           m_Partial;
    AP4_Position   m_Position;
    AP4_Cardinal   m_ReferenceCount;
};

/*----------------------------------------------------------------------
|   TestStream::TestStream
+---------------------------------------------------------------------*/
TestStream::TestStream(AP4_Size size, bool partial) :
    m_Position(0),
    m_Partial(partial),
    m_ReferenceCount(1)
{
    m_Buffer.SetDataSize(size);
    for (unsigned int i=0; i<size; i++) {
        m_Buffer.UseData()[i] = (unsigned char)i;
    }
}

/*----------------------------------------------------------------------
|   TestStream::ReadPartial
+---------------------------------------------------------------------*/
AP4_Result 
TestStream::ReadPartial(void*     buffer, 
                        AP4_Size  bytes_to_read, 
                        AP4_Size& bytes_read)
{
    // default values
    bytes_read = 0;

    // shortcut
    if (bytes_to_read == 0) {
        return AP4_SUCCESS;
    }

    // clamp to range
    if (m_Position+bytes_to_read > m_Buffer.GetDataSize()) {
        bytes_to_read = (AP4_Size)(m_Buffer.GetDataSize() - m_Position);
    }

    // check for end of stream
    if (bytes_to_read == 0) {
        return AP4_ERROR_EOS;
    }

    if (m_Partial) {
        bytes_to_read = ((unsigned int)rand())%bytes_to_read;
        if (bytes_to_read == 0) bytes_to_read = 1;
    }
    
    // read from the memory
    AP4_CopyMemory(buffer, m_Buffer.GetData()+m_Position, bytes_to_read);
    m_Position += bytes_to_read;

    bytes_read = bytes_to_read;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   TestStream::WritePartial
+---------------------------------------------------------------------*/
AP4_Result 
TestStream::WritePartial(const void* buffer, 
                         AP4_Size    bytes_to_write, 
                         AP4_Size&   bytes_written)
{
    // default values
    bytes_written = 0;

    // shortcut
    if (bytes_to_write == 0) {
        return AP4_SUCCESS;
    }

    // clamp to range
    if (m_Position+bytes_to_write > m_Buffer.GetDataSize()) {
        bytes_to_write = (AP4_Size)(m_Buffer.GetDataSize() - m_Position);
    }

    // write to memory
    AP4_CopyMemory((void*)(m_Buffer.UseData()+m_Position), buffer, bytes_to_write);
    m_Position += bytes_to_write;

    bytes_written = bytes_to_write;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   TestStream::Seek
+---------------------------------------------------------------------*/
AP4_Result 
TestStream::Seek(AP4_Position position)
{
    if (position > m_Buffer.GetDataSize()) return AP4_FAILURE;
    m_Position = position;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   TestStream::AddReference
+---------------------------------------------------------------------*/
void
TestStream::AddReference()
{
    m_ReferenceCount++;
}

/*----------------------------------------------------------------------
|   TestStream::Release
+---------------------------------------------------------------------*/
void
TestStream::Release()
{
    if (--m_ReferenceCount == 0) {
        delete this;
    }
}

/*----------------------------------------------------------------------
|   DoTest
+---------------------------------------------------------------------*/
static int
DoTest(unsigned int buffer_size,  unsigned int seek_as_read_threshold, unsigned int source_size, bool partial)
{
    TestStream* source = new TestStream(source_size, partial);
    AP4_BufferedInputStream* stream = new AP4_BufferedInputStream(*source, buffer_size, seek_as_read_threshold);
    unsigned char* buffer = new unsigned char[4096];
    
    // read all linearly
    AP4_Position position = 0;
    AP4_Result result;
    for (;;) {
        unsigned int chunk = ((unsigned int)rand())%(2*buffer_size);
        AP4_Size bytes_read = 0;
        result = stream->ReadPartial(buffer, chunk, bytes_read);
        if (result == AP4_SUCCESS) {
            if (chunk) CHECK(bytes_read);
            CHECK(bytes_read <= chunk);
            for (unsigned int i=0; i<bytes_read; i++) {
                CHECK(buffer[i+position] == (unsigned char)(buffer[i+position]));
            }
            position += bytes_read;
            AP4_Position where;
            stream->Tell(where);
            CHECK(where == position);
        } else {
            CHECK(result == AP4_ERROR_EOS);
            CHECK(position == source_size);
            break;
        }
    }
    
    // read with seeks
    position = 0;
    result = stream->Seek(0);
    CHECK(result == AP4_SUCCESS);
    AP4_Position where;
    stream->Tell(where);
    CHECK(where == 0);
    for (unsigned int i=0; i<1000; i++) {
        if ((rand()%7) == 0) {
            position = (unsigned int)rand()%source_size;
            result = stream->Seek(position);
            CHECK(result == AP4_SUCCESS);
            AP4_Position where;
            stream->Tell(where);
            CHECK(where == position);
        }

        unsigned int chunk = ((unsigned int)rand())%(2*buffer_size);
        AP4_Size bytes_read = 0;
        result = stream->ReadPartial(buffer, chunk, bytes_read);
        if (result == AP4_SUCCESS) {
            if (chunk) CHECK(bytes_read);
            CHECK(bytes_read <= chunk);
            for (unsigned int i=0; i<bytes_read; i++) {
                CHECK(buffer[i+position] == (unsigned char)(buffer[i+position]));
            }
            position += bytes_read;
            AP4_Position where;
            stream->Tell(where);
            CHECK(where == position);
        } else {
            CHECK(result == AP4_ERROR_EOS);
            break;
        }
    }
    
    stream->Release();
    source->Release();
    delete[] buffer;
    return 0;
}

/*----------------------------------------------------------------------
|   TestBuffer
+---------------------------------------------------------------------*/
static int
TestBuffer(unsigned int buffer_size)
{
    for (unsigned int source_size=1; source_size<buffer_size*32; source_size += 17) {
        for (unsigned int seek_as_read_threshold=0; seek_as_read_threshold < 128; seek_as_read_threshold+=16) { 
            int result = DoTest(buffer_size, seek_as_read_threshold, source_size, true);
            if (result < 0) return result;
            result = DoTest(buffer_size, seek_as_read_threshold, source_size, false);
            if (result < 0) return result;
        }
    }
    
    return 0;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    for (unsigned int buffer_size=1; buffer_size<256; buffer_size++) {
        int result = TestBuffer(buffer_size);
        if (result < 0) return 1;
    }
    
    return 0;                                            
}

