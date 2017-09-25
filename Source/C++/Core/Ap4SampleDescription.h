/*****************************************************************
|
|    AP4 - Sample Descriptions
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

#ifndef _AP4_SAMPLE_DESCRIPTION_H_
#define _AP4_SAMPLE_DESCRIPTION_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4Atom.h"
#include "Ap4EsDescriptor.h"
#include "Ap4EsdsAtom.h"
#include "Ap4Array.h"
#include "Ap4AvccAtom.h"
#include "Ap4HvccAtom.h"
#include "Ap4DynamicCast.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class AP4_SampleEntry;
class AP4_DataBuffer;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_SAMPLE_FORMAT_MP4A = AP4_ATOM_TYPE('m','p','4','a');
const AP4_UI32 AP4_SAMPLE_FORMAT_MP4V = AP4_ATOM_TYPE('m','p','4','v');
const AP4_UI32 AP4_SAMPLE_FORMAT_MP4S = AP4_ATOM_TYPE('m','p','4','s');
const AP4_UI32 AP4_SAMPLE_FORMAT_AVC1 = AP4_ATOM_TYPE('a','v','c','1');
const AP4_UI32 AP4_SAMPLE_FORMAT_AVC2 = AP4_ATOM_TYPE('a','v','c','2');
const AP4_UI32 AP4_SAMPLE_FORMAT_AVC3 = AP4_ATOM_TYPE('a','v','c','3');
const AP4_UI32 AP4_SAMPLE_FORMAT_AVC4 = AP4_ATOM_TYPE('a','v','c','4');
const AP4_UI32 AP4_SAMPLE_FORMAT_DVAV = AP4_ATOM_TYPE('d','v','a','v');
const AP4_UI32 AP4_SAMPLE_FORMAT_DVA1 = AP4_ATOM_TYPE('d','v','a','1');
const AP4_UI32 AP4_SAMPLE_FORMAT_HVC1 = AP4_ATOM_TYPE('h','v','c','1');
const AP4_UI32 AP4_SAMPLE_FORMAT_HEV1 = AP4_ATOM_TYPE('h','e','v','1');
const AP4_UI32 AP4_SAMPLE_FORMAT_DVHE = AP4_ATOM_TYPE('d','v','h','e');
const AP4_UI32 AP4_SAMPLE_FORMAT_DVH1 = AP4_ATOM_TYPE('d','v','h','1');
const AP4_UI32 AP4_SAMPLE_FORMAT_ALAC = AP4_ATOM_TYPE('a','l','a','c');
const AP4_UI32 AP4_SAMPLE_FORMAT_OWMA = AP4_ATOM_TYPE('o','w','m','a');
const AP4_UI32 AP4_SAMPLE_FORMAT_OVC1 = AP4_ATOM_TYPE('o','v','c','1');
const AP4_UI32 AP4_SAMPLE_FORMAT_AVCP = AP4_ATOM_TYPE('a','v','c','p');
const AP4_UI32 AP4_SAMPLE_FORMAT_DRAC = AP4_ATOM_TYPE('d','r','a','c');
const AP4_UI32 AP4_SAMPLE_FORMAT_DRA1 = AP4_ATOM_TYPE('d','r','a','1');
const AP4_UI32 AP4_SAMPLE_FORMAT_AC_3 = AP4_ATOM_TYPE('a','c','-','3');
const AP4_UI32 AP4_SAMPLE_FORMAT_EC_3 = AP4_ATOM_TYPE('e','c','-','3');
const AP4_UI32 AP4_SAMPLE_FORMAT_DTSC = AP4_ATOM_TYPE('d','t','s','c');
const AP4_UI32 AP4_SAMPLE_FORMAT_DTSH = AP4_ATOM_TYPE('d','t','s','h');
const AP4_UI32 AP4_SAMPLE_FORMAT_DTSL = AP4_ATOM_TYPE('d','t','s','l');
const AP4_UI32 AP4_SAMPLE_FORMAT_DTSE = AP4_ATOM_TYPE('d','t','s','e');
const AP4_UI32 AP4_SAMPLE_FORMAT_G726 = AP4_ATOM_TYPE('g','7','2','6');
const AP4_UI32 AP4_SAMPLE_FORMAT_MJP2 = AP4_ATOM_TYPE('m','j','p','2');
const AP4_UI32 AP4_SAMPLE_FORMAT_OKSD = AP4_ATOM_TYPE('o','k','s','d');
const AP4_UI32 AP4_SAMPLE_FORMAT_RAW_ = AP4_ATOM_TYPE('r','a','w',' ');
const AP4_UI32 AP4_SAMPLE_FORMAT_RTP_ = AP4_ATOM_TYPE('r','t','p',' ');
const AP4_UI32 AP4_SAMPLE_FORMAT_S263 = AP4_ATOM_TYPE('s','2','6','3');
const AP4_UI32 AP4_SAMPLE_FORMAT_SAMR = AP4_ATOM_TYPE('s','a','m','r');
const AP4_UI32 AP4_SAMPLE_FORMAT_SAWB = AP4_ATOM_TYPE('s','a','w','b');
const AP4_UI32 AP4_SAMPLE_FORMAT_SAWP = AP4_ATOM_TYPE('s','a','w','p');
const AP4_UI32 AP4_SAMPLE_FORMAT_SEVC = AP4_ATOM_TYPE('s','e','v','c');
const AP4_UI32 AP4_SAMPLE_FORMAT_SQCP = AP4_ATOM_TYPE('s','q','c','p');
const AP4_UI32 AP4_SAMPLE_FORMAT_SRTP = AP4_ATOM_TYPE('s','r','t','p');
const AP4_UI32 AP4_SAMPLE_FORMAT_SSMV = AP4_ATOM_TYPE('s','s','m','v');
const AP4_UI32 AP4_SAMPLE_FORMAT_TEXT = AP4_ATOM_TYPE('t','e','t','x');
const AP4_UI32 AP4_SAMPLE_FORMAT_TWOS = AP4_ATOM_TYPE('t','w','o','s');
const AP4_UI32 AP4_SAMPLE_FORMAT_TX3G = AP4_ATOM_TYPE('t','x','3','g');
const AP4_UI32 AP4_SAMPLE_FORMAT_VC_1 = AP4_ATOM_TYPE('v','c','-','1');
const AP4_UI32 AP4_SAMPLE_FORMAT_XML_ = AP4_ATOM_TYPE('x','m','l',' ');
const AP4_UI32 AP4_SAMPLE_FORMAT_STPP = AP4_ATOM_TYPE('s','t','p','p');

const char*
AP4_GetFormatName(AP4_UI32 format);

/*----------------------------------------------------------------------
|   AP4_SampleDescription
+---------------------------------------------------------------------*/
class AP4_SampleDescription
{
 public:
    AP4_IMPLEMENT_DYNAMIC_CAST(AP4_SampleDescription)

