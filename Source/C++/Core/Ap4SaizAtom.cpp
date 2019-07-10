/*****************************************************************
|
|    AP4 - saiz Atoms 
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
#include "Ap4Atom.h"
#include "Ap4SaizAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SaizAtom)

/*----------------------------------------------------------------------
|   AP4_SaizAtom::Create
+---------------------------------------------------------------------*/
AP4_SaizAtom*
AP4_SaizAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 0) return NULL;
    return new AP4_SaizAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_SaizAtom::AP4_SaizAtom
+---------------------------------------------------------------------*/
AP4_SaizAtom::AP4_SaizAtom() :
    AP4_Atom(AP4_ATOM_TYPE_SAIZ, AP4_FULL_ATOM_HEADER_SIZE+1+4, 0, 0),
    m_AuxInfoType(0),
    m_AuxInfoTypeParameter(0),
    m_DefaultSampleInfoSize(0),
    m_SampleCount(0)
{
}

/*----------------------------------------------------------------------
|   AP4_SaizAtom::AP4_SaizAtom
+---------------------------------------------------------------------*/
AP4_SaizAtom::AP4_SaizAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_SAIZ, size, version, flags),
    m_AuxInfoType(0),
    m_AuxInfoTypeParameter(0)
{
    AP4_UI32 remains = size-GetHeaderSize();
    if (flags & 1) {
        stream.ReadUI32(m_AuxInfoType);
        stream.ReadUI32(m_AuxInfoTypeParameter);
        remains -= 8;
    }
    stream.ReadUI08(m_DefaultSampleInfoSize);
    stream.ReadUI32(m_SampleCount);
    remains -= 5;
    if (m_DefaultSampleInfoSize == 0) { 
        // means that the sample info entries  have different sizes
        if (m_SampleCount > remains) m_SampleCount = remains; // sanity check
        AP4_Cardinal sample_count = m_SampleCount;
        m_Entries.SetItemCount(sample_count);
        unsigned char* buffer = new AP4_UI08[sample_count];
        AP4_Result result = stream.Read(buffer, sample_count);
        if (AP4_FAILED(result)) {
            delete[] buffer;
            return;
        }
        for (unsigned int i=0; i<sample_count; i++) {
            m_Entries[i] = buffer[i];
        }
        delete[] buffer;
    }
}

/*----------------------------------------------------------------------
|   AP4_SaizAtom::SetSampleCount
+---------------------------------------------------------------------*/
AP4_Result
AP4_SaizAtom::SetSampleCount(AP4_UI32 sample_count)
{
    m_SampleCount = sample_count;
    unsigned int extra = (m_Flags&1)?8:0;
    if (m_DefaultSampleInfoSize == 0) {
        if (sample_count) {
            m_Entries.SetItemCount(sample_count);
        } else {
            m_Entries.Clear();
        }
        SetSize(AP4_FULL_ATOM_HEADER_SIZE+extra+1+4+sample_count);
    } else {
        SetSize(AP4_FULL_ATOM_HEADER_SIZE+extra+1+4);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SaizAtom::SetDefaultSampleInfoSize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_SaizAtom::SetDefaultSampleInfoSize(AP4_UI08 sample_info_size)
{
    m_DefaultSampleInfoSize = sample_info_size;
    m_Entries.Clear();
    unsigned int extra = (m_Flags&1)?8:0;
    SetSize(AP4_FULL_ATOM_HEADER_SIZE+extra+1+4);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SaizAtom::GetSampleInfoSize
+---------------------------------------------------------------------*/
AP4_Result
AP4_SaizAtom::GetSampleInfoSize(AP4_Ordinal sample, AP4_UI08& sample_info_size)
{
    if (m_DefaultSampleInfoSize) {
        sample_info_size = m_DefaultSampleInfoSize;
    } else {
        // check the sample index
        if (sample >= m_SampleCount) {
            sample_info_size = 0;
            return AP4_ERROR_OUT_OF_RANGE;
        } else {
            sample_info_size = m_Entries[sample];
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SaizAtom::SetSampleInfoSize
+---------------------------------------------------------------------*/
AP4_Result
AP4_SaizAtom::SetSampleInfoSize(AP4_Ordinal sample, AP4_UI08 sample_info_size)
{
    // check the sample index
    if (sample >= m_SampleCount) {
        return AP4_ERROR_OUT_OF_RANGE;
    } else {
        if (m_DefaultSampleInfoSize) {
            if (m_DefaultSampleInfoSize != sample_info_size) {
                return AP4_ERROR_INVALID_STATE;
            }
        } else {
            m_Entries[sample] = sample_info_size;
        }
        return AP4_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   AP4_SaizAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SaizAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    if (m_Flags&1) {
        result = stream.WriteUI32(m_AuxInfoType);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_AuxInfoTypeParameter);
        if (AP4_FAILED(result)) return result;
    }
    result = stream.WriteUI08(m_DefaultSampleInfoSize);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(m_SampleCount);
    if (AP4_FAILED(result)) return result;
    
    if (m_DefaultSampleInfoSize == 0) {
        for (AP4_UI32 i=0; i<m_SampleCount; i++) {
            result = stream.WriteUI08(m_Entries[i]);
            if (AP4_FAILED(result)) return result;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_SaizAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SaizAtom::InspectFields(AP4_AtomInspector& inspector)
{
    if (m_Flags&1) {
        inspector.AddField("aux info type", m_AuxInfoType, AP4_AtomInspector::HINT_HEX);
        inspector.AddField("aux info type parameter", m_AuxInfoTypeParameter, AP4_AtomInspector::HINT_HEX);
    }
    inspector.AddField("default sample info size", m_DefaultSampleInfoSize);
    inspector.AddField("sample count", m_SampleCount);

    if (inspector.GetVerbosity() >= 2) {
        char header[32];
        for (AP4_Ordinal i=0; i<m_Entries.ItemCount(); i++) {
            AP4_FormatString(header, sizeof(header), "entry %8d", i);
            inspector.AddField(header, m_Entries[i]);
        }
    }

    return AP4_SUCCESS;
}
