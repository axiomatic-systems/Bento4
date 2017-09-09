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
#include "Ap4SbgpAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SbgpAtom)


/*----------------------------------------------------------------------
|   AP4_SbgpAtom::Create
+---------------------------------------------------------------------*/
AP4_SbgpAtom*
AP4_SbgpAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_SbgpAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_SbgpAtom::AP4_SbgpAtom
+---------------------------------------------------------------------*/
AP4_SbgpAtom::AP4_SbgpAtom() :
    AP4_Atom(AP4_ATOM_TYPE_SBGP, AP4_FULL_ATOM_HEADER_SIZE+8, 0, 0),
    m_GroupingType(0),
    m_GroupingTypeParameter(0)
{
}

/*----------------------------------------------------------------------
|   AP4_SbgpAtom::AP4_SbgpAtom
+---------------------------------------------------------------------*/
AP4_SbgpAtom::AP4_SbgpAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_SBGP, size, version, flags),
    m_GroupingType(0),
    m_GroupingTypeParameter(0)
{
    AP4_UI32 remains = size-GetHeaderSize();
    stream.ReadUI32(m_GroupingType);
    remains -= 4;
    if (version >= 1) {
        stream.ReadUI32(m_GroupingTypeParameter);
        remains -= 4;
    }
    AP4_UI32 entry_count = 0;
    AP4_Result result = stream.ReadUI32(entry_count);
    if (AP4_FAILED(result)) return;
    remains -= 4;
    if (remains < entry_count*8) {
        return;
    }
    m_Entries.SetItemCount(entry_count);
    for (unsigned int i=0; i<entry_count; i++) {
        Entry entry;
        stream.ReadUI32(entry.sample_count);
        stream.ReadUI32(entry.group_description_index);
        m_Entries[i] = entry;
    }
}

/*----------------------------------------------------------------------
|   AP4_SbgpAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SbgpAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    result = stream.WriteUI32(m_GroupingType);
    if (AP4_FAILED(result)) return result;

    if (m_Version >= 1) {
        result = stream.WriteUI32(m_GroupingTypeParameter);
        if (AP4_FAILED(result)) return result;
    }
    
    result = stream.WriteUI32(m_Entries.ItemCount());
    if (AP4_FAILED(result)) return result;
    for (unsigned int i=0; i<m_Entries.ItemCount(); i++) {
        result = stream.WriteUI32(m_Entries[i].sample_count);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_Entries[i].group_description_index);
        if (AP4_FAILED(result)) return result;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SbgpAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SbgpAtom::InspectFields(AP4_AtomInspector& inspector)
{
    char fourcc[5];
    AP4_FormatFourChars(fourcc, m_GroupingType);
    inspector.AddField("grouping_type", fourcc);
    if (m_Version >= 1) {
        inspector.AddField("grouping_type_parameter", m_GroupingTypeParameter);
    }
    inspector.AddField("entry_count", m_Entries.ItemCount());

    if (inspector.GetVerbosity() >= 2) {
        char header[32];
        char value[128];
        for (AP4_Ordinal i=0; i<m_Entries.ItemCount(); i++) {
            AP4_FormatString(header, sizeof(header), "entry %02d", i);
            AP4_FormatString(value, sizeof(value), "c:%u,g:%u", m_Entries[i].sample_count, m_Entries[i].group_description_index);
            inspector.AddField(header, value);
        }
    }

    return AP4_SUCCESS;
}
