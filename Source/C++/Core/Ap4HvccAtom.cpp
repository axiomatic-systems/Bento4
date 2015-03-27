/*****************************************************************
|
|    AP4 - hvcC Atoms
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
#include "Ap4HvccAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_HvccAtom)

/*----------------------------------------------------------------------
|   AP4_HvccAtom::GetProfileName
+---------------------------------------------------------------------*/
const char*
AP4_HvccAtom::GetProfileName(AP4_UI08 profile_space, AP4_UI08 profile)
{
    if (profile_space != 0) {
        return NULL;
    }
    switch (profile) {
        case AP4_HEVC_PROFILE_MAIN:               return "Main";
        case AP4_HEVC_PROFILE_MAIN_10:            return "Main 10";
        case AP4_HEVC_PROFILE_MAIN_STILL_PICTURE: return "Main Still Picture";
        case AP4_HEVC_PROFILE_REXT:               return "Rext";
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_HvccAtom::GetChromaFormatName
+---------------------------------------------------------------------*/
const char*
AP4_HvccAtom::GetChromaFormatName(AP4_UI08 chroma_format)
{
    switch (chroma_format) {
        case AP4_HEVC_CHROMA_FORMAT_MONOCHROME: return "Monochrome";
        case AP4_HEVC_CHROMA_FORMAT_420:        return "4:2:0";
        case AP4_HEVC_CHROMA_FORMAT_422:        return "4:2:2";
        case AP4_HEVC_CHROMA_FORMAT_444:        return "4:4:4";
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_AvccAtom::Create
+---------------------------------------------------------------------*/
AP4_HvccAtom*
AP4_HvccAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;
    
    return new AP4_HvccAtom(size, payload_data.GetData());
}

/*----------------------------------------------------------------------
|   AP4_HvccAtom::AP4_HvccAtom
+---------------------------------------------------------------------*/
AP4_HvccAtom::AP4_HvccAtom() :
    AP4_Atom(AP4_ATOM_TYPE_HVCC, AP4_ATOM_HEADER_SIZE),
    m_ConfigurationVersion(1),
    m_GeneralProfileSpace(0),
    m_GeneralTierFlag(0),
    m_GeneralProfile(0),
    m_GeneralProfileCompatibilityFlags(0),
    m_GeneralConstraintIndicatorFlags(0),
    m_GeneralLevel(0),
    m_Reserved1(0),
    m_MinSpatialSegmentation(0),
    m_Reserved2(0),
    m_ParallelismType(0),
    m_Reserved3(0),
    m_ChromaFormat(0),
    m_Reserved4(0),
    m_LumaBitDepth(0),
    m_Reserved5(0),
    m_ChromaBitDepth(0),
    m_AverageFrameRate(0),
    m_ConstantFrameRate(0),
    m_NumTemporalLayers(0),
    m_TemporalIdNested(0),
    m_NaluLengthSize(0)
{
    UpdateRawBytes();
    m_Size32 += m_RawBytes.GetDataSize();
}

/*----------------------------------------------------------------------
|   AP4_HvccAtom::AP4_HvccAtom
+---------------------------------------------------------------------*/
AP4_HvccAtom::AP4_HvccAtom(const AP4_HvccAtom& other) :
    AP4_Atom(AP4_ATOM_TYPE_HVCC, other.m_Size32),
    m_ConfigurationVersion(other.m_ConfigurationVersion),
    m_GeneralProfileSpace(other.m_GeneralProfileSpace),
    m_GeneralTierFlag(other.m_GeneralTierFlag),
    m_GeneralProfile(other.m_GeneralProfile),
    m_GeneralProfileCompatibilityFlags(other.m_GeneralProfileCompatibilityFlags),
    m_GeneralConstraintIndicatorFlags(other.m_GeneralConstraintIndicatorFlags),
    m_GeneralLevel(other.m_GeneralLevel),
    m_Reserved1(other.m_Reserved1),
    m_MinSpatialSegmentation(other.m_MinSpatialSegmentation),
    m_Reserved2(other.m_Reserved2),
    m_ParallelismType(other.m_ParallelismType),
    m_Reserved3(other.m_Reserved3),
    m_ChromaFormat(other.m_ChromaFormat),
    m_Reserved4(other.m_Reserved4),
    m_LumaBitDepth(other.m_LumaBitDepth),
    m_Reserved5(other.m_Reserved5),
    m_ChromaBitDepth(other.m_ChromaBitDepth),
    m_AverageFrameRate(other.m_AverageFrameRate),
    m_ConstantFrameRate(other.m_ConstantFrameRate),
    m_NumTemporalLayers(other.m_NumTemporalLayers),
    m_TemporalIdNested(other.m_TemporalIdNested),
    m_NaluLengthSize(other.m_NaluLengthSize),
    m_RawBytes(other.m_RawBytes)
{
    // deep copy of the parameters
    unsigned int i = 0;
    for (i=0; i<other.m_Sequences.ItemCount(); i++) {
        m_Sequences.Append(other.m_Sequences[i]);
    }
}

/*----------------------------------------------------------------------
|   AP4_HvccAtom::AP4_HvccAtom
+---------------------------------------------------------------------*/
AP4_HvccAtom::AP4_HvccAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_HVCC, size)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    m_RawBytes.SetData(payload, payload_size);

    // parse the payload
    m_ConfigurationVersion   = payload[0];
    m_GeneralProfileSpace    = (payload[1]>>6) & 0x03;
    m_GeneralTierFlag        = (payload[1]>>5) & 0x01;
    m_GeneralProfile         = (payload[1]   ) & 0x1F;
    m_GeneralProfileCompatibilityFlags = AP4_BytesToUInt32BE(&payload[2]);
    m_GeneralConstraintIndicatorFlags  = (((AP4_UI64)AP4_BytesToUInt32BE(&payload[6]))<<16) | AP4_BytesToUInt16BE(&payload[10]);
    m_GeneralLevel           = payload[12];
    m_Reserved1              = (payload[13]>>4) & 0x0F;
    m_MinSpatialSegmentation = AP4_BytesToUInt16BE(&payload[13]) & 0x0FFF;
    m_Reserved2              = (payload[15]>>2) & 0x3F;
    m_ParallelismType        = payload[15] & 0x03;
    m_Reserved3              = (payload[16]>>2) & 0x3F;
    m_ChromaFormat           = payload[16] & 0x03;
    m_Reserved4              = (payload[17]>>3) & 0x1F;
    m_LumaBitDepth           = 8+(payload[17] & 0x07);
    m_Reserved5              = (payload[18]>>3) & 0x1F;
    m_ChromaBitDepth         = 8+(payload[18] & 0x07);
    m_AverageFrameRate       = AP4_BytesToUInt16BE(&payload[19]);
    m_ConstantFrameRate      = (payload[21]>>6) & 0x03;
    m_NumTemporalLayers      = (payload[21]>>3) & 0x07;
    m_TemporalIdNested       = (payload[21]>>2) & 0x01;
    m_NaluLengthSize         = 1+(payload[21] & 0x03);
    
    AP4_UI08 num_seq = payload[22];
    m_Sequences.SetItemCount(num_seq);
    unsigned int cursor = 23;
    for (unsigned int i=0; i<num_seq; i++) {

        Sequence& seq = m_Sequences[i];
        if (cursor+1 > payload_size) break;
        seq.m_ArrayCompleteness = (payload[cursor] >> 7) & 0x01;
        seq.m_Reserved          = (payload[cursor] >> 6) & 0x01;
        seq.m_NaluType          = payload[cursor] & 0x3F;
        cursor += 1;
        
        if (cursor+2 > payload_size) break;
        AP4_UI16 nalu_count = AP4_BytesToUInt16BE(&payload[cursor]);
        seq.m_Nalus.SetItemCount(nalu_count);
        cursor += 2;
        
        for (unsigned int j=0; j<nalu_count; j++) {
            if (cursor+2 > payload_size) break;
            unsigned int nalu_length = AP4_BytesToUInt16BE(&payload[cursor]);
            cursor += 2;
            if (cursor + nalu_length > payload_size) break;
            seq.m_Nalus[j].SetData(&payload[cursor], nalu_length);
            cursor += nalu_length;
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_HvccAtom::UpdateRawBytes
+---------------------------------------------------------------------*/
void
AP4_HvccAtom::UpdateRawBytes()
{
    // FIXME: not implemented yet
}

/*----------------------------------------------------------------------
|   AP4_HvccAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_HvccAtom::WriteFields(AP4_ByteStream& stream)
{
    return stream.Write(m_RawBytes.GetData(), m_RawBytes.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_HvccAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_HvccAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("Configuration Version", m_ConfigurationVersion);
    inspector.AddField("Profile Space", m_GeneralProfileSpace);
    const char* profile_name = GetProfileName(m_GeneralProfileSpace, m_GeneralProfile);
    if (profile_name) {
        inspector.AddField("Profile", profile_name);
    } else {
        inspector.AddField("Profile", m_GeneralProfile);
    }
    inspector.AddField("Tier", m_GeneralTierFlag);
    inspector.AddField("Profile Compatibility", m_GeneralProfileCompatibilityFlags, AP4_AtomInspector::HINT_HEX);
    inspector.AddField("Constraint", m_GeneralConstraintIndicatorFlags, AP4_AtomInspector::HINT_HEX);
    inspector.AddField("Level", m_GeneralLevel);
    inspector.AddField("Min Spatial Segmentation", m_MinSpatialSegmentation);
    inspector.AddField("Parallelism Type", m_ParallelismType);
    inspector.AddField("Chroma Format", m_ChromaFormat);
    inspector.AddField("Chroma Depth", m_ChromaBitDepth);
    inspector.AddField("Luma Depth", m_LumaBitDepth);
    inspector.AddField("Average Frame Rate", m_AverageFrameRate);
    inspector.AddField("Constant Frame Rate", m_ConstantFrameRate);
    inspector.AddField("Number Of Temporal Layers", m_NumTemporalLayers);
    inspector.AddField("Temporal Id Nested", m_TemporalIdNested);
    inspector.AddField("NALU Length Size", m_NaluLengthSize);
    return AP4_SUCCESS;
}
