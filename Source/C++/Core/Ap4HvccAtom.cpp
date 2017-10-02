/*****************************************************************
|
|    AP4 - hvcC Atoms
|
|    Copyright 2002-2016 Axiomatic Systems, LLC
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
#include "Ap4HevcParser.h"

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
    m_LumaBitDepth(8),
    m_Reserved5(0),
    m_ChromaBitDepth(8),
    m_AverageFrameRate(0),
    m_ConstantFrameRate(0),
    m_NumTemporalLayers(0),
    m_TemporalIdNested(0),
    m_NaluLengthSize(4)
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
AP4_HvccAtom::AP4_HvccAtom(AP4_UI08                         general_profile_space,
                           AP4_UI08                         general_tier_flag,
                           AP4_UI08                         general_profile,
                           AP4_UI32                         general_profile_compatibility_flags,
                           AP4_UI64                         general_constraint_indicator_flags,
                           AP4_UI08                         general_level,
                           AP4_UI32                         min_spatial_segmentation,
                           AP4_UI08                         parallelism_type,
                           AP4_UI08                         chroma_format,
                           AP4_UI08                         luma_bit_depth,
                           AP4_UI08                         chroma_bit_depth,
                           AP4_UI16                         average_frame_rate,
                           AP4_UI08                         constant_frame_rate,
                           AP4_UI08                         num_temporal_layers,
                           AP4_UI08                         temporal_id_nested,
                           AP4_UI08                         nalu_length_size,
                           const AP4_Array<AP4_DataBuffer>& video_parameters,
                           AP4_UI08                         video_parameters_completeness,
                           const AP4_Array<AP4_DataBuffer>& sequence_parameters,
                           AP4_UI08                         sequence_parameters_completeness,
                           const AP4_Array<AP4_DataBuffer>& picture_parameters,
                           AP4_UI08                         picture_parameters_completeness) :
    AP4_Atom(AP4_ATOM_TYPE_HVCC, AP4_ATOM_HEADER_SIZE),
    m_ConfigurationVersion(1),
    m_GeneralProfileSpace(general_profile_space),
    m_GeneralTierFlag(general_tier_flag),
    m_GeneralProfile(general_profile),
    m_GeneralProfileCompatibilityFlags(general_profile_compatibility_flags),
    m_GeneralConstraintIndicatorFlags(general_constraint_indicator_flags),
    m_GeneralLevel(general_level),
    m_Reserved1(0),
    m_MinSpatialSegmentation(min_spatial_segmentation),
    m_Reserved2(0),
    m_ParallelismType(parallelism_type),
    m_Reserved3(0),
    m_ChromaFormat(chroma_format),
    m_Reserved4(0),
    m_LumaBitDepth(luma_bit_depth),
    m_Reserved5(0),
    m_ChromaBitDepth(chroma_bit_depth),
    m_AverageFrameRate(average_frame_rate),
    m_ConstantFrameRate(constant_frame_rate),
    m_NumTemporalLayers(num_temporal_layers),
    m_TemporalIdNested(temporal_id_nested),
    m_NaluLengthSize(nalu_length_size)
{
    // deep copy of the parameters
    AP4_HvccAtom::Sequence vps_sequence;
    vps_sequence.m_NaluType = AP4_HEVC_NALU_TYPE_VPS_NUT;
    vps_sequence.m_ArrayCompleteness = video_parameters_completeness;
    for (unsigned int i=0; i<video_parameters.ItemCount(); i++) {
        vps_sequence.m_Nalus.Append(video_parameters[i]);
    }
    if (vps_sequence.m_Nalus.ItemCount()) {
        m_Sequences.Append(vps_sequence);
    }
    
    AP4_HvccAtom::Sequence sps_sequence;
    sps_sequence.m_NaluType = AP4_HEVC_NALU_TYPE_SPS_NUT;
    sps_sequence.m_ArrayCompleteness = sequence_parameters_completeness;
    for (unsigned int i=0; i<sequence_parameters.ItemCount(); i++) {
        sps_sequence.m_Nalus.Append(sequence_parameters[i]);
    }
    if (sps_sequence.m_Nalus.ItemCount()) {
        m_Sequences.Append(sps_sequence);
    }

    AP4_HvccAtom::Sequence pps_sequence;
    pps_sequence.m_NaluType = AP4_HEVC_NALU_TYPE_PPS_NUT;
    pps_sequence.m_ArrayCompleteness = picture_parameters_completeness;
    for (unsigned int i=0; i<picture_parameters.ItemCount(); i++) {
        pps_sequence.m_Nalus.Append(picture_parameters[i]);
    }
    if (pps_sequence.m_Nalus.ItemCount()) {
        m_Sequences.Append(pps_sequence);
    }
    
    UpdateRawBytes();
    m_Size32 += m_RawBytes.GetDataSize();
}

/*----------------------------------------------------------------------
|   AP4_HvccAtom::AP4_HvccAtom
+---------------------------------------------------------------------*/
AP4_HvccAtom::AP4_HvccAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_HVCC, size)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;

    // keep a raw copy
    if (payload_size < 22) return;
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
    AP4_BitWriter bits(23);
    bits.Write(m_ConfigurationVersion, 8);
    bits.Write(m_GeneralProfileSpace, 2);
    bits.Write(m_GeneralTierFlag, 1);
    bits.Write(m_GeneralProfile, 5);
    bits.Write(m_GeneralProfileCompatibilityFlags, 32);
    bits.Write((AP4_UI32)(m_GeneralConstraintIndicatorFlags>>32), 16);
    bits.Write((AP4_UI32)(m_GeneralConstraintIndicatorFlags), 32);
    bits.Write(m_GeneralLevel, 8);
    bits.Write(0xFF, 4);
    bits.Write(m_MinSpatialSegmentation, 12);
    bits.Write(0xFF, 6);
    bits.Write(m_ParallelismType, 2);
    bits.Write(0xFF, 6);
    bits.Write(m_ChromaFormat, 2);
    bits.Write(0xFF, 5);
    bits.Write(m_LumaBitDepth >= 8 ? m_LumaBitDepth - 8 : 0, 3);
    bits.Write(0xFF, 5);
    bits.Write(m_ChromaBitDepth >= 8 ? m_ChromaBitDepth - 8 : 0, 3);
    bits.Write(m_AverageFrameRate, 16);
    bits.Write(m_ConstantFrameRate, 2);
    bits.Write(m_NumTemporalLayers, 3);
    bits.Write(m_TemporalIdNested, 1);
    bits.Write(m_NaluLengthSize > 0 ? m_NaluLengthSize - 1 : 0, 2);
    bits.Write(m_Sequences.ItemCount(), 8);
    
    m_RawBytes.SetData(bits.GetData(), 23);

    for (unsigned int i=0; i<m_Sequences.ItemCount(); i++) {
        AP4_UI08 bytes[3];
        bytes[0] = (m_Sequences[i].m_ArrayCompleteness ? (1<<7) : 0) | m_Sequences[i].m_NaluType;
        AP4_BytesFromUInt16BE(&bytes[1], m_Sequences[i].m_Nalus.ItemCount());
        m_RawBytes.AppendData(bytes, 3);
        
        for (unsigned int j=0; j<m_Sequences[i].m_Nalus.ItemCount(); j++) {
            AP4_UI08 size[2];
            AP4_BytesFromUInt16BE(&size[0], (AP4_UI16)m_Sequences[i].m_Nalus[j].GetDataSize());
            m_RawBytes.AppendData(size, 2);
            m_RawBytes.AppendData(m_Sequences[i].m_Nalus[j].GetData(), m_Sequences[i].m_Nalus[j].GetDataSize());
        }
    }
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