    // type constants of the sample description
    enum Type {
        TYPE_UNKNOWN   = 0x00,
        TYPE_MPEG      = 0x01,
        TYPE_PROTECTED = 0x02,
        TYPE_AVC       = 0x03,
        TYPE_HEVC      = 0x04,
        TYPE_SUBTITLES = 0x05
    };

    // constructors & destructor
    AP4_SampleDescription(Type            type, 
                          AP4_UI32        format, 
                          AP4_AtomParent* details);
    virtual ~AP4_SampleDescription() {}
    virtual AP4_SampleDescription* Clone(AP4_Result* result = NULL);
    
    // accessors
    Type                  GetType()    const { return m_Type;    }
    AP4_UI32              GetFormat()  const { return m_Format;  }
    const AP4_AtomParent& GetDetails() const { return m_Details; }

    // info
    virtual AP4_Result GetCodecString(AP4_String& codec);

    // factories
    virtual AP4_Atom* ToAtom() const;

 protected:
    Type           m_Type;
    AP4_UI32       m_Format;
    AP4_AtomParent m_Details;
};

/*----------------------------------------------------------------------
|   AP4_UnknownSampleDescription
+---------------------------------------------------------------------*/
class AP4_UnknownSampleDescription : public AP4_SampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_UnknownSampleDescription, AP4_SampleDescription)

    // this constructor takes makes a copy of the atom passed as an argument
    AP4_UnknownSampleDescription(AP4_Atom* atom);
    ~AP4_UnknownSampleDescription();

    virtual AP4_SampleDescription* Clone(AP4_Result* result);
    virtual AP4_Atom* ToAtom() const;    
    
    // accessor
    const AP4_Atom* GetAtom() { return m_Atom; }
    
