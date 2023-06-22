/*****************************************************************
|
|    AP4 - trun Atoms 
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
#include "Ap4TrunAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_TrunAtom)

/*----------------------------------------------------------------------
|   AP4_TrunAtom::Create
+---------------------------------------------------------------------*/
AP4_TrunAtom*
AP4_TrunAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_TrunAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::ComputeRecordFieldsCount
+---------------------------------------------------------------------*/
unsigned int 
AP4_TrunAtom::ComputeRecordFieldsCount(AP4_UI32 flags)
{
    // count the number of bits set to 1 in the second byte of the flags
    unsigned int count = 0;
    for (unsigned int i=0; i<8; i++) {
        if (flags & (1<<(i+8))) ++count;
    }
    return count;
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::ComputeOptionalFieldsCount
+---------------------------------------------------------------------*/
unsigned int 
AP4_TrunAtom::ComputeOptionalFieldsCount(AP4_UI32 flags)
{
    // count the number of bits set to 1 in the LSB of the flags
    unsigned int count = 0;
    for (unsigned int i=0; i<8; i++) {
        if (flags & (1<<i)) ++count;
    }
    return count;
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::AP4_TrunAtom
+---------------------------------------------------------------------*/
AP4_TrunAtom::AP4_TrunAtom(AP4_UI32 flags, 
                           AP4_SI32 data_offset,
                           AP4_UI32 first_sample_flags) :
    AP4_Atom(AP4_ATOM_TYPE_TRUN, AP4_FULL_ATOM_HEADER_SIZE+4, 0, flags),
    m_DataOffset(data_offset),
    m_FirstSampleFlags(first_sample_flags)
{
    m_Size32 += 4 * ComputeOptionalFieldsCount(flags);
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::AP4_TrunAtom
+---------------------------------------------------------------------*/
AP4_TrunAtom::AP4_TrunAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_TRUN, size, version, flags)
{
    if (size < AP4_FULL_ATOM_HEADER_SIZE + 4) {
        return;
    }
    AP4_UI32 sample_count = 0;
    stream.ReadUI32(sample_count);
    AP4_Size bytes_left = size - AP4_FULL_ATOM_HEADER_SIZE - 4;

    // read optional fields
    int optional_fields_count = (int)ComputeOptionalFieldsCount(flags);
    if (flags & AP4_TRUN_FLAG_DATA_OFFSET_PRESENT) {
        AP4_UI32 offset = 0;
        if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(offset))) {
            return;
        }
        m_DataOffset = (AP4_SI32)offset;
        if (optional_fields_count == 0) {
            return;
        }
        --optional_fields_count;
        bytes_left -= 4;
    }
    if (flags & AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT) {
        if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_FirstSampleFlags))) {
            return;
        }
        if (optional_fields_count == 0) {
            return;
        }
        --optional_fields_count;
        bytes_left -= 4;
    }
    
    // discard unknown optional fields 
    for (int i=0; i<optional_fields_count; i++) {
        AP4_UI32 discard;
        if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(discard))) {
            return;
        }
        bytes_left -= 4;
    }
    
    int record_fields_count = (int)ComputeRecordFieldsCount(flags);
    if (record_fields_count && ((bytes_left / (record_fields_count*4)) < sample_count)) {
        // not enough data for all samples, the format is invalid
        return;
    }
    if (AP4_FAILED(m_Entries.SetItemCount(sample_count))) {
        return;
    }
    for (unsigned int i=0; i<sample_count; i++) {
        if (flags & AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT) {
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_Entries[i].sample_duration))) {
                return;;
            }
            --record_fields_count;
            bytes_left -= 4;
        }
        if (flags & AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT) {
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_Entries[i].sample_size))) {
                return;
            }
            --record_fields_count;
            bytes_left -= 4;
        }
        if (flags & AP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT) {
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_Entries[i].sample_flags))) {
                return;
            }
            --record_fields_count;
            bytes_left -= 4;
        }
        if (flags & AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT) {
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_Entries[i].sample_composition_time_offset))) {
                return;
            }
            --record_fields_count;
            bytes_left -= 4;
        }
    
        // skip unknown fields 
        for (int j=0;j<record_fields_count; j++) {
            AP4_UI32 discard;
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(discard))) {
                return;
            }
            bytes_left -= 4;
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::UpdateFlags
+---------------------------------------------------------------------*/
void
AP4_TrunAtom::UpdateFlags(AP4_UI32 flags)
{
    m_Flags  = flags;
    m_Size32 = AP4_FULL_ATOM_HEADER_SIZE + 4 + 4 * ComputeOptionalFieldsCount(flags);
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::SetEntries
+---------------------------------------------------------------------*/
AP4_Result
AP4_TrunAtom::SetEntries(const AP4_Array<Entry>& entries)
{
    // resize the entry array
    m_Entries.SetItemCount(entries.ItemCount());
    
    // copy entries
    AP4_Cardinal entry_count = entries.ItemCount();
    for (unsigned int i=0; i<entry_count; i++) {
        m_Entries[i] = entries[i];
    }
    
    // update the atom size
    unsigned int record_fields_count = ComputeRecordFieldsCount(m_Flags);
    m_Size32 += entries.ItemCount()*record_fields_count*4;
    
    // notify the parent
    if (m_Parent) m_Parent->OnChildChanged(this);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TrunAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    result = stream.WriteUI32(m_Entries.ItemCount());
    if (AP4_FAILED(result)) return result;
    if (m_Flags & AP4_TRUN_FLAG_DATA_OFFSET_PRESENT) {
        result = stream.WriteUI32((AP4_UI32)m_DataOffset);
        if (AP4_FAILED(result)) return result;
    }
    if (m_Flags & AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT) {
        result = stream.WriteUI32(m_FirstSampleFlags);
        if (AP4_FAILED(result)) return result;
    }
    AP4_UI32 sample_count = m_Entries.ItemCount();
    for (unsigned int i=0; i<sample_count; i++) {
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT) {
            result = stream.WriteUI32(m_Entries[i].sample_duration);
            if (AP4_FAILED(result)) return result;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT) {
            result = stream.WriteUI32(m_Entries[i].sample_size);
            if (AP4_FAILED(result)) return result;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT) {
            result = stream.WriteUI32(m_Entries[i].sample_flags);
            if (AP4_FAILED(result)) return result;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT) {
            stream.WriteUI32(m_Entries[i].sample_composition_time_offset);
            if (AP4_FAILED(result)) return result;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TrunAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("sample count", m_Entries.ItemCount());
    if (m_Flags & AP4_TRUN_FLAG_DATA_OFFSET_PRESENT) {
        inspector.AddField("data offset", m_DataOffset);
    }
    if (m_Flags & AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT) {
        inspector.AddField("first sample flags", m_FirstSampleFlags, AP4_AtomInspector::HINT_HEX);
    }
    if (inspector.GetVerbosity() > 0) {
        inspector.StartArray("entries");

        AP4_UI32 sample_count = m_Entries.ItemCount();
        for (unsigned int i = 0; i < sample_count; i++) {
            inspector.StartObject(NULL, 0, true);

            if (m_Flags & AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT) {
                inspector.AddField(inspector.GetVerbosity() >= 2 ? "sample_duration" : "d",
                                   m_Entries[i].sample_duration);
            }
            if (m_Flags & AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT) {
                inspector.AddField(inspector.GetVerbosity() >= 2 ? "sample_size" : "s",
                                   m_Entries[i].sample_size);
            }
            if (m_Flags & AP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT) {
                inspector.AddField(inspector.GetVerbosity() >= 2 ? "sample_flags" : "f",
                                   m_Entries[i].sample_flags);
            }
            if (m_Flags & AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT) {
                inspector.AddField(inspector.GetVerbosity() >= 2 ? "sample_composition_time_offset" : "c",
                                   m_Entries[i].sample_composition_time_offset);
            }

            inspector.EndObject();
        }

        inspector.EndArray();
    }
    
    return AP4_SUCCESS;
}
