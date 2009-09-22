/*****************************************************************
|
|    AP4 - Neptune Adapters
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
#include "Ap4NeptuneAdapters.h"


/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::NPT_InputStream_To_AP4_ByteStream_Adapter
+---------------------------------------------------------------------*/
NPT_InputStream_To_AP4_ByteStream_Adapter::NPT_InputStream_To_AP4_ByteStream_Adapter(NPT_InputStreamReference& stream)
    : m_Stream(stream), m_ReferenceCount(1)
{
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::~NPT_InputStream_To_AP4_ByteStream_Adapter
+---------------------------------------------------------------------*/
NPT_InputStream_To_AP4_ByteStream_Adapter::~NPT_InputStream_To_AP4_ByteStream_Adapter()
{
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::ReadPartial
+---------------------------------------------------------------------*/
AP4_Result
NPT_InputStream_To_AP4_ByteStream_Adapter::ReadPartial(void*     buffer, 
                                                       AP4_Size  bytes_to_read, 
                                                       AP4_Size& bytes_read)
{
    return MapResult(m_Stream->Read(buffer, bytes_to_read, &bytes_read));
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::WritePartial
+---------------------------------------------------------------------*/
AP4_Result
NPT_InputStream_To_AP4_ByteStream_Adapter::WritePartial(const void* , 
                                                        AP4_Size    , 
                                                        AP4_Size&   )
{
    return AP4_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::Seek
+---------------------------------------------------------------------*/
AP4_Result
NPT_InputStream_To_AP4_ByteStream_Adapter::Seek(AP4_Position position)
{
    return MapResult(m_Stream->Seek(position));
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::
+---------------------------------------------------------------------*/
AP4_Result
NPT_InputStream_To_AP4_ByteStream_Adapter::Tell(AP4_Position& position)
{
    return MapResult(m_Stream->Tell(position));
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::GetSize
+---------------------------------------------------------------------*/
AP4_Result
NPT_InputStream_To_AP4_ByteStream_Adapter::GetSize(AP4_LargeSize& size)
{
    return MapResult(m_Stream->GetSize(size));
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::AddReference
+---------------------------------------------------------------------*/
void
NPT_InputStream_To_AP4_ByteStream_Adapter::AddReference()
{
    ++m_ReferenceCount;
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::Release
+---------------------------------------------------------------------*/
void
NPT_InputStream_To_AP4_ByteStream_Adapter::Release()
{
    if (--m_ReferenceCount == 0) {
        delete this;
    }
}

/*----------------------------------------------------------------------
|   NPT_InputStream_To_AP4_ByteStream_Adapter::MapResult
+---------------------------------------------------------------------*/
AP4_Result
NPT_InputStream_To_AP4_ByteStream_Adapter::MapResult(NPT_Result result)
{
    switch (result) {
        case NPT_ERROR_EOS: return AP4_ERROR_EOS;
        default: return result;
    }
}
