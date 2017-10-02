/*****************************************************************
|
|    AP4 - Sample Description Objects
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
#include "Ap4SampleDescription.h"
#include "Ap4EsDescriptor.h"
#include "Ap4SLConfigDescriptor.h"
#include "Ap4SampleEntry.h"
#include "Ap4AvccAtom.h"
#include "Ap4HvccAtom.h"
#include "Ap4Utils.h"
#include "Ap4Mp4AudioInfo.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_UnknownSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_AudioSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_VideoSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_GenericAudioSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_GenericVideoSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_MpegSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_MpegAudioSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_MpegVideoSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_MpegSystemSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_AvcSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_HevcSampleDescription)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SubtitleSampleDescription)

/*----------------------------------------------------------------------
|  AP4_GetFormatName
+---------------------------------------------------------------------*/
const char*
AP4_GetFormatName(AP4_UI32 format)
{
    switch (format) {
        case AP4_SAMPLE_FORMAT_MP4A: return "MPEG-4 Audio";
        case AP4_SAMPLE_FORMAT_MP4V: return "MPEG-4 Video";
        case AP4_SAMPLE_FORMAT_MP4S: return "MPEG-4 Systems";
        case AP4_SAMPLE_FORMAT_ALAC: return "Apple Lossless Audio";
        case AP4_SAMPLE_FORMAT_AVC1: return "H.264";
        case AP4_SAMPLE_FORMAT_AVC2: return "H.264";
        case AP4_SAMPLE_FORMAT_AVC3: return "H.264";
        case AP4_SAMPLE_FORMAT_AVC4: return "H.264";
        case AP4_SAMPLE_FORMAT_DVAV: return "Dolby Vision (H.264)";
        case AP4_SAMPLE_FORMAT_DVA1: return "Dolby Vision (H.264)";
        case AP4_SAMPLE_FORMAT_HEV1: return "H.265";
        case AP4_SAMPLE_FORMAT_HVC1: return "H.265";
        case AP4_SAMPLE_FORMAT_DVH1: return "Dolby Vision (H.265)";
        case AP4_SAMPLE_FORMAT_DVHE: return "Dolby Vision (H.265)";
        case AP4_SAMPLE_FORMAT_OVC1: return "VC-1";
        case AP4_SAMPLE_FORMAT_OWMA: return "WMA";
        case AP4_SAMPLE_FORMAT_AC_3: return "Dolby Digital (AC-3)";
        case AP4_SAMPLE_FORMAT_EC_3: return "Dolby Digital Plus (Enhanced AC-3)";
        case AP4_SAMPLE_FORMAT_DTSC: return "DTS";
        case AP4_SAMPLE_FORMAT_DTSH: return "DTS-HD";
        case AP4_SAMPLE_FORMAT_DTSL: return "DTS-HD Lossless";
        case AP4_SAMPLE_FORMAT_DTSE: return "DTS Low Bitrate";
        case AP4_SAMPLE_FORMAT_AVCP: return "Advanced Video Coding Parameters";
        case AP4_SAMPLE_FORMAT_DRAC: return "Dirac";
        case AP4_SAMPLE_FORMAT_DRA1: return "DRA Audio";
        case AP4_SAMPLE_FORMAT_G726: return "G726";
        case AP4_SAMPLE_FORMAT_MJP2: return "Motion JPEG 2000";
        case AP4_SAMPLE_FORMAT_OKSD: return "OMA Keys";
        case AP4_SAMPLE_FORMAT_RAW_: return "Uncompressed Audio";
        case AP4_SAMPLE_FORMAT_RTP_: return "RTP Hints";
        case AP4_SAMPLE_FORMAT_S263: return "H.263";
        case AP4_SAMPLE_FORMAT_SAMR: return "Narrowband AMR";
        case AP4_SAMPLE_FORMAT_SAWB: return "Wideband AMR";
        case AP4_SAMPLE_FORMAT_SAWP: return "Extended AMR";
        case AP4_SAMPLE_FORMAT_SEVC: return "EVRC Voice";
        case AP4_SAMPLE_FORMAT_SQCP: return "13K Voice";
        case AP4_SAMPLE_FORMAT_SRTP: return "SRTP Hints";
        case AP4_SAMPLE_FORMAT_SSMV: return "SMV Voice";
        case AP4_SAMPLE_FORMAT_TEXT: return "Textual Metadata";
        case AP4_SAMPLE_FORMAT_TWOS: return "Uncompressed 16-bit Audio";
        case AP4_SAMPLE_FORMAT_TX3G: return "Timed Text";
        case AP4_SAMPLE_FORMAT_VC_1: return "SMPTE VC-1";
        case AP4_SAMPLE_FORMAT_XML_: return "XML Metadata";
        case AP4_SAMPLE_FORMAT_STPP: return "Timed Text";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_SampleDescription::AP4_SampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription::AP4_SampleDescription(Type            type,
                                             AP4_UI32        format,
                                             AP4_AtomParent* details) :
    m_Type(type), m_Format(format)
{
    if (details) {
        for (AP4_List<AP4_Atom>::Item* item = details->GetChildren().FirstItem();
             item;
             item = item->GetNext()) {
            AP4_Atom* atom = item->GetData();
            if (atom) {
                AP4_Atom* clone = atom->Clone();
                if (clone) m_Details.AddChild(clone);
            }
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_SampleDescription::Clone
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_SampleDescription::Clone(AP4_Result* result)
{
    if (result) *result = AP4_SUCCESS;
    AP4_Atom* atom = ToAtom();
    if (atom == NULL) {
        if (result) *result = AP4_FAILURE;
        return NULL;
    }
    
    // serialize the atom to a buffer
    AP4_MemoryByteStream* mbs = new AP4_MemoryByteStream((AP4_UI32)atom->GetSize());
    atom->Write(*mbs);
    delete atom;
    atom = NULL;
    
    // parse the buffer back to an atom
    mbs->Seek(0);
    AP4_AtomFactory* factory = new AP4_AtomFactory();
    factory->PushContext(AP4_ATOM_TYPE_STSD);
    AP4_Atom* atom_clone = NULL;
    AP4_Result lresult = factory->CreateAtomFromStream(*mbs, atom_clone);
    factory->PopContext();
    delete factory;
    if (result) *result = lresult;
    mbs->Release();
    if (AP4_FAILED(lresult)) return NULL;
    
    // convert the atom clone to a sample description
    AP4_SampleEntry* sample_entry = AP4_DYNAMIC_CAST(AP4_SampleEntry, atom_clone);
    if (sample_entry == NULL) {
        if (result) *result = AP4_ERROR_INTERNAL;
        delete atom_clone;
        return NULL;
    }
    
    AP4_SampleDescription* clone = sample_entry->ToSampleDescription();
    if (clone == NULL) {
        if (result) *result = AP4_ERROR_INTERNAL;
    }
    
    delete atom_clone;
    return clone;
}

/*----------------------------------------------------------------------
|   AP4_SampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_SampleDescription::ToAtom() const
{
    return new AP4_SampleEntry(m_Format);
}

/*----------------------------------------------------------------------
|   AP4_SampleDescription::GetCodecString
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleDescription::GetCodecString(AP4_String& codec)
{
    char coding[5];
    AP4_FormatFourChars(coding, m_Format);
    codec.Assign(coding, 4);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleDescription::AP4_UnknownSampleDescription
+---------------------------------------------------------------------*/
AP4_UnknownSampleDescription::AP4_UnknownSampleDescription(AP4_Atom* atom) :
    AP4_SampleDescription(AP4_SampleDescription::TYPE_UNKNOWN, 
                          atom->GetType(), 
                          NULL),
    m_Atom(atom->Clone())
{
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleDescription::~AP4_UnknownSampleDescription
+---------------------------------------------------------------------*/
AP4_UnknownSampleDescription::~AP4_UnknownSampleDescription()
{
    delete m_Atom;
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleDescription::Clone
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_UnknownSampleDescription::Clone(AP4_Result* result)
{
    AP4_Atom* atom_clone = NULL;
    if (m_Atom) {
        atom_clone = m_Atom->Clone();
        if (atom_clone == NULL) {
            if (result) *result = AP4_FAILURE;
            return NULL;
        }
    }
    if (result) *result = AP4_SUCCESS;
    return new AP4_UnknownSampleDescription(atom_clone);
}

/*----------------------------------------------------------------------
|   AP4_UnknownSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom* 
AP4_UnknownSampleDescription::ToAtom() const
{
    if (m_Atom) {
        return m_Atom->Clone();
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_GenericAudioSampleDescription
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_GenericAudioSampleDescription::ToAtom() const
{
    AP4_AudioSampleEntry* sample_entry = new AP4_AudioSampleEntry(m_Format,
                                                                  m_SampleRate<<16,
                                                                  m_SampleSize,
                                                                  m_ChannelCount);
    AP4_AtomParent& details = const_cast<AP4_AtomParent&>(m_Details);
    for (AP4_List<AP4_Atom>::Item* item = details.GetChildren().FirstItem();
                                   item;
                                   item = item->GetNext()) {
        AP4_Atom* child = item->GetData();
        sample_entry->AddChild(child->Clone());
    }
    return sample_entry;
}

/*----------------------------------------------------------------------
|   AP4_GenericVideoSampleDescription
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_GenericVideoSampleDescription::ToAtom() const
{
    AP4_VisualSampleEntry* sample_entry = new AP4_VisualSampleEntry(m_Format,
                                                                    m_Width,
                                                                    m_Height,
                                                                    m_Depth,
                                                                    m_CompressorName.GetChars());
    AP4_AtomParent& details = const_cast<AP4_AtomParent&>(m_Details);
    for (AP4_List<AP4_Atom>::Item* item = details.GetChildren().FirstItem();
                                   item;
                                   item = item->GetNext()) {
        AP4_Atom* child = item->GetData();
        sample_entry->AddChild(child->Clone());
    }
    return sample_entry;
}

/*----------------------------------------------------------------------
|   AP4_AvcSampleDescription::AP4_AvcSampleDescription
+---------------------------------------------------------------------*/
AP4_AvcSampleDescription::AP4_AvcSampleDescription(AP4_UI32     format,
                                                   AP4_UI16     width,
                                                   AP4_UI16     height,
                                                   AP4_UI16     depth,
                                                   const char*  compressor_name,
                                                   AP4_UI08     profile,
                                                   AP4_UI08     level,
                                                   AP4_UI08     profile_compatibility,
                                                   AP4_UI08     length_size,
                                                   const AP4_Array<AP4_DataBuffer>& sequence_parameters,
                                                   const AP4_Array<AP4_DataBuffer>& picture_parameters) :
    AP4_SampleDescription(TYPE_AVC, format, NULL),
    AP4_VideoSampleDescription(width, height, depth, compressor_name)
{
    m_AvccAtom = new AP4_AvccAtom(profile, 
                                  level, 
                                  profile_compatibility,
                                  length_size,
                                  sequence_parameters,
                                  picture_parameters);
    m_Details.AddChild(m_AvccAtom);
}

/*----------------------------------------------------------------------
|   AP4_AvcSampleDescription::AP4_AvcSampleDescription
+---------------------------------------------------------------------*/
AP4_AvcSampleDescription::AP4_AvcSampleDescription(AP4_UI32            format,
                                                   AP4_UI16            width,
                                                   AP4_UI16            height,
                                                   AP4_UI16            depth,
                                                   const char*         compressor_name,
                                                   const AP4_AvccAtom* avcc) :
    AP4_SampleDescription(TYPE_AVC, format, NULL),
    AP4_VideoSampleDescription(width, height, depth, compressor_name)
{
    if (avcc) {
        m_AvccAtom = new AP4_AvccAtom(*avcc);
    } else {
        // should never happen
        m_AvccAtom = new AP4_AvccAtom();
    }
    m_Details.AddChild(m_AvccAtom);
}

/*----------------------------------------------------------------------
|   AP4_AvcSampleDescription::AP4_AvcSampleDescription
+---------------------------------------------------------------------*/
AP4_AvcSampleDescription::AP4_AvcSampleDescription(AP4_UI32        format,
                                                   AP4_UI16        width,
                                                   AP4_UI16        height,
                                                   AP4_UI16        depth,
                                                   const char*     compressor_name,
                                                   AP4_AtomParent* details) :
    AP4_SampleDescription(TYPE_AVC, format, details),
    AP4_VideoSampleDescription(width, height, depth, compressor_name),
    m_AvccAtom(NULL)
{
    AP4_AvccAtom* avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, m_Details.GetChild(AP4_ATOM_TYPE_AVCC));
    if (avcc) {
        m_AvccAtom = avcc;
    } else {
        // shoud never happen
        m_AvccAtom = new AP4_AvccAtom();
        m_Details.AddChild(m_AvccAtom);
    }
}

/*----------------------------------------------------------------------
|   AP4_AvcSampleDescription::GetCodecString
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcSampleDescription::GetCodecString(AP4_String& codec) {
    char coding[5];
    AP4_FormatFourChars(coding, GetFormat());
    char workspace[64];
    AP4_FormatString(workspace,
                     sizeof(workspace),
                     "%s.%02X%02X%02X",
                     coding,
                     GetProfile(),
                     GetProfileCompatibility(),
                     GetLevel());
    codec = workspace;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AvcSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_AvcSampleDescription::ToAtom() const
{
    return new AP4_AvcSampleEntry(m_Format,
                                  m_Width,
                                  m_Height,
                                  m_Depth,
                                  m_CompressorName.GetChars(),
                                  &m_Details);
}

/*----------------------------------------------------------------------
|   AP4_HevcSampleDescription::AP4_HevcSampleDescription
+---------------------------------------------------------------------*/
AP4_HevcSampleDescription::AP4_HevcSampleDescription(AP4_UI32            format,
                                                     AP4_UI16            width,
                                                     AP4_UI16            height,
                                                     AP4_UI16            depth,
                                                     const char*         compressor_name,
                                                     const AP4_HvccAtom* hvcc) :
    AP4_SampleDescription(TYPE_HEVC, format, NULL),
    AP4_VideoSampleDescription(width, height, depth, compressor_name)
{
    if (hvcc) {
        m_HvccAtom = new AP4_HvccAtom(*hvcc);
    } else {
        // should never happen
        m_HvccAtom = new AP4_HvccAtom();
    }
    m_Details.AddChild(m_HvccAtom);
}

/*----------------------------------------------------------------------
|   AP4_HevcSampleDescription::AP4_HevcSampleDescription
+---------------------------------------------------------------------*/
AP4_HevcSampleDescription::AP4_HevcSampleDescription(AP4_UI32        format,
                                                     AP4_UI16        width,
                                                     AP4_UI16        height,
                                                     AP4_UI16        depth,
                                                     const char*     compressor_name,
                                                     AP4_AtomParent* details) :
    AP4_SampleDescription(TYPE_HEVC, format, details),
    AP4_VideoSampleDescription(width, height, depth, compressor_name),
    m_HvccAtom(NULL)
{
    AP4_HvccAtom* hvcc = AP4_DYNAMIC_CAST(AP4_HvccAtom, m_Details.GetChild(AP4_ATOM_TYPE_HVCC));
    if (hvcc) {
        m_HvccAtom = hvcc;
    } else {
        // shoud never happen
        m_HvccAtom = new AP4_HvccAtom();
        m_Details.AddChild(m_HvccAtom);
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcSampleDescription::AP4_HevcSampleDescription
+---------------------------------------------------------------------*/
AP4_HevcSampleDescription::AP4_HevcSampleDescription(AP4_UI32                         format,
                                                     AP4_UI16                         width,
                                                     AP4_UI16                         height,
                                                     AP4_UI16                         depth,
                                                     const char*                      compressor_name,
                                                     AP4_UI08                         general_profile_space,
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
    AP4_SampleDescription(TYPE_HEVC, format, NULL),
    AP4_VideoSampleDescription(width, height, depth, compressor_name)
{
    m_HvccAtom = new AP4_HvccAtom(general_profile_space,
                                  general_tier_flag,
                                  general_profile,
                                  general_profile_compatibility_flags,
                                  general_constraint_indicator_flags,
                                  general_level,
                                  min_spatial_segmentation,
                                  parallelism_type,
                                  chroma_format,
                                  luma_bit_depth,
                                  chroma_bit_depth,
                                  average_frame_rate,
                                  constant_frame_rate,
                                  num_temporal_layers,
                                  temporal_id_nested,
                                  nalu_length_size,
                                  video_parameters,
                                  video_parameters_completeness,
                                  sequence_parameters,
                                  sequence_parameters_completeness,
                                  picture_parameters,
                                  picture_parameters_completeness);
    m_Details.AddChild(m_HvccAtom);
}

/*----------------------------------------------------------------------
|   ReverseBits
+---------------------------------------------------------------------*/
static AP4_UI32
ReverseBits(AP4_UI32 bits)
{
    unsigned int count = sizeof(bits) * 8;
    AP4_UI32 reverse_bits = 0;
     
    while (bits) {
       reverse_bits = (reverse_bits << 1) | (bits & 1);
       bits >>= 1;
       count--;
    }
    return (count < 32) ? (reverse_bits << count) : 0;
}

/*----------------------------------------------------------------------
|   AP4_HevcSampleDescription:GetCodecString
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcSampleDescription::GetCodecString(AP4_String& codec) {
    char coding[5];
    AP4_FormatFourChars(coding, GetFormat());
    char profile_space[2] = {0,0};
    if (GetGeneralProfileSpace() > 0 && GetGeneralProfileSpace() <= 3) {
        profile_space[0] = 'A'+GetGeneralProfileSpace()-1;
    }
    AP4_UI64 constraints = GetGeneralConstraintIndicatorFlags();
    while (constraints && ((constraints & 0xFF) == 0)) {
        constraints >>= 8;
    }
    char workspace[64];
    AP4_FormatString(workspace,
                     sizeof(workspace),
                     "%s.%s%d.%X.%c%d.%llx",
                     coding,
                     profile_space,
                     GetGeneralProfile(),
                     ReverseBits(GetGeneralProfileCompatibilityFlags()),
                     GetGeneralTierFlag()?'H':'L',
                     GetGeneralLevel(),
                     constraints);
    codec = workspace;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_HevcSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_HevcSampleDescription::ToAtom() const
{
    return new AP4_HevcSampleEntry(m_Format,
                                   m_Width,
                                   m_Height,
                                   m_Depth,
                                   m_CompressorName.GetChars(),
                                   &m_Details);
}

/*----------------------------------------------------------------------
|   AP4_MpegSampleDescription::AP4_MpegSampleDescription
+---------------------------------------------------------------------*/
AP4_MpegSampleDescription::AP4_MpegSampleDescription(AP4_UI32      format,
                                                     AP4_EsdsAtom* esds) :
    AP4_SampleDescription(TYPE_MPEG, format, NULL),
    m_StreamType(0),
    m_ObjectTypeId(0),
    m_BufferSize(0),
    m_MaxBitrate(0),
    m_AvgBitrate(0)
{
    if (esds) {
        // get the es descriptor
        const AP4_EsDescriptor* es_desc = esds->GetEsDescriptor();
        if (es_desc == NULL) return;

        // get the decoder config descriptor
        const AP4_DecoderConfigDescriptor* dc_desc =
            es_desc->GetDecoderConfigDescriptor();
        if (dc_desc) {
            m_StreamType   = dc_desc->GetStreamType();
            m_ObjectTypeId = dc_desc->GetObjectTypeIndication();
            m_BufferSize   = dc_desc->GetBufferSize();
            m_MaxBitrate   = dc_desc->GetMaxBitrate();
            m_AvgBitrate   = dc_desc->GetAvgBitrate();
            const AP4_DecoderSpecificInfoDescriptor* dsi_desc =
                dc_desc->GetDecoderSpecificInfoDescriptor();
            if (dsi_desc != NULL) {
                m_DecoderInfo.SetData(dsi_desc->GetDecoderSpecificInfo().GetData(),
                                      dsi_desc->GetDecoderSpecificInfo().GetDataSize());
            }
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_MpegSampleDescription::AP4_MpegSampleDescription
+---------------------------------------------------------------------*/
AP4_MpegSampleDescription::AP4_MpegSampleDescription(
    AP4_UI32              format,
    StreamType            stream_type,
    OTI                   oti,
    const AP4_DataBuffer* decoder_info,
    AP4_UI32              buffer_size,
    AP4_UI32              max_bitrate,
    AP4_UI32              avg_bitrate) :
    AP4_SampleDescription(TYPE_MPEG, format, NULL),
    m_StreamType(stream_type),
    m_ObjectTypeId(oti),
    m_BufferSize(buffer_size),
    m_MaxBitrate(max_bitrate),
    m_AvgBitrate(avg_bitrate)
{
    if (decoder_info != NULL) {
        m_DecoderInfo.SetData(decoder_info->GetData(), decoder_info->GetDataSize());
    }
}

/*----------------------------------------------------------------------
|   AP4_MpegSampleDescription::CreateEsDescriptor
+---------------------------------------------------------------------*/
AP4_EsDescriptor* 
AP4_MpegSampleDescription::CreateEsDescriptor() const
{
    AP4_EsDescriptor* desc = new AP4_EsDescriptor(0);
    AP4_DecoderSpecificInfoDescriptor* dsi_desc;
    if (m_DecoderInfo.GetDataSize() != 0) {
        dsi_desc = new AP4_DecoderSpecificInfoDescriptor(m_DecoderInfo);
    } else {
        dsi_desc = NULL;
    }
    AP4_DecoderConfigDescriptor* decoder_config = 
        new AP4_DecoderConfigDescriptor(m_StreamType,
        m_ObjectTypeId,
        m_BufferSize,
        m_MaxBitrate,
        m_AvgBitrate,
        dsi_desc);
    desc->AddSubDescriptor(decoder_config);
    
    // add a fixed SL Config
    desc->AddSubDescriptor(new AP4_SLConfigDescriptor());
    
    return desc;
}

/*----------------------------------------------------------------------
|   AP4_MpegSystemSampleDescription::AP4_MpegSystemSampleDescription
+---------------------------------------------------------------------*/
AP4_MpegSystemSampleDescription::AP4_MpegSystemSampleDescription(AP4_EsdsAtom* esds) :
    AP4_MpegSampleDescription(AP4_ATOM_TYPE_MP4S, esds)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegSystemSampleDescription::AP4_MpegSystemSampleDescription
+---------------------------------------------------------------------*/
AP4_MpegSystemSampleDescription::AP4_MpegSystemSampleDescription(
    StreamType            stream_type,
    OTI                   oti,
    const AP4_DataBuffer* decoder_info,
    AP4_UI32              buffer_size,
    AP4_UI32              max_bitrate,
    AP4_UI32              avg_bitrate) :
    AP4_MpegSampleDescription(AP4_ATOM_TYPE_MP4S,
                              stream_type,
                              oti,
                              decoder_info,
                              buffer_size,
                              max_bitrate,
                              avg_bitrate)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegSystemSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_MpegSystemSampleDescription::ToAtom() const
{
    return new AP4_Mp4sSampleEntry(CreateEsDescriptor());
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleDescription::AP4_MpegAudioSampleDescription
+---------------------------------------------------------------------*/
AP4_MpegAudioSampleDescription::AP4_MpegAudioSampleDescription(
    AP4_UI32  sample_rate,
    AP4_UI16  sample_size,
    AP4_UI16  channel_count,
    AP4_EsdsAtom* esds) :
    AP4_MpegSampleDescription(AP4_ATOM_TYPE_MP4A, esds),
    AP4_AudioSampleDescription(sample_rate, sample_size, channel_count)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleDescription::AP4_MpegAudioSampleDescription
+---------------------------------------------------------------------*/
AP4_MpegAudioSampleDescription::AP4_MpegAudioSampleDescription(
    OTI                   oti,
    AP4_UI32              sample_rate,
    AP4_UI16              sample_size,
    AP4_UI16              channel_count,
    const AP4_DataBuffer* decoder_info,
    AP4_UI32              buffer_size,
    AP4_UI32              max_bitrate,
    AP4_UI32              avg_bitrate) :
    AP4_MpegSampleDescription(AP4_ATOM_TYPE_MP4A,
                              AP4_STREAM_TYPE_AUDIO,
                              oti, 
                              decoder_info, buffer_size, 
                              max_bitrate, avg_bitrate),
    AP4_AudioSampleDescription(sample_rate, sample_size, channel_count)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_MpegAudioSampleDescription::ToAtom() const
{
    return new AP4_Mp4aSampleEntry(m_SampleRate<<16,
                                   m_SampleSize,
                                   m_ChannelCount,
                                   CreateEsDescriptor());
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleDescription::GetCodecString
+---------------------------------------------------------------------*/
AP4_Result
AP4_MpegAudioSampleDescription::GetCodecString(AP4_String& codec)
{
    char coding[5];
    AP4_FormatFourChars(coding, GetFormat());
    char workspace[64];
    workspace[0] = 0;
    if (GetFormat() == AP4_SAMPLE_FORMAT_MP4A) {
        if (GetObjectTypeId() == AP4_OTI_MPEG4_AUDIO) {
            Mpeg4AudioObjectType object_type = GetMpeg4AudioObjectType();
            if (object_type == AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_LC) {
                const AP4_DataBuffer& dsi = GetDecoderInfo();
                if (dsi.GetDataSize()) {
                    AP4_Mp4AudioDecoderConfig dec_config;
                    AP4_Result result = dec_config.Parse(dsi.GetData(), dsi.GetDataSize());
                    if (AP4_SUCCEEDED(result)) {
                        if (dec_config.m_Extension.m_PsPresent) {
                            object_type = AP4_MPEG4_AUDIO_OBJECT_TYPE_PS;
                        } else if (dec_config.m_Extension.m_SbrPresent) {
                            object_type = AP4_MPEG4_AUDIO_OBJECT_TYPE_SBR;
                        }
                    }
                }
            }
            AP4_FormatString(workspace, sizeof(workspace), "%s.%02X.%d", coding, (int)GetObjectTypeId(), object_type);
        } else {
            AP4_FormatString(workspace, sizeof(workspace), "%s.%02X", coding, (int)GetObjectTypeId());
        }
    }
    
    codec = workspace;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleDescription::GetMpeg4AudioObjectType
+---------------------------------------------------------------------*/
AP4_MpegAudioSampleDescription::Mpeg4AudioObjectType
AP4_MpegAudioSampleDescription::GetMpeg4AudioObjectType() const
{
    if (m_ObjectTypeId == AP4_OTI_MPEG4_AUDIO &&
        m_DecoderInfo.GetDataSize() >= 1) {
        AP4_UI08 type = m_DecoderInfo.GetData()[0]>>3;
        if (type == 31) {
            if (m_DecoderInfo.GetDataSize() < 2) return 0;
            type = 32+(((m_DecoderInfo.GetData()[0]&0x07)<<3) |
                       ((m_DecoderInfo.GetData()[1]&0xE0)>>5));
        }
        return type;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_MpegVideoSampleDescription::AP4_MpegVideoSampleDescription
+---------------------------------------------------------------------*/
AP4_MpegVideoSampleDescription::AP4_MpegVideoSampleDescription(
    AP4_UI16      width,
    AP4_UI16      height,
    AP4_UI16      depth,
    const char*   compressor_name,
    AP4_EsdsAtom* esds) :
    
    AP4_MpegSampleDescription(AP4_ATOM_TYPE_MP4V, esds),
    AP4_VideoSampleDescription(width, height, depth, compressor_name)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegVideoSampleDescription::AP4_MpegVideoSampleDescription
+---------------------------------------------------------------------*/
AP4_MpegVideoSampleDescription::AP4_MpegVideoSampleDescription(
    OTI                   oti,
    AP4_UI16              width,
    AP4_UI16              height,
    AP4_UI16              depth,
    const char*           compressor_name,
    const AP4_DataBuffer* decoder_info,
    AP4_UI32              buffer_size,
    AP4_UI32              max_bitrate,
    AP4_UI32              avg_bitrate) :
    AP4_MpegSampleDescription(AP4_ATOM_TYPE_MP4V, 
                              AP4_STREAM_TYPE_VISUAL,
                              oti,
                              decoder_info,
                              buffer_size,
                              max_bitrate,
                              avg_bitrate),
    AP4_VideoSampleDescription(width, height, depth, compressor_name)
{
}

/*----------------------------------------------------------------------
|   AP4_MpegVideoSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_MpegVideoSampleDescription::ToAtom() const
{
    return new AP4_Mp4vSampleEntry(m_Width,
                                   m_Height,
                                   m_Depth,
                                   m_CompressorName.GetChars(),
                                   CreateEsDescriptor());
}

/*----------------------------------------------------------------------
|   AP4_MpegSampleDescription::GetStreamTypeString
+---------------------------------------------------------------------*/
const char* 
AP4_MpegSampleDescription::GetStreamTypeString(StreamType type)
{
    switch (type) {
        case AP4_STREAM_TYPE_FORBIDDEN: return "INVALID"; 
        case AP4_STREAM_TYPE_OD:        return "Object Descriptor";
        case AP4_STREAM_TYPE_CR:        return "CR";	
        case AP4_STREAM_TYPE_BIFS:      return "BIFS";
        case AP4_STREAM_TYPE_VISUAL:    return "Visual";
        case AP4_STREAM_TYPE_AUDIO:     return "Audio";
        case AP4_STREAM_TYPE_MPEG7:     return "MPEG-7";
        case AP4_STREAM_TYPE_IPMP:      return "IPMP";
        case AP4_STREAM_TYPE_OCI:       return "OCI";
        case AP4_STREAM_TYPE_MPEGJ:     return "MPEG-J";
        default:                        return "UNKNOWN";
    }
}

/*----------------------------------------------------------------------
|   AP4_MpegSampleDescription::GetObjectTypeString
+---------------------------------------------------------------------*/
const char* 
AP4_MpegSampleDescription::GetObjectTypeString(OTI oti)
{
    switch (oti) {
        case AP4_OTI_MPEG4_SYSTEM:         return "MPEG-4 System";
        case AP4_OTI_MPEG4_SYSTEM_COR:     return "MPEG-4 System COR";
        case AP4_OTI_MPEG4_VISUAL:         return "MPEG-4 Video";
        case AP4_OTI_MPEG4_AUDIO:          return "MPEG-4 Audio";
        case AP4_OTI_MPEG2_VISUAL_SIMPLE:  return "MPEG-2 Video Simple Profile";
        case AP4_OTI_MPEG2_VISUAL_MAIN:    return "MPEG-2 Video Main Profile";
        case AP4_OTI_MPEG2_VISUAL_SNR:     return "MPEG-2 Video SNR";
        case AP4_OTI_MPEG2_VISUAL_SPATIAL: return "MPEG-2 Video Spatial";
        case AP4_OTI_MPEG2_VISUAL_HIGH:    return "MPEG-2 Video High";
        case AP4_OTI_MPEG2_VISUAL_422:     return "MPEG-2 Video 4:2:2";
        case AP4_OTI_MPEG2_AAC_AUDIO_MAIN: return "MPEG-2 Audio AAC Main Profile";
        case AP4_OTI_MPEG2_AAC_AUDIO_LC:   return "MPEG-2 Audio AAC Low Complexity";
        case AP4_OTI_MPEG2_AAC_AUDIO_SSRP: return "MPEG-2 Audio AAC SSRP";
        case AP4_OTI_MPEG2_PART3_AUDIO:    return "MPEG-2 Audio Part-3";
        case AP4_OTI_MPEG1_VISUAL:         return "MPEG-1 Video";
        case AP4_OTI_MPEG1_AUDIO:          return "MPEG-1 Audio";
        case AP4_OTI_JPEG:                 return "JPEG";
        case AP4_OTI_JPEG2000:             return "JPEG-2000";
        case AP4_OTI_EVRC_VOICE:           return "EVRC Voice";
        case AP4_OTI_SMV_VOICE:            return "SMV Voice";
        case AP4_OTI_3GPP2_CMF:            return "3GPP2 CMF";
        case AP4_OTI_SMPTE_VC1:            return "SMPTE VC1 Video";
        case AP4_OTI_DIRAC_VIDEO:          return "Dirac Video";
        case AP4_OTI_AC3_AUDIO:            return "AC3 Audio";
        case AP4_OTI_EAC3_AUDIO:           return "E-AC3 Audio";
        case AP4_OTI_DRA_AUDIO:            return "DRA Audio";
        case AP4_OTI_G719_AUDIO:           return "G.719 Audio";
        case AP4_OTI_DTS_AUDIO:            return "DTS Audio";
        case AP4_OTI_DTS_HIRES_AUDIO:      return "DTS High Resolution Audio";
        case AP4_OTI_DTS_MASTER_AUDIO:     return "DTS Master Audio";
        case AP4_OTI_DTS_EXPRESS_AUDIO:    return "DTS Express/LBR Audio";
        case AP4_OTI_13K_VOICE:            return "13K Voice";
        default:                           return "UNKNOWN";
    }
}

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleDescription::GetMpeg4AudioObjectTypeString
+---------------------------------------------------------------------*/
const char* 
AP4_MpegAudioSampleDescription::GetMpeg4AudioObjectTypeString(Mpeg4AudioObjectType type)
{
    switch (type) {
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_MAIN:                return "AAC Main Profile";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_LC:                  return "AAC Low Complexity";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_SSR:                 return "AAC Scalable Sample Rate";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_LTP:                 return "AAC Long Term Predictor";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_SBR:                     return "Spectral Band Replication";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_SCALABLE:            return "AAC Scalable";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_TWINVQ:                  return "Twin VQ";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_CELP:                    return "CELP";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_HVXC:                    return "HVXC";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_TTSI:                    return "TTSI";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_MAIN_SYNTHETIC:          return "Main Synthetic";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_WAVETABLE_SYNTHESIS:     return "Wavetable Synthesis";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_GENERAL_MIDI:            return "General MIDI";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ALGORITHMIC_SYNTHESIS:   return "Algorithmic Synthesis";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LC:               return "Error Resilient AAC Low Complexity";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LTP:              return "Error Resilient AAC Long Term Prediction";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_SCALABLE:         return "Error Resilient AAC Scalable";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_TWINVQ:               return "Error Resilient Twin VQ";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_BSAC:                 return "Error Resilient Bit Sliced Arithmetic Coding";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LD:               return "Error Resilient AAC Low Delay";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_CELP:                 return "Error Resilient CELP";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_HVXC:                 return "Error Resilient HVXC";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_HILN:                 return "Error Resilient HILN";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_PARAMETRIC:           return "Error Resilient Parametric";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_SSC:                     return "SSC";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_PS:                      return "Parametric Stereo";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_MPEG_SURROUND:           return "MPEG Surround";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_LAYER_1:                 return "MPEG Layer 1";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_LAYER_2:                 return "MPEG Layer 2";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_LAYER_3:                 return "MPEG Layer 3";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_DST:                     return "Direct Stream Transfer";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ALS:                     return "ALS Lossless Coding";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_SLS:                     return "SLS Scalable Lossless Coding";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_SLS_NON_CORE:            return "SLS Scalable Lossless Coding (Non Core)";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_ELD:              return "Error Resilient AAC ELD";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_SMR_SIMPLE:              return "SMR Simple";
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_SMR_MAIN:                return "SMR Main";
        default:                                                  return "UNKNOWN";
    }
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleDescription::AP4_SubtitleSampleDescription
+---------------------------------------------------------------------*/
AP4_SubtitleSampleDescription::AP4_SubtitleSampleDescription(AP4_UI32    format,
                                                             const char* namespce,
                                                             const char* schema_location,
                                                             const char* image_mime_type) :
    AP4_SampleDescription(AP4_SampleDescription::TYPE_SUBTITLES, format, NULL),
    m_Namespace(namespce),
    m_SchemaLocation(schema_location),
    m_ImageMimeType(image_mime_type)
{
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleDescription::Clone
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_SubtitleSampleDescription::Clone(AP4_Result* result)
{
    if (result) *result = AP4_SUCCESS;
    return new AP4_SubtitleSampleDescription(m_Format,
                                             m_Namespace.GetChars(),
                                             m_SchemaLocation.GetChars(),
                                             m_ImageMimeType.GetChars());
}

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_SubtitleSampleDescription::ToAtom() const
{
    return new AP4_SubtitleSampleEntry(m_Format,
                                       m_Namespace.GetChars(),
                                       m_SchemaLocation.GetChars(),
                                       m_ImageMimeType.GetChars());
}
    