private:
    AP4_Atom* m_Atom;
};

/*----------------------------------------------------------------------
|   AP4_AudioSampleDescription  // MIXIN class
+---------------------------------------------------------------------*/
class AP4_AudioSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST(AP4_AudioSampleDescription)

    // constructor and destructor
    AP4_AudioSampleDescription(AP4_UI32 sample_rate,
                               AP4_UI16 sample_size,
                               AP4_UI16 channel_count) :
    m_SampleRate(sample_rate),
    m_SampleSize(sample_size),
    m_ChannelCount(channel_count) {}
    virtual ~AP4_AudioSampleDescription() {}
    
    // accessors
    AP4_UI32 GetSampleRate()   { return m_SampleRate;   }
    AP4_UI16 GetSampleSize()   { return m_SampleSize;   }
    AP4_UI16 GetChannelCount() { return m_ChannelCount; }

protected:
    // members
    AP4_UI32 m_SampleRate;
    AP4_UI16 m_SampleSize;
    AP4_UI16 m_ChannelCount;
};

/*----------------------------------------------------------------------
|   AP4_VideoSampleDescription  // MIXIN class
+---------------------------------------------------------------------*/
class AP4_VideoSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST(AP4_VideoSampleDescription)

    // constructor and destructor
    AP4_VideoSampleDescription(AP4_UI16    width,
                               AP4_UI16    height,
                               AP4_UI16    depth,
                               const char* compressor_name) :
    m_Width(width),
    m_Height(height),
    m_Depth(depth),
    m_CompressorName(compressor_name) {}
    virtual ~AP4_VideoSampleDescription() {}

    // accessors
    AP4_UI16    GetWidth()          { return m_Width;  }
    AP4_UI16    GetHeight()         { return m_Height; }
    AP4_UI16    GetDepth()          { return m_Depth;  }
    const char* GetCompressorName() { return m_CompressorName.GetChars(); }

protected:
    // members
    AP4_UI16   m_Width;
    AP4_UI16   m_Height;
    AP4_UI16   m_Depth;
    AP4_String m_CompressorName;
};

/*----------------------------------------------------------------------
|   AP4_GenericAudioSampleDescription
+---------------------------------------------------------------------*/
class AP4_GenericAudioSampleDescription : public AP4_SampleDescription,
                                          public AP4_AudioSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_GenericAudioSampleDescription, AP4_SampleDescription, AP4_AudioSampleDescription)

    // constructors
    AP4_GenericAudioSampleDescription(AP4_UI32        format,
                                      AP4_UI32        sample_rate,
                                      AP4_UI16        sample_size,
                                      AP4_UI16        channel_count,
                                      AP4_AtomParent* details) :
        AP4_SampleDescription(TYPE_UNKNOWN, format, details),
        AP4_AudioSampleDescription(sample_rate, sample_size, channel_count) {}
    
    // inherited from AP4_SampleDescription
    virtual AP4_Atom* ToAtom() const;
};

