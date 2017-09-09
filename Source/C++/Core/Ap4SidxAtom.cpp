/*****************************************************************
|
|    AP4 - sidx Atoms
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
#include "Ap4SidxAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SidxAtom)


/*----------------------------------------------------------------------
|   AP4_SidxAtom::Create
+---------------------------------------------------------------------*/
AP4_SidxAtom*
AP4_SidxAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_SidxAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_SidxAtom::AP4_SidxAtom
+---------------------------------------------------------------------*/
AP4_SidxAtom::AP4_SidxAtom(AP4_UI32 reference_id,
                           AP4_UI32 timescale,
                           AP4_UI64 earliest_presentation_time,
                           AP4_UI64 first_offset) :
    AP4_Atom(AP4_ATOM_TYPE_SIDX, AP4_FULL_ATOM_HEADER_SIZE+20, 0, 0),
    m_ReferenceId(reference_id),
    m_TimeScale(timescale),
    m_EarliestPresentationTime(earliest_presentation_time),
    m_FirstOffset(first_offset)
{
    if ((earliest_presentation_time >> 32) || (first_offset >> 32)) {
        m_Version = 1;
        m_Size32 += 8;
    }
}

/*----------------------------------------------------------------------
|   AP4_SidxAtom::AP4_SidxAtom
+---------------------------------------------------------------------*/
AP4_SidxAtom::AP4_SidxAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_SIDX, size, version, flags)
{
    stream.ReadUI32(m_ReferenceId);
    stream.ReadUI32(m_TimeScale);
    if (version == 0) {
        AP4_UI32 earliest_presentation_time = 0;
        AP4_UI32 first_offset = 0;
        stream.ReadUI32(earliest_presentation_time);
        stream.ReadUI32(first_offset);
        m_EarliestPresentationTime = earliest_presentation_time;
        m_FirstOffset              = first_offset;
    } else {
        stream.ReadUI64(m_EarliestPresentationTime);
        stream.ReadUI64(m_FirstOffset);
    }
    AP4_UI16 reserved;
    stream.ReadUI16(reserved);
    AP4_UI16 reference_count = 0;
    stream.ReadUI16(reference_count);
    if (size < AP4_FULL_ATOM_HEADER_SIZE+4+4+(version==0?8:16)+2+2+reference_count*12) {
        // not enough space to store all references, something's wrong
        return;
    }
    m_References.SetItemCount(reference_count);
    for (unsigned int i=0; i<reference_count; i++) {
        AP4_UI32 value = 0;
        stream.ReadUI32(value);
        m_References[i].m_ReferenceType = (value&(1<<31))?1:0;
        m_References[i].m_ReferencedSize = value & 0x7FFFFFFF;
        stream.ReadUI32(m_References[i].m_SubsegmentDuration);
        stream.ReadUI32(value);
        m_References[i].m_StartsWithSap = ((value&(1<<31)) != 0);
        m_References[i].m_SapType       = (AP4_UI08)((value >>28)&0x07);
        m_References[i].m_SapDeltaTime  = value & 0x0FFFFFFF;
    }
}

/*----------------------------------------------------------------------
|   AP4_SidxAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SidxAtom::WriteFields(AP4_ByteStream& stream)
{
    stream.WriteUI32(m_ReferenceId);
    stream.WriteUI32(m_TimeScale);
    if (m_Version == 0) {
        stream.WriteUI32((AP4_UI32)m_EarliestPresentationTime);
        stream.WriteUI32((AP4_UI32)m_FirstOffset);
    } else {
        stream.WriteUI64(m_EarliestPresentationTime);
        stream.WriteUI64(m_FirstOffset);
    }
    stream.WriteUI16(0);
    stream.WriteUI16((AP4_UI16)m_References.ItemCount());
    for (unsigned int i=0; i<m_References.ItemCount(); i++) {
        stream.WriteUI32((m_References[i].m_ReferenceType<<31) |
                          m_References[i].m_ReferencedSize);
        stream.WriteUI32(m_References[i].m_SubsegmentDuration);
        stream.WriteUI32(((m_References[i].m_StartsWithSap?1:0)<<31) |
                          (m_References[i].m_SapType << 28) |
                           m_References[i].m_SapDeltaTime);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SidxAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SidxAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("reference_ID", m_ReferenceId);
    inspector.AddField("timescale", m_TimeScale);
    inspector.AddField("earliest_presentation_time", m_EarliestPresentationTime);
    inspector.AddField("first_offset", m_FirstOffset);

    if (inspector.GetVerbosity() >= 1) {
        AP4_UI32 reference_count = m_References.ItemCount();
        for (unsigned int i=0; i<reference_count; i++) {
            char header[32];
            AP4_FormatString(header, sizeof(header), "entry %04d", i);
            char value[256];
            AP4_FormatString(value, sizeof(value), "reference_type=%d, referenced_size=%u, subsegment_duration=%u, starts_with_SAP=%d, SAP_type=%d, SAP_delta_time=%d",
                             m_References[i].m_ReferenceType,
                             m_References[i].m_ReferencedSize,
                             m_References[i].m_SubsegmentDuration,
                             m_References[i].m_StartsWithSap,
                             m_References[i].m_SapType,
                             m_References[i].m_SapDeltaTime);
            inspector.AddField(header, value);
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SidxAtom::SetReferenceCount
+---------------------------------------------------------------------*/
void
AP4_SidxAtom::SetReferenceCount(unsigned int count) {
    m_Size32 -= m_References.ItemCount()*12;
    m_References.SetItemCount(count);
    m_Size32 += m_References.ItemCount()*12;
}
