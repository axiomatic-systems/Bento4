/*****************************************************************
|
|    AP4 - stz2 Atoms 
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
#include "Ap4Stz2Atom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_Stz2Atom)

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::Create
+---------------------------------------------------------------------*/
AP4_Stz2Atom*
AP4_Stz2Atom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version != 0) return NULL;
    return new AP4_Stz2Atom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::AP4_Stz2Atom
+---------------------------------------------------------------------*/
AP4_Stz2Atom::AP4_Stz2Atom(AP4_UI08 field_size) :
    AP4_Atom(AP4_ATOM_TYPE_STZ2, AP4_FULL_ATOM_HEADER_SIZE+8, 0, 0),
    m_FieldSize(field_size),
    m_SampleCount(0)
{
    if (m_FieldSize != 4 && m_FieldSize != 8 && m_FieldSize != 16) {
        m_FieldSize = 16;
    }
}

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::AP4_Stz2Atom
+---------------------------------------------------------------------*/
AP4_Stz2Atom::AP4_Stz2Atom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_STZ2, size, version, flags)
{
    AP4_UI08 reserved;
    stream.ReadUI08(reserved);
    stream.ReadUI08(reserved);
    stream.ReadUI08(reserved);
    stream.ReadUI08(m_FieldSize);
    stream.ReadUI32(m_SampleCount);
    if (m_FieldSize != 4 && m_FieldSize != 8 && m_FieldSize != 16) {
        // illegale field size
        return;
    }

    AP4_Cardinal sample_count = m_SampleCount;
    m_Entries.SetItemCount(sample_count);
    unsigned int table_size = (sample_count*m_FieldSize+7)/8;
    if ((table_size+8) > size) return;
    unsigned char* buffer = new unsigned char[table_size];
    AP4_Result result = stream.Read(buffer, table_size);
    if (AP4_FAILED(result)) {
        delete[] buffer;
        return;
    }
    switch (m_FieldSize) {
        case 4:
            for (unsigned int i=0; i<sample_count; i++) {
                if ((i%2) == 0) {
                    m_Entries[i] = (buffer[i/2]>>4)&0x0F;
                } else {
                    m_Entries[i] = buffer[i/2]&0x0F;
                }
            }
            break;

        case 8:
            for (unsigned int i=0; i<sample_count; i++) {
                m_Entries[i] = buffer[i];
            }
            break;

        case 16:
            for (unsigned int i=0; i<sample_count; i++) {
                m_Entries[i] = AP4_BytesToUInt16BE(&buffer[i*2]);
            }
            break;
    }
    delete[] buffer;
}

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Stz2Atom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    // sample size
    result = stream.WriteUI08(0);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(0);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(0);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(m_FieldSize);
    if (AP4_FAILED(result)) return result;

    // sample count
    result = stream.WriteUI32(m_SampleCount);
    if (AP4_FAILED(result)) return result;

    // write the sample sizes
    switch (m_FieldSize) {
        case 4:
            for (AP4_UI32 i=0; i<m_SampleCount; i+=2) {
                if (i+1 < m_SampleCount) {
                    result = stream.WriteUI08(((m_Entries[i]&0x0F)<<4) | (m_Entries[i+1]&0x0F));
                } else {
                    result = stream.WriteUI08((m_Entries[i]&0x0F)<<4);
                }
                if (AP4_FAILED(result)) return result;
            }
            break;

        case 8:
            for (AP4_UI32 i=0; i<m_SampleCount; i++) {
                result = stream.WriteUI08((AP4_UI08)m_Entries[i]);
                if (AP4_FAILED(result)) return result;
            }
            break;

        case 16:
            for (AP4_UI32 i=0; i<m_SampleCount; i++) {
                result = stream.WriteUI16((AP4_UI16)m_Entries[i]);
                if (AP4_FAILED(result)) return result;
            }
            break;
    }

    return result;
}

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::GetSampleCount
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Stz2Atom::GetSampleCount()
{
    return m_SampleCount;
}

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::GetSampleSize
+---------------------------------------------------------------------*/
AP4_Result
AP4_Stz2Atom::GetSampleSize(AP4_Ordinal sample, AP4_Size& sample_size)
{
    // check the sample index
    if (sample > m_SampleCount || sample == 0) {
        sample_size = 0;
        return AP4_ERROR_OUT_OF_RANGE;
    } else {
        sample_size = m_Entries[sample - 1];
        return AP4_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::SetSampleSize
+---------------------------------------------------------------------*/
AP4_Result
AP4_Stz2Atom::SetSampleSize(AP4_Ordinal sample, AP4_Size sample_size)
{
    // check the sample index
    if (sample > m_SampleCount || sample == 0) {
        return AP4_ERROR_OUT_OF_RANGE;
    } else {
        m_Entries[sample - 1] = sample_size;
        return AP4_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::AddEntry
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Stz2Atom::AddEntry(AP4_UI32 size)
{
    m_Entries.Append(size);
    m_SampleCount++;
    if (m_FieldSize == 4) {
        if ((m_SampleCount%2) == 1) {
            m_Size32++;
        }
    } else {
        m_Size32 += m_FieldSize/8;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Stz2Atom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Stz2Atom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("field_size", m_FieldSize);
    inspector.AddField("sample_count", m_Entries.ItemCount());

    if (inspector.GetVerbosity() >= 2) {
        char header[32];
        for (AP4_Ordinal i=0; i<m_Entries.ItemCount(); i++) {
            AP4_FormatString(header, sizeof(header), "entry %8d", i);
            inspector.AddField(header, m_Entries[i]);
        }
    }

    return AP4_SUCCESS;
}
