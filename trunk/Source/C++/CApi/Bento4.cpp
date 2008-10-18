/*****************************************************************
|
|    Bento4 - C API Implementation
|
|    Copyright 2002-2008 Gilles Boccon-Gibod & Julien Boeuf
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
#include "Bento4.h"
#include "Ap4.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const int AP4_FILE_BYTE_STREAM_MODE_READ = AP4_FileByteStream::STREAM_MODE_READ;
const int AP4_FILE_BYTE_STREAM_MODE_WRITE = AP4_FileByteStream::STREAM_MODE_WRITE;
const int AP4_FILE_BYTE_STREAM_MODE_READ_WRITE = AP4_FileByteStream::STREAM_MODE_READ_WRITE;

/*----------------------------------------------------------------------
|   AP4_ByteStream_AddReference
+---------------------------------------------------------------------*/
void 
AP4_ByteStream_AddReference(AP4_ByteStream* self)
{
    self->AddReference();
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_Release
+---------------------------------------------------------------------*/
void 
AP4_ByteStream_Release(AP4_ByteStream* self)
{
    self->Release();
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_ReadPartial
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_ReadPartial(AP4_ByteStream*  self,
                           void*            buffer,
                           AP4_Size         bytes_to_read,
                           AP4_Size*        bytes_read)
{
    return self->ReadPartial(buffer, bytes_to_read, *bytes_read);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_Read
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_Read(AP4_ByteStream* self, 
                    void*           buffer, 
                    AP4_Size        bytes_to_read)
{
    return self->Read(buffer, bytes_to_read);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_ReadDouble
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_ReadDouble(AP4_ByteStream* self, double* value)
{
    return self->ReadDouble(*value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_ReadUI64
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_ReadUI64(AP4_ByteStream* self, AP4_UI64* value)
{
    return self->ReadUI64(*value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_ReadUI32
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_ReadUI32(AP4_ByteStream* self, AP4_UI32* value)
{
    return self->ReadUI32(*value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_ReadUI24
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_ReadUI24(AP4_ByteStream* self, AP4_UI32* value)
{
    return self->ReadUI24(*value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_ReadUI16
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_ReadUI16(AP4_ByteStream* self, AP4_UI16* value)
{
    return self->ReadUI16(*value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_ReadUI08
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_ReadUI08(AP4_ByteStream* self, AP4_UI08* value)
{
    return self->ReadUI08(*value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_ReadString
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_ReadString(AP4_ByteStream* self, 
                          char*           buffer, 
                          AP4_Size        size)
{
    return self->ReadString(buffer, size);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_WritePartial
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_WritePartial(AP4_ByteStream* self,
                            const void*     buffer,
                            AP4_Size        bytes_to_write,
                            AP4_Size*       bytes_written)
{
    return self->WritePartial(buffer, bytes_to_write, *bytes_written);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_Write
+---------------------------------------------------------------------*/
AP4_Result
AP4_ByteStream_Write(AP4_ByteStream* self,
                     const void*     buffer,
                     AP4_Size        bytes_to_write)
{
    return self->Write(buffer, bytes_to_write);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_WriteDouble
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_WriteDouble(AP4_ByteStream* self, double value)
{
    return self->WriteDouble(value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_WriteUI64
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_WriteUI64(AP4_ByteStream* self, AP4_UI64 value)
{
    return self->WriteUI64(value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_WriteUI32
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_WriteUI32(AP4_ByteStream* self, AP4_UI32 value)
{
    return self->WriteUI32(value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_WriteUI24
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_WriteUI24(AP4_ByteStream* self, AP4_UI32 value)
{
    return self->WriteUI24(value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_WriteUI16
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_WriteUI16(AP4_ByteStream* self, AP4_UI16 value)
{
    return self->WriteUI16(value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_WriteUI08
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_WriteUI08(AP4_ByteStream* self, AP4_UI08 value)
{
    return self->WriteUI08(value);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_WriteString
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_WriteString(AP4_ByteStream* self, const char* buffer)
{
    return self->WriteString(buffer);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_Seek
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_Seek(AP4_ByteStream* self, AP4_Position position)
{
    return self->Seek(position);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_Tell
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_Tell(AP4_ByteStream* self, AP4_Position* position)
{
    return self->Tell(*position);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_GetSize
+---------------------------------------------------------------------*/
AP4_Result
AP4_ByteStream_GetSize(AP4_ByteStream* self, AP4_LargeSize* size)
{
    return self->GetSize(*size);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_CopyTo
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ByteStream_CopyTo(AP4_ByteStream* self, 
                      AP4_ByteStream* receiver, 
                      AP4_LargeSize   size)
{
    return self->CopyTo(*receiver, size);
}

/*----------------------------------------------------------------------
|   Ap4_ByteStream_Flush
+---------------------------------------------------------------------*/
AP4_Result 
Ap4_ByteStream_Flush(AP4_ByteStream* self)
{
    return self->Flush();
}

/*----------------------------------------------------------------------
|   AP4_SubStream_Create
+---------------------------------------------------------------------*/
AP4_ByteStream* 
AP4_SubStream_Create(AP4_ByteStream* container, 
                     AP4_Position     position, 
                     AP4_LargeSize    size)
{
    return new AP4_SubStream(*container, position, size);
}

/*----------------------------------------------------------------------
|   AP4_MemoryByteStream_Create
+---------------------------------------------------------------------*/
AP4_ByteStream* 
AP4_MemoryByteStream_Create(AP4_Size size)
{
    return new AP4_MemoryByteStream(size);
}

/*----------------------------------------------------------------------
|   AP4_MemoryByteStream_FromBuffer
+---------------------------------------------------------------------*/
AP4_ByteStream* 
AP4_MemoryByteStream_FromBuffer(const AP4_UI08* buffer, AP4_Size size)
{
    return new AP4_MemoryByteStream(buffer, size);
}

/*----------------------------------------------------------------------
|   AP4_MemoryByteStream_AdaptDataBuffer
+---------------------------------------------------------------------*/
AP4_ByteStream*
AP4_MemoryByteStream_AdaptDataBuffer(AP4_DataBuffer* buffer)
{
    return new AP4_MemoryByteStream(*buffer);
}

/*----------------------------------------------------------------------
|   AP4_FileByteStream_Create
+---------------------------------------------------------------------*/
AP4_ByteStream*
AP4_FileByteStream_Create(const char* name, int mode)
{
    return new AP4_FileByteStream(name, (AP4_FileByteStream::Mode) mode);
}



