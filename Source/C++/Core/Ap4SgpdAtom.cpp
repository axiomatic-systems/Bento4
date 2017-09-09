/*****************************************************************
|
|    AP4 - sbgp Atoms
|
|    Copyright 2002-2014 Axiomatic Systems, LLC
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
#include "Ap4SgpdAtom.h"
#include "Ap4Utils.h"
#include "Ap4ContainerAtom.h"
#include "Ap4AtomFactory.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SgpdAtom)


/*----------------------------------------------------------------------
|   AP4_SgpdAtom::Create
+---------------------------------------------------------------------*/
AP4_SgpdAtom*
AP4_SgpdAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_SgpdAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_SgpdAtom::~AP4_SgpdAtom
+---------------------------------------------------------------------*/
AP4_SgpdAtom::~AP4_SgpdAtom()
{
    for (AP4_List<AP4_DataBuffer>::Item* item = m_Entries.FirstItem();
                                         item;
                                         item = item->GetNext()) {
        delete item->GetData();
    }
}

/*----------------------------------------------------------------------
|   AP4_SgpdAtom::AP4_SgpdAtom
+---------------------------------------------------------------------*/
AP4_SgpdAtom::AP4_SgpdAtom(AP4_UI32         size,
                           AP4_UI08         version,
                           AP4_UI32         flags,
                           AP4_ByteStream&  stream) :
    AP4_Atom(AP4_ATOM_TYPE_SGPD, size, version, flags),
    m_GroupingType(0),
    m_DefaultLength(0)
{
    AP4_Size bytes_available = size-AP4_FULL_ATOM_HEADER_SIZE;
    stream.ReadUI32(m_GroupingType);
    bytes_available -= 4;
    if (version >= 1) {
        stream.ReadUI32(m_DefaultLength);
        bytes_available -= 4;
    }

    // read the number of entries
    AP4_UI32 entry_count = 0;
    AP4_Result result = stream.ReadUI32(entry_count);
    if (AP4_FAILED(result)) return;
    bytes_available -= 4;

    // read all entries
    for (unsigned int i=0; i<entry_count; i++) {
        AP4_UI32 description_length = m_DefaultLength;
        if (m_Version == 0) {
            // entry size unknown, read the whole thing
            description_length = bytes_available;
        } else {
            if (m_DefaultLength == 0) {
                description_length = stream.ReadUI32(description_length);
            }
        }
        if (description_length <= bytes_available) {
            AP4_DataBuffer* payload = new AP4_DataBuffer();
            if (description_length) {
                payload->SetDataSize(description_length);
                stream.Read(payload->UseData(), description_length);
            }
            m_Entries.Add(payload);
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_SgpdAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SgpdAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    result = stream.WriteUI32(m_GroupingType);
    if (AP4_FAILED(result)) return result;

    if (m_Version >= 1) {
        result = stream.WriteUI32(m_DefaultLength);
        if (AP4_FAILED(result)) return result;
    }
    
    // write the children
    result = stream.WriteUI32(m_Entries.ItemCount());
    if (AP4_FAILED(result)) return result;
    
    for (AP4_List<AP4_DataBuffer>::Item* item = m_Entries.FirstItem();
                                         item;
                                         item = item->GetNext()) {
        AP4_DataBuffer* entry = item->GetData();
        if (m_Version >= 1) {
            if (m_DefaultLength == 0) {
                stream.WriteUI32((AP4_UI32)entry->GetDataSize());
            }
        }
        result = stream.Write(entry->GetData(), entry->GetDataSize());
        if (AP4_FAILED(result)) {
            return result;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SgpdAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SgpdAtom::InspectFields(AP4_AtomInspector& inspector)
{
    char fourcc[5];
    AP4_FormatFourChars(fourcc, m_GroupingType);
    inspector.AddField("grouping_type", fourcc);
    if (m_Version >= 1) {
        inspector.AddField("default_length", m_DefaultLength);
    }
    inspector.AddField("entry_count", m_Entries.ItemCount());
    
    // inspect entries
    char header[32];
    unsigned int i=0;
    for (AP4_List<AP4_DataBuffer>::Item* item = m_Entries.FirstItem();
                                         item;
                                         item = item->GetNext()) {
        AP4_DataBuffer* entry = item->GetData();
        AP4_FormatString(header, sizeof(header), "entry %02d", i);
        ++i;
        inspector.AddField(header, entry->GetData(), entry->GetDataSize());
    }

    return AP4_SUCCESS;
}
