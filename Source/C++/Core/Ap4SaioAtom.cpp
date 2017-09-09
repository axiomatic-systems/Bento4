/*****************************************************************
|
|    AP4 - saio Atoms 
|
|    Copyright 2002-2011 Axiomatic Systems, LLC
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
#include "Ap4SaioAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SaioAtom)


/*----------------------------------------------------------------------
|   AP4_SaioAtom::Create
+---------------------------------------------------------------------*/
AP4_SaioAtom*
AP4_SaioAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_SaioAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_SaioAtom::AP4_SaioAtom
+---------------------------------------------------------------------*/
AP4_SaioAtom::AP4_SaioAtom() :
    AP4_Atom(AP4_ATOM_TYPE_SAIO, AP4_FULL_ATOM_HEADER_SIZE+4, 0, 0),
    m_AuxInfoType(0),
    m_AuxInfoTypeParameter(0)
{
}

/*----------------------------------------------------------------------
|   AP4_SaioAtom::AddEntry
+---------------------------------------------------------------------*/
AP4_Result           
AP4_SaioAtom::AddEntry(AP4_UI64 offset)
{
    m_Entries.Append(offset);
    unsigned int extra = (m_Flags&1)?8:0;
    SetSize(AP4_FULL_ATOM_HEADER_SIZE+extra+4+m_Entries.ItemCount()*(m_Version==0?4:8));
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SaioAtom::SetEntry
+---------------------------------------------------------------------*/
AP4_Result           
AP4_SaioAtom::SetEntry(AP4_Ordinal entry_index, AP4_UI64 offset)
{
    if (m_Entries.ItemCount() <= entry_index) return AP4_ERROR_OUT_OF_RANGE;
    m_Entries[entry_index] = offset;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SaioAtom::AP4_SaioAtom
+---------------------------------------------------------------------*/
AP4_SaioAtom::AP4_SaioAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_SAIO, size, version, flags),
    m_AuxInfoType(0),
    m_AuxInfoTypeParameter(0)
{
    AP4_UI32 remains = size-GetHeaderSize();
    if (flags & 1) {
        stream.ReadUI32(m_AuxInfoType);
        stream.ReadUI32(m_AuxInfoTypeParameter);
        remains -= 8;
    }
    AP4_UI32 entry_count = 0;
    AP4_Result result = stream.ReadUI32(entry_count);
    if (AP4_FAILED(result)) return;
    remains -= 4;
    if (remains < entry_count*(m_Version==0?4:8)) {
        return;
    }
    m_Entries.SetItemCount(entry_count);
    for (unsigned int i=0; i<entry_count; i++) {
        if (m_Version == 0) {
            AP4_UI32 entry = 0;
            result = stream.ReadUI32(entry);
            if (AP4_FAILED(result)) return;
            m_Entries[i] = entry;
        } else {
            AP4_UI64 entry = 0;
            result = stream.ReadUI64(entry);
            if (AP4_FAILED(result)) return;
            m_Entries[i] = entry;
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_SaioAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SaioAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    if (m_Flags&1) {
        result = stream.WriteUI32(m_AuxInfoType);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_AuxInfoTypeParameter);
        if (AP4_FAILED(result)) return result;
    }
    result = stream.WriteUI32(m_Entries.ItemCount());
    if (AP4_FAILED(result)) return result;
    for (unsigned int i=0; i<m_Entries.ItemCount(); i++) {
        if (m_Version == 0) {
            result = stream.WriteUI32((AP4_UI32)m_Entries[i]);
            if (AP4_FAILED(result)) return result;
        } else {
            result = stream.WriteUI64(m_Entries[i]);
            if (AP4_FAILED(result)) return result;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SaioAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SaioAtom::InspectFields(AP4_AtomInspector& inspector)
{
    if (m_Flags&1) {
        inspector.AddField("aux info type", m_AuxInfoType, AP4_AtomInspector::HINT_HEX);
        inspector.AddField("aux info type parameter", m_AuxInfoTypeParameter, AP4_AtomInspector::HINT_HEX);
    }
    inspector.AddField("entry count", m_Entries.ItemCount());

    if (inspector.GetVerbosity() >= 2) {
        char header[32];
        for (AP4_Ordinal i=0; i<m_Entries.ItemCount(); i++) {
            AP4_FormatString(header, sizeof(header), "entry %8d", i);
            inspector.AddField(header, m_Entries[i]);
        }
    }

    return AP4_SUCCESS;
}