/*----------------------------------------------------------------------
|   AP4_GenericVideoSampleDescription
+---------------------------------------------------------------------*/
class AP4_GenericVideoSampleDescription : public AP4_SampleDescription,
                                          public AP4_VideoSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_GenericVideoSampleDescription, AP4_SampleDescription, AP4_VideoSampleDescription)

    // constructor
    AP4_GenericVideoSampleDescription(AP4_UI32        format,
                                      AP4_UI16        width,
                                      AP4_UI16        height,
                                      AP4_UI16        depth,
                                      const char*     compressor_name,
                                      AP4_AtomParent* details) :
        AP4_SampleDescription(TYPE_UNKNOWN, format, details),
        AP4_VideoSampleDescription(width, height, depth, compressor_name) {}
    
    // inherited from AP4_SampleDescription
    virtual AP4_Atom* ToAtom() const;    
};

/*----------------------------------------------------------------------
|   AP4_AvcSampleDescription
+---------------------------------------------------------------------*/
class AP4_AvcSampleDescription : public AP4_SampleDescription,
                                 public AP4_VideoSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_AvcSampleDescription, AP4_SampleDescription, AP4_VideoSampleDescription)

    // constructors
    AP4_AvcSampleDescription(AP4_UI32            format, // avc1, avc2, avc3 or avc4
                             AP4_UI16            width,
                             AP4_UI16            height,
                             AP4_UI16            depth,
                             const char*         compressor_name,
                             const AP4_AvccAtom* avcc);
    
    AP4_AvcSampleDescription(AP4_UI32        format, // avc1, avc2, avc3 or avc4
                             AP4_UI16        width,
                             AP4_UI16        height,
                             AP4_UI16        depth,
                             const char*     compressor_name,
                             AP4_AtomParent* details);

    AP4_AvcSampleDescription(AP4_UI32                         format,
                             AP4_UI16                         width,
                             AP4_UI16                         height,
                             AP4_UI16                         depth,
                             const char*                      compressor_name,
                             AP4_UI08                         profile,
                             AP4_UI08                         level,
                             AP4_UI08                         profile_compatibility,
                             AP4_UI08                         nalu_length_size,
                             const AP4_Array<AP4_DataBuffer>& sequence_parameters,
                             const AP4_Array<AP4_DataBuffer>& picture_parameters);
    
    // accessors
    AP4_UI08 GetConfigurationVersion() const { return m_AvccAtom->GetConfigurationVersion(); }
    AP4_UI08 GetProfile() const { return m_AvccAtom->GetProfile(); }
    AP4_UI08 GetLevel() const { return m_AvccAtom->GetLevel(); }
    AP4_UI08 GetProfileCompatibility() const { return m_AvccAtom->GetProfileCompatibility(); }
    AP4_UI08 GetNaluLengthSize() const { return m_AvccAtom->GetNaluLengthSize(); }
    AP4_Array<AP4_DataBuffer>& GetSequenceParameters() {return m_AvccAtom->GetSequenceParameters(); }
    AP4_Array<AP4_DataBuffer>& GetPictureParameters() { return m_AvccAtom->GetPictureParameters(); }
    const AP4_DataBuffer& GetRawBytes() const { return m_AvccAtom->GetRawBytes(); }
    
    // inherited from AP4_SampleDescription
    virtual AP4_Atom* ToAtom() const;
    virtual AP4_Result GetCodecString(AP4_String& codec);
    
    // static methods
    static const char* GetProfileName(AP4_UI08 profile) {
        return AP4_AvccAtom::GetProfileName(profile);
    }

private:
    AP4_AvccAtom* m_AvccAtom;
};

