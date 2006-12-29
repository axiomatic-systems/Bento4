/*****************************************************************
|
|    AP4 - Atom Based Sample Tables
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf
|
|
|    This atom is part of AP4 (MP4 Audio Proatom Library).
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
|    along with Bento4|GPL; see the atom COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4AtomSampleTable.h"
#include "Ap4ByteStream.h"
#include "Ap4StsdAtom.h"
#include "Ap4StscAtom.h"
#include "Ap4StcoAtom.h"
#include "Ap4Co64Atom.h"
#include "Ap4StszAtom.h"
#include "Ap4SttsAtom.h"
#include "Ap4CttsAtom.h"
#include "Ap4StssAtom.h"
#include "Ap4Sample.h"
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::AP4_AtomSampleTable
+---------------------------------------------------------------------*/
AP4_AtomSampleTable::AP4_AtomSampleTable(AP4_ContainerAtom* stbl, 
                                         AP4_ByteStream&    sample_stream) :
    m_SampleStream(sample_stream)
{
    m_StscAtom = dynamic_cast<AP4_StscAtom*>(stbl->GetChild(AP4_ATOM_TYPE_STSC));
    m_StcoAtom = dynamic_cast<AP4_StcoAtom*>(stbl->GetChild(AP4_ATOM_TYPE_STCO));
    m_StszAtom = dynamic_cast<AP4_StszAtom*>(stbl->GetChild(AP4_ATOM_TYPE_STSZ));
    m_CttsAtom = dynamic_cast<AP4_CttsAtom*>(stbl->GetChild(AP4_ATOM_TYPE_CTTS));
    m_SttsAtom = dynamic_cast<AP4_SttsAtom*>(stbl->GetChild(AP4_ATOM_TYPE_STTS));
    m_StssAtom = dynamic_cast<AP4_StssAtom*>(stbl->GetChild(AP4_ATOM_TYPE_STSS));
    m_StsdAtom = dynamic_cast<AP4_StsdAtom*>(stbl->GetChild(AP4_ATOM_TYPE_STSD));
    m_Co64Atom = dynamic_cast<AP4_Co64Atom*>(stbl->GetChild(AP4_ATOM_TYPE_CO64));

    // keep a reference to the sample stream
    m_SampleStream.AddReference();
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::~AP4_AtomSampleTable
+---------------------------------------------------------------------*/
AP4_AtomSampleTable::~AP4_AtomSampleTable()
{
    m_SampleStream.Release();
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::GetSample
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomSampleTable::GetSample(AP4_Ordinal index, 
                               AP4_Sample& sample)
{
    AP4_Result result;

    // check that we have a chunk offset table
    if (m_StcoAtom == NULL && m_Co64Atom == NULL) {
        return AP4_ERROR_INVALID_FORMAT;
    }

    // MP4 uses 1-based indexes internally, so adjust by one
    index++;

    // find out in which chunk this sample is located
    AP4_Ordinal chunk, skip, desc;
    result = m_StscAtom->GetChunkForSample(index, chunk, skip, desc);
    if (AP4_FAILED(result)) return result;
    
    // check that the result is within bounds
    if (skip > index) return AP4_ERROR_INTERNAL;

    // get the atom offset for this chunk
    AP4_UI64 offset;
    if (m_StcoAtom) {
        AP4_UI32 offset_32;
        result = m_StcoAtom->GetChunkOffset(chunk, offset_32);
        offset = offset_32;
    } else {
        result = m_Co64Atom->GetChunkOffset(chunk, offset);
    }
    if (AP4_FAILED(result)) return result;
    
    // compute the additional offset inside the chunk
    for (unsigned int i = index-skip; i < index; i++) {
        AP4_Size size;
        result = m_StszAtom->GetSampleSize(i, size); 
        if (AP4_FAILED(result)) return result;
        offset += size;
    }

    // set the description index
    sample.SetDescriptionIndex(desc-1); // adjust for 0-based indexes

    // set the dts and cts
    AP4_UI32      cts_offset;
    AP4_TimeStamp dts;
    result = m_SttsAtom->GetDts(index, dts);
    if (AP4_FAILED(result)) return result;
    sample.SetDts(dts);
    if (m_CttsAtom == NULL) {
        sample.SetCts(dts);
    } else {
        result = m_CttsAtom->GetCtsOffset(index, cts_offset); 
	    if (AP4_FAILED(result)) return result;
        sample.SetCts(dts + cts_offset);
    }     

    // set the size
    AP4_Size sample_size;
    result = m_StszAtom->GetSampleSize(index, sample_size);
    if (AP4_FAILED(result)) return result;
    sample.SetSize(sample_size);

    // set the offset
    sample.SetOffset(offset);

    // set the data stream
    sample.SetDataStream(m_SampleStream);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::GetSampleCount
+---------------------------------------------------------------------*/
AP4_Cardinal
AP4_AtomSampleTable::GetSampleCount()
{
    return m_StszAtom ? m_StszAtom->GetSampleCount() : 0;
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::GetSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_AtomSampleTable::GetSampleDescription(AP4_Ordinal index)
{
    return m_StsdAtom ? m_StsdAtom->GetSampleDescription(index) : NULL;
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::GetSampleDescriptionCount
+---------------------------------------------------------------------*/
AP4_Cardinal
AP4_AtomSampleTable::GetSampleDescriptionCount()
{
    return m_StsdAtom ? m_StsdAtom->GetSampleDescriptionCount() : 0;
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::GetChunkForSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_AtomSampleTable::GetChunkForSample(AP4_Ordinal  sample,
                                       AP4_Ordinal& chunk,
                                       AP4_Ordinal& skip,
                                       AP4_Ordinal& sample_description)
{
    return m_StscAtom ? m_StscAtom->GetChunkForSample(sample, chunk, skip, sample_description) : AP4_FAILURE;
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::GetChunkOffset
+---------------------------------------------------------------------*/
AP4_Result 
AP4_AtomSampleTable::GetChunkOffset(AP4_Ordinal chunk, AP4_Position& offset)
{
    if (m_StcoAtom) {
        AP4_UI32 offset_32;
        AP4_Result result = m_StcoAtom->GetChunkOffset(chunk, offset_32);
        if (AP4_SUCCEEDED(result)) {
            offset = offset_32;
        } else {
            offset = 0;
        }
        return result;
    } else if (m_Co64Atom) {
        return m_Co64Atom->GetChunkOffset(chunk, offset);
    } else {
        offset = 0;
        return AP4_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::SetChunkOffset
+---------------------------------------------------------------------*/
AP4_Result 
AP4_AtomSampleTable::SetChunkOffset(AP4_Ordinal chunk, AP4_Position offset)
{
    if (m_StcoAtom) {
        if ((offset >> 32) != 0) return AP4_ERROR_OUT_OF_RANGE;
        return m_StcoAtom->SetChunkOffset(chunk, (AP4_UI32)offset);
    } else if (m_Co64Atom) {
        return m_Co64Atom->SetChunkOffset(chunk, offset);
    } else {
        return AP4_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::SetSampleSize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_AtomSampleTable::SetSampleSize(AP4_Ordinal sample, AP4_Size size)
{
    return m_StszAtom ? m_StszAtom->SetSampleSize(sample, size) : AP4_FAILURE;
}

/*----------------------------------------------------------------------
|   AP4_AtomSampleTable::GetSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_AtomSampleTable::GetSampleIndexForTimeStamp(AP4_TimeStamp ts, 
                                                AP4_Ordinal& index)
{
    return m_SttsAtom ? m_SttsAtom->GetSampleIndexForTimeStamp(ts, index) 
                      : AP4_FAILURE;
}
