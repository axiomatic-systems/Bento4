/*****************************************************************
|
|    AP4 - Atomix Adapters
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
#include "Ap4AtomixAdapters.h"

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::ATX_InputStream_To_AP4_ByteStream_Adapter
+---------------------------------------------------------------------*/
ATX_InputStream_To_AP4_ByteStream_Adapter::ATX_InputStream_To_AP4_ByteStream_Adapter(ATX_InputStream* stream) :
    m_Stream(stream),
    m_ReferenceCount(1)
{
    ATX_REFERENCE_OBJECT(stream);
}

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::~ATX_InputStream_To_AP4_ByteStream_Adapter
+---------------------------------------------------------------------*/
ATX_InputStream_To_AP4_ByteStream_Adapter::~ATX_InputStream_To_AP4_ByteStream_Adapter()
{
    ATX_RELEASE_OBJECT(m_Stream);
}

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::MapResult
+---------------------------------------------------------------------*/
AP4_Result
ATX_InputStream_To_AP4_ByteStream_Adapter::MapResult(ATX_Result result)
{
    switch (result) {
        case ATX_ERROR_EOS: return AP4_ERROR_EOS;
        default: return result;
    }
}

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::AddReference
+---------------------------------------------------------------------*/
void
ATX_InputStream_To_AP4_ByteStream_Adapter::AddReference()
{
    ++m_ReferenceCount;
}