/*----------------------------------------------------------------------
|   AP4_HevcSampleDescription
+---------------------------------------------------------------------*/
class AP4_HevcSampleDescription : public AP4_SampleDescription,
                                  public AP4_VideoSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_HevcSampleDescription, AP4_SampleDescription, AP4_VideoSampleDescription)

    // constructors
    AP4_HevcSampleDescription(AP4_UI32            format, // hvc1 or hev1
                              AP4_UI16            width,
                              AP4_UI16            height,
                              AP4_UI16            depth,
                              const char*         compressor_name,
                              const AP4_HvccAtom* hvcc);
    
    AP4_HevcSampleDescription(AP4_UI32        format, // hvc1 or hev1
                              AP4_UI16        width,
                              AP4_UI16        height,
                              AP4_UI16        depth,
                              const char*     compressor_name,
                              AP4_AtomParent* details);
    
    AP4_HevcSampleDescription(AP4_UI32                         format,
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
                              AP4_UI08                         picture_parameters_completeness);
 
    // accessors
    AP4_UI08 GetConfigurationVersion()             const { return m_HvccAtom->GetConfigurationVersion(); }
    AP4_UI08 GetGeneralProfileSpace()              const { return m_HvccAtom->GetGeneralProfileSpace(); }
    AP4_UI08 GetGeneralTierFlag()                  const { return m_HvccAtom->GetGeneralTierFlag(); }
    AP4_UI08 GetGeneralProfile()                   const { return m_HvccAtom->GetGeneralProfile(); }
    AP4_UI32 GetGeneralProfileCompatibilityFlags() const { return m_HvccAtom->GetGeneralProfileCompatibilityFlags(); }
    AP4_UI64 GetGeneralConstraintIndicatorFlags()  const { return m_HvccAtom->GetGeneralConstraintIndicatorFlags(); }
    AP4_UI08 GetGeneralLevel()                     const { return m_HvccAtom->GetGeneralLevel(); }
    AP4_UI32 GetMinSpatialSegmentation()           const { return m_HvccAtom->GetMinSpatialSegmentation(); }
    AP4_UI08 GetParallelismType()                  const { return m_HvccAtom->GetParallelismType(); }
    AP4_UI08 GetChromaFormat()                     const { return m_HvccAtom->GetChromaFormat(); }
    AP4_UI08 GetLumaBitDepth()                     const { return m_HvccAtom->GetLumaBitDepth(); }
    AP4_UI08 GetChromaBitDepth()                   const { return m_HvccAtom->GetChromaBitDepth(); }
    AP4_UI16 GetAverageFrameRate()                 const { return m_HvccAtom->GetAverageFrameRate(); }
    AP4_UI08 GetConstantFrameRate()                const { return m_HvccAtom->GetConstantFrameRate(); }
    AP4_UI08 GetNumTemporalLayers()                const { return m_HvccAtom->GetNumTemporalLayers(); }
    AP4_UI08 GetTemporalIdNested()                 const { return m_HvccAtom->GetTemporalIdNested(); }
    AP4_UI08 GetNaluLengthSize()                   const { return m_HvccAtom->GetNaluLengthSize(); }
    const AP4_Array<AP4_HvccAtom::Sequence>& GetSequences() const { return m_HvccAtom->GetSequences(); }
    const AP4_DataBuffer& GetRawBytes()            const { return m_HvccAtom->GetRawBytes(); }
    
    // inherited from AP4_SampleDescription
    virtual AP4_Atom* ToAtom() const;
    virtual AP4_Result GetCodecString(AP4_String& codec);
    
    // static methods
    static const char* GetProfileName(AP4_UI08 profile_space, AP4_UI08 profile) {
        return AP4_HvccAtom::GetProfileName(profile_space, profile);
    }

private:
    AP4_HvccAtom* m_HvccAtom;
};

