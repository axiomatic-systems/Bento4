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
#include "Atomix.h"
#include "Ap4ByteStream.h"

/*----------------------------------------------------------------------
|   ATX_InputStream_To_AP4_ByteStream_Adapter
+---------------------------------------------------------------------*/
class ATX_InputStream_To_AP4_ByteStream_Adapter : public AP4_ByteStream {
public:
    ATX_InputStream_To_AP4_ByteStream_Adapter(ATX_InputStream* stream);
    virtual ~ATX_InputStream_To_AP4_ByteStream_Adapter();

    // AP4_ByteStream methods
    virtual AP4_Result ReadPartial(void*     buffer, 
                                   AP4_Size  bytes_to_read, 
                                   AP4_Size& bytes_read);
    virtual AP4_Result WritePartial(const void* buffer, 
                                    AP4_Size    bytes_to_write, 
                                    AP4_Size&   bytes_written);
    virtual AP4_Result Seek(AP4_Position position);
    virtual AP4_Result Tell(AP4_Position& position);
    virtual AP4_Result GetSize(AP4_LargeSize& size);

    // AP4_Referenceable methods
    virtual void AddReference();
    virtual void Release();

private:
    // methods
    AP4_Result MapResult(ATX_Result result);

    // members
    ATX_InputStream* m_Stream;
    ATX_Cardinal     m_ReferenceCount;
};

/*----------------------------------------------------------------------
|   AP4_ByteStream_To_ATX_InputStream_Adapter
+---------------------------------------------------------------------*/
// it is important to keep this structure a POD (no methods)
// because the strict compilers will not like use using
// the offsetof() macro necessary when using ATX_SELF()
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_InputStream);
    ATX_IMPLEMENTS(ATX_Referenceable);
    
    /* members */
    ATX_Cardinal    reference_count;
    AP4_ByteStream* source;
} AP4_ByteStream_To_ATX_InputStream_Adapter;

ATX_Result
AP4_ByteStream_To_ATX_InputStream_Adapter_Create(AP4_ByteStream*   source_stream,
                                                 ATX_InputStream** object);