/*----------------------------------------------------------------------
|   DcfAtxToAp4StreamAdapter::Release
+---------------------------------------------------------------------*/
void
ATX_InputStream_To_AP4_ByteStream_Adapter::Release()
{
    if (--m_ReferenceCount == 0) {
        delete this;
    }
}

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::ReadPartial
+---------------------------------------------------------------------*/
AP4_Result
ATX_InputStream_To_AP4_ByteStream_Adapter::ReadPartial(void* buffer, AP4_Size bytes_to_read, AP4_Size& bytes_read)
{
    ATX_Result result = ATX_InputStream_Read(m_Stream, buffer, bytes_to_read, &bytes_read);
    return MapResult(result);
}

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::Write
+---------------------------------------------------------------------*/
AP4_Result
ATX_InputStream_To_AP4_ByteStream_Adapter::WritePartial(const void*, AP4_Size, AP4_Size&)
{
    return AP4_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::Seek
+---------------------------------------------------------------------*/
AP4_Result
ATX_InputStream_To_AP4_ByteStream_Adapter::Seek(AP4_Position position)
{
    return MapResult(ATX_InputStream_Seek(m_Stream, (ATX_Position)position));
}

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::Tell
+---------------------------------------------------------------------*/
AP4_Result
ATX_InputStream_To_AP4_ByteStream_Adapter::Tell(AP4_Position& position)
{
    return MapResult(ATX_InputStream_Tell(m_Stream, &position));
}

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter::GetSize
+---------------------------------------------------------------------*/
AP4_Result
ATX_InputStream_To_AP4_ByteStream_Adapter::GetSize(AP4_LargeSize& size)
{
    return MapResult(ATX_InputStream_GetSize(m_Stream, &size));
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_MapResult
+---------------------------------------------------------------------*/
static ATX_Result
AP4_ByteStream_To_ATX_InputStream_Adapter_MapResult(AP4_Result result)
{
    switch (result) {
        case AP4_ERROR_EOS:
            return ATX_ERROR_EOS;
            
        default:
            return result;
    }
} 

/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_Read
+---------------------------------------------------------------------*/
ATX_METHOD
AP4_ByteStream_To_ATX_InputStream_Adapter_Read(ATX_InputStream* _self, 
                                               ATX_Any          buffer,
                                               ATX_Size         bytes_to_read,
                                               ATX_Size*        bytes_read)
{
    AP4_ByteStream_To_ATX_InputStream_Adapter* self = ATX_SELF(AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_InputStream);
    
    AP4_Size local_read = 0;
    AP4_Result result = self->source->ReadPartial(buffer, bytes_to_read, local_read);
    if (bytes_read) *bytes_read = local_read;
    
    return AP4_ByteStream_To_ATX_InputStream_Adapter_MapResult(result);
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_Seek
+---------------------------------------------------------------------*/
ATX_METHOD
AP4_ByteStream_To_ATX_InputStream_Adapter_Seek(ATX_InputStream* _self, 
                                               ATX_Position     offset)
{
    AP4_ByteStream_To_ATX_InputStream_Adapter* self = ATX_SELF(AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_InputStream);

    AP4_Result result = self->source->Seek(offset);

    return AP4_ByteStream_To_ATX_InputStream_Adapter_MapResult(result);
}
                        
/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_Tell
+---------------------------------------------------------------------*/
ATX_METHOD
AP4_ByteStream_To_ATX_InputStream_Adapter_Tell(ATX_InputStream* _self, 
                                               ATX_Position* offset)
{
    AP4_ByteStream_To_ATX_InputStream_Adapter* self = ATX_SELF(AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_InputStream);

    AP4_Position source_position;
    AP4_Result result = self->source->Tell(source_position);
    *offset = (ATX_Position)source_position;
    return AP4_ByteStream_To_ATX_InputStream_Adapter_MapResult(result);
}
                              
/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_GetSize
+---------------------------------------------------------------------*/
ATX_METHOD
AP4_ByteStream_To_ATX_InputStream_Adapter_GetSize(ATX_InputStream* _self, 
                                                  ATX_LargeSize*   size)
{
    AP4_ByteStream_To_ATX_InputStream_Adapter* self = ATX_SELF(AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_InputStream);

    AP4_Result result = self->source->GetSize(*size);
    
    return AP4_ByteStream_To_ATX_InputStream_Adapter_MapResult(result);
}
                                
/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_GetAvailable
+---------------------------------------------------------------------*/
ATX_METHOD
AP4_ByteStream_To_ATX_InputStream_Adapter_GetAvailable(ATX_InputStream* _self, 
                                                       ATX_LargeSize*   available)
{
    AP4_ByteStream_To_ATX_InputStream_Adapter* self = ATX_SELF(AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_InputStream);
    
    /* default value */
    *available = 0;
    
    AP4_LargeSize source_size = 0;
    AP4_Position  source_position = 0;
    AP4_Result result = self->source->GetSize(source_size);
    if (AP4_FAILED(result)) return AP4_ByteStream_To_ATX_InputStream_Adapter_MapResult(result);
    result = self->source->Tell(source_position);
    if (AP4_FAILED(result)) return AP4_ByteStream_To_ATX_InputStream_Adapter_MapResult(result);
    
    *available = (ATX_LargeSize)(source_size-source_position);
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_Destroy
+---------------------------------------------------------------------*/
static ATX_Result
AP4_ByteStream_To_ATX_InputStream_Adapter_Destroy(AP4_ByteStream_To_ATX_InputStream_Adapter* self)
{
    /* release the reference to the source */
    if (self->source) self->source->Release();
    ATX_FreeMemory(self);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_GetInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AP4_ByteStream_To_ATX_InputStream_Adapter)
    ATX_GET_INTERFACE_ACCEPT(AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_InputStream)
    ATX_GET_INTERFACE_ACCEPT(AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_InputStream)
    AP4_ByteStream_To_ATX_InputStream_Adapter_Read,
    AP4_ByteStream_To_ATX_InputStream_Adapter_Seek,
    AP4_ByteStream_To_ATX_InputStream_Adapter_Tell,
    AP4_ByteStream_To_ATX_InputStream_Adapter_GetSize,
    AP4_ByteStream_To_ATX_InputStream_Adapter_GetAvailable
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(AP4_ByteStream_To_ATX_InputStream_Adapter, reference_count)

/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter_Create
+---------------------------------------------------------------------*/
ATX_Result
AP4_ByteStream_To_ATX_InputStream_Adapter_Create(AP4_ByteStream*   source_stream,
                                                 ATX_InputStream** object)
{
    /* check parameters */
    if (source_stream == NULL || object == NULL) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }
    
    /* allocate new object */
    AP4_ByteStream_To_ATX_InputStream_Adapter* stream;
    stream = (AP4_ByteStream_To_ATX_InputStream_Adapter*)ATX_AllocateZeroMemory(sizeof(AP4_ByteStream_To_ATX_InputStream_Adapter));
    if (stream == NULL) {
        *object = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* initialize the object */
    stream->reference_count = 1;
    
    /* keep a reference to the source stream */
    stream->source = source_stream;
    stream->source->AddReference();
    
    /* setup the interfaces */
    ATX_SET_INTERFACE(stream, AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_InputStream);
    ATX_SET_INTERFACE(stream, AP4_ByteStream_To_ATX_InputStream_Adapter, ATX_Referenceable);
    *object = &ATX_BASE(stream, ATX_InputStream);

    return ATX_SUCCESS;
}