/*----------------------------------------------------------------------
|   AP4_MpegSampleDescription
+---------------------------------------------------------------------*/
class AP4_MpegSampleDescription : public AP4_SampleDescription
{
 public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_MpegSampleDescription, AP4_SampleDescription)

    // types
    typedef AP4_UI08 StreamType;
    typedef AP4_UI08 OTI;
    
    // class methods
    static const char* GetStreamTypeString(StreamType type);
    static const char* GetObjectTypeString(OTI oti);

    // constructors & destructor
    AP4_MpegSampleDescription(AP4_UI32      format,
                              AP4_EsdsAtom* esds);
    AP4_MpegSampleDescription(AP4_UI32              format,
                              StreamType            stream_type,
                              OTI                   oti,
                              const AP4_DataBuffer* decoder_info,
                              AP4_UI32              buffer_size,
                              AP4_UI32              max_bitrate,
                              AP4_UI32              avg_bitrate);
    
    // accessors
    AP4_Byte GetStreamType()   const { return m_StreamType; }
    AP4_Byte GetObjectTypeId() const { return m_ObjectTypeId; }
    AP4_UI32 GetBufferSize()   const { return m_BufferSize; }
    AP4_UI32 GetMaxBitrate()   const { return m_MaxBitrate; }
    AP4_UI32 GetAvgBitrate()   const { return m_AvgBitrate; }
    const AP4_DataBuffer& GetDecoderInfo() const { return m_DecoderInfo; }

    // methods
    AP4_EsDescriptor* CreateEsDescriptor() const;

 protected:
    // members
    AP4_UI32       m_Format;
    StreamType     m_StreamType;
    OTI            m_ObjectTypeId;
    AP4_UI32       m_BufferSize;
    AP4_UI32       m_MaxBitrate;
    AP4_UI32       m_AvgBitrate;
    AP4_DataBuffer m_DecoderInfo;
};

/*----------------------------------------------------------------------
|   AP4_MpegSystemSampleDescription
+---------------------------------------------------------------------*/
class AP4_MpegSystemSampleDescription : public AP4_MpegSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_MpegSystemSampleDescription, AP4_MpegSampleDescription)

    // constructor
    AP4_MpegSystemSampleDescription(AP4_EsdsAtom* esds);
    AP4_MpegSystemSampleDescription(StreamType            stream_type,
                                    OTI                   oti,
                                    const AP4_DataBuffer* decoder_info,
                                    AP4_UI32              buffer_size,
                                    AP4_UI32              max_bitrate,
                                    AP4_UI32              avg_bitrate);

    // methods
    AP4_Atom* ToAtom() const;
};

/*----------------------------------------------------------------------
|   AP4_MpegAudioSampleDescription
+---------------------------------------------------------------------*/
class AP4_MpegAudioSampleDescription : public AP4_MpegSampleDescription,
                                       public AP4_AudioSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_MpegAudioSampleDescription, AP4_MpegSampleDescription, AP4_AudioSampleDescription)

    // types
    typedef AP4_UI08 Mpeg4AudioObjectType;
    
    // class methods
    static const char* GetMpeg4AudioObjectTypeString(Mpeg4AudioObjectType type);
    
    // constructor
    AP4_MpegAudioSampleDescription(AP4_UI32      sample_rate,
                                   AP4_UI16      sample_size,
                                   AP4_UI16      channel_count,
                                   AP4_EsdsAtom* esds);
                                   
    AP4_MpegAudioSampleDescription(OTI                   oti,
                                   AP4_UI32              sample_rate,
                                   AP4_UI16              sample_size,
                                   AP4_UI16              channel_count,
                                   const AP4_DataBuffer* decoder_info,
                                   AP4_UI32              buffer_size,
                                   AP4_UI32              max_bitrate,
                                   AP4_UI32              avg_bitrate);

    // inherited from AP4_SampleDescription
    virtual AP4_Result GetCodecString(AP4_String& codec);
    AP4_Atom* ToAtom() const;

    /**
     * For sample descriptions of MPEG-4 audio tracks (i.e GetObjectTypeId() 
     * returns AP4_OTI_MPEG4_AUDIO), this method returns the MPEG4 Audio Object 
     * Type. For other sample descriptions, this method returns 0.
     */
    Mpeg4AudioObjectType GetMpeg4AudioObjectType() const;
};

/*----------------------------------------------------------------------
|   AP4_MpegVideoSampleDescription
+---------------------------------------------------------------------*/
class AP4_MpegVideoSampleDescription : public AP4_MpegSampleDescription,
                                       public AP4_VideoSampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_MpegVideoSampleDescription, AP4_MpegSampleDescription, AP4_VideoSampleDescription)

    // constructor
    AP4_MpegVideoSampleDescription(AP4_UI16      width,
                                   AP4_UI16      height,
                                   AP4_UI16      depth,
                                   const char*   compressor_name,
                                   AP4_EsdsAtom* esds);
                                   
    AP4_MpegVideoSampleDescription(OTI                   oti,
                                   AP4_UI16              width,
                                   AP4_UI16              height,
                                   AP4_UI16              depth,
                                   const char*           compressor_name,
                                   const AP4_DataBuffer* decoder_info,
                                   AP4_UI32              buffer_size,
                                   AP4_UI32              max_bitrate,
                                   AP4_UI32              avg_bitrate);

    // methods
    AP4_Atom* ToAtom() const;
};

/*----------------------------------------------------------------------
|   AP4_SubtitleSampleDescription
+---------------------------------------------------------------------*/
class AP4_SubtitleSampleDescription : public AP4_SampleDescription
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_SubtitleSampleDescription, AP4_SampleDescription)

    // constructor
    AP4_SubtitleSampleDescription(AP4_UI32    format,
                                  const char* namespce,
                                  const char* schema_location,
                                  const char* image_mime_type);

    virtual AP4_SampleDescription* Clone(AP4_Result* result);
    virtual AP4_Atom* ToAtom() const;    
    
    // accessor
    const AP4_String& GetNamespace()      { return m_Namespace;      }
    const AP4_String& GetSchemaLocation() { return m_SchemaLocation; }
    const AP4_String& GetImageMimeType()  { return m_ImageMimeType;  }
    
private:
    AP4_String m_Namespace;
    AP4_String m_SchemaLocation;
    AP4_String m_ImageMimeType;
};

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_FORBIDDEN = 0x00;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_OD        = 0x01;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_CR        = 0x02;	
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_BIFS      = 0x03;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_VISUAL    = 0x04;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_AUDIO     = 0x05;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_MPEG7     = 0x06;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_IPMP      = 0x07;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_OCI       = 0x08;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_MPEGJ     = 0x09;
const AP4_MpegSampleDescription::StreamType AP4_STREAM_TYPE_TEXT      = 0x0D;

const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG4_SYSTEM         = 0x01;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG4_SYSTEM_COR     = 0x02;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG4_TEXT           = 0x08;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG4_VISUAL         = 0x20;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG4_AUDIO          = 0x40;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_VISUAL_SIMPLE  = 0x60;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_VISUAL_MAIN    = 0x61;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_VISUAL_SNR     = 0x62;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_VISUAL_SPATIAL = 0x63;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_VISUAL_HIGH    = 0x64;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_VISUAL_422     = 0x65;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_AAC_AUDIO_MAIN = 0x66;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_AAC_AUDIO_LC   = 0x67;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_AAC_AUDIO_SSRP = 0x68;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG2_PART3_AUDIO    = 0x69;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG1_VISUAL         = 0x6A;
const AP4_MpegSampleDescription::OTI AP4_OTI_MPEG1_AUDIO          = 0x6B;
const AP4_MpegSampleDescription::OTI AP4_OTI_JPEG                 = 0x6C;
const AP4_MpegSampleDescription::OTI AP4_OTI_PNG                  = 0x6D;
const AP4_MpegSampleDescription::OTI AP4_OTI_JPEG2000             = 0x6E;
const AP4_MpegSampleDescription::OTI AP4_OTI_EVRC_VOICE           = 0xA0;
const AP4_MpegSampleDescription::OTI AP4_OTI_SMV_VOICE            = 0xA1;
const AP4_MpegSampleDescription::OTI AP4_OTI_3GPP2_CMF            = 0xA2;
const AP4_MpegSampleDescription::OTI AP4_OTI_SMPTE_VC1            = 0xA3;
const AP4_MpegSampleDescription::OTI AP4_OTI_DIRAC_VIDEO          = 0xA4;
const AP4_MpegSampleDescription::OTI AP4_OTI_AC3_AUDIO            = 0xA5;
const AP4_MpegSampleDescription::OTI AP4_OTI_EAC3_AUDIO           = 0xA6;
const AP4_MpegSampleDescription::OTI AP4_OTI_DRA_AUDIO            = 0xA7;
const AP4_MpegSampleDescription::OTI AP4_OTI_G719_AUDIO           = 0xA8;
const AP4_MpegSampleDescription::OTI AP4_OTI_DTS_AUDIO            = 0xA9;
const AP4_MpegSampleDescription::OTI AP4_OTI_DTS_HIRES_AUDIO      = 0xAA;
const AP4_MpegSampleDescription::OTI AP4_OTI_DTS_MASTER_AUDIO     = 0xAB;
const AP4_MpegSampleDescription::OTI AP4_OTI_DTS_EXPRESS_AUDIO    = 0xAC;
const AP4_MpegSampleDescription::OTI AP4_OTI_13K_VOICE            = 0xE1;

const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_MAIN              = 1;  /**< AAC Main Profile                             */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_LC                = 2;  /**< AAC Low Complexity                           */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_SSR               = 3;  /**< AAC Scalable Sample Rate                     */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_LTP               = 4;  /**< AAC Long Term Predictor                      */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_SBR                   = 5;  /**< Spectral Band Replication                    */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_SCALABLE          = 6;  /**< AAC Scalable                                 */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_TWINVQ                = 7;  /**< Twin VQ                                      */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_CELP                  = 8;  /**< CELP                                         */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_HVXC                  = 9;  /**< HVXC                                         */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_TTSI                  = 12; /**< TTSI                                         */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_MAIN_SYNTHETIC        = 13; /**< Main Synthetic                               */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_WAVETABLE_SYNTHESIS   = 14; /**< WavetableSynthesis                           */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_GENERAL_MIDI          = 15; /**< General MIDI                                 */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ALGORITHMIC_SYNTHESIS = 16; /**< Algorithmic Synthesis                        */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LC             = 17; /**< Error Resilient AAC Low Complexity           */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LTP            = 19; /**< Error Resilient AAC Long Term Prediction     */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_SCALABLE       = 20; /**< Error Resilient AAC Scalable                 */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_TWINVQ             = 21; /**< Error Resilient Twin VQ                      */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_BSAC               = 22; /**< Error Resilient Bit Sliced Arithmetic Coding */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LD             = 23; /**< Error Resilient AAC Low Delay                */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_CELP               = 24; /**< Error Resilient CELP                         */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_HVXC               = 25; /**< Error Resilient HVXC                         */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_HILN               = 26; /**< Error Resilient HILN                         */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_PARAMETRIC         = 27; /**< Error Resilient Parametric                   */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_SSC                   = 28; /**< SSC                                          */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_PS                    = 29; /**< Parametric Stereo                            */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_MPEG_SURROUND         = 30; /**< MPEG Surround                                */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_LAYER_1               = 32; /**< MPEG Layer 1                                 */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_LAYER_2               = 33; /**< MPEG Layer 2                                 */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_LAYER_3               = 34; /**< MPEG Layer 3                                 */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_DST                   = 35; /**< DST Direct Stream Transfer                   */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ALS                   = 36; /**< ALS Lossless Coding                          */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_SLS                   = 37; /**< SLS Scalable Lossless Coding                 */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_SLS_NON_CORE          = 38; /**< SLS Sclable Lossless Coding Non-Core         */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_ELD            = 39; /**< Error Resilient AAC ELD                      */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_SMR_SIMPLE            = 40; /**< SMR Simple                                   */
const AP4_UI08 AP4_MPEG4_AUDIO_OBJECT_TYPE_SMR_MAIN              = 41; /**< SMR Main                                     */

#endif // _AP4_SAMPLE_DESCRIPTION_H_

