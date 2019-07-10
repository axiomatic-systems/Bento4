/*****************************************************************
|
|    AP4 - avcC Atoms 
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

#ifndef _AP4_HVCC_ATOM_H_
#define _AP4_HVCC_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_HEVC_PROFILE_MAIN               = 1;
const AP4_UI08 AP4_HEVC_PROFILE_MAIN_10            = 2;
const AP4_UI08 AP4_HEVC_PROFILE_MAIN_STILL_PICTURE = 3;
const AP4_UI08 AP4_HEVC_PROFILE_REXT               = 4;

const AP4_UI08 AP4_HEVC_CHROMA_FORMAT_MONOCHROME = 0;
const AP4_UI08 AP4_HEVC_CHROMA_FORMAT_420        = 1;
const AP4_UI08 AP4_HEVC_CHROMA_FORMAT_422        = 2;
const AP4_UI08 AP4_HEVC_CHROMA_FORMAT_444        = 3;

/*----------------------------------------------------------------------
|   AP4_HvccAtom
+---------------------------------------------------------------------*/
class AP4_HvccAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_HvccAtom, AP4_Atom)

    // types
    class Sequence {
    public:
        AP4_UI08                  m_ArrayCompleteness;
        AP4_UI08                  m_Reserved;
        AP4_UI08                  m_NaluType;
        AP4_Array<AP4_DataBuffer> m_Nalus;
    };
    
    // class methods
    static AP4_HvccAtom* Create(AP4_Size size, AP4_ByteStream& stream);
    static const char*   GetProfileName(AP4_UI08 profile_space, AP4_UI08 profile);
    static const char*   GetChromaFormatName(AP4_UI08 chroma_format);
    
    // constructors
    AP4_HvccAtom();
    AP4_HvccAtom(const AP4_HvccAtom& other); // copy construtor
    AP4_HvccAtom(AP4_UI08                         general_profile_space,
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

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI08 GetConfigurationVersion()             const { return m_ConfigurationVersion; }
    AP4_UI08 GetGeneralProfileSpace()              const { return m_GeneralProfileSpace; }
    AP4_UI08 GetGeneralTierFlag()                  const { return m_GeneralTierFlag; }
    AP4_UI08 GetGeneralProfile()                   const { return m_GeneralProfile; }
    AP4_UI32 GetGeneralProfileCompatibilityFlags() const { return m_GeneralProfileCompatibilityFlags; }
    AP4_UI64 GetGeneralConstraintIndicatorFlags()  const { return m_GeneralConstraintIndicatorFlags; }
    AP4_UI08 GetGeneralLevel()                     const { return m_GeneralLevel; }
    AP4_UI32 GetMinSpatialSegmentation()           const { return m_MinSpatialSegmentation; }
    AP4_UI08 GetParallelismType()                  const { return m_ParallelismType; }
    AP4_UI08 GetChromaFormat()                     const { return m_ChromaFormat; }
    AP4_UI08 GetLumaBitDepth()                     const { return m_LumaBitDepth; }
    AP4_UI08 GetChromaBitDepth()                   const { return m_ChromaBitDepth; }
    AP4_UI16 GetAverageFrameRate()                 const { return m_AverageFrameRate; }
    AP4_UI08 GetConstantFrameRate()                const { return m_ConstantFrameRate; }
    AP4_UI08 GetNumTemporalLayers()                const { return m_NumTemporalLayers; }
    AP4_UI08 GetTemporalIdNested()                 const { return m_TemporalIdNested; }
    AP4_UI08 GetNaluLengthSize()                   const { return m_NaluLengthSize; }
    const AP4_Array<Sequence>& GetSequences()      const { return m_Sequences; }
    const AP4_DataBuffer& GetRawBytes()            const { return m_RawBytes; }

private:
    // methods
    AP4_HvccAtom(AP4_UI32 size, const AP4_UI08* payload);
    void UpdateRawBytes();
    
    // members
    AP4_UI08                  m_ConfigurationVersion;
    AP4_UI08                  m_GeneralProfileSpace;
    AP4_UI08                  m_GeneralTierFlag;
    AP4_UI08                  m_GeneralProfile;
    AP4_UI32                  m_GeneralProfileCompatibilityFlags;
    AP4_UI64                  m_GeneralConstraintIndicatorFlags;
    AP4_UI08                  m_GeneralLevel;
    AP4_UI08                  m_Reserved1;
    AP4_UI32                  m_MinSpatialSegmentation;
    AP4_UI08                  m_Reserved2;
    AP4_UI08                  m_ParallelismType;
    AP4_UI08                  m_Reserved3;
    AP4_UI08                  m_ChromaFormat;
    AP4_UI08                  m_Reserved4;
    AP4_UI08                  m_LumaBitDepth;
    AP4_UI08                  m_Reserved5;
    AP4_UI08                  m_ChromaBitDepth;
    AP4_UI16                  m_AverageFrameRate;
    AP4_UI08                  m_ConstantFrameRate;
    AP4_UI08                  m_NumTemporalLayers;
    AP4_UI08                  m_TemporalIdNested;
    AP4_UI08                  m_NaluLengthSize;
    AP4_Array<Sequence>       m_Sequences;
    AP4_DataBuffer            m_RawBytes;
};

#endif // _AP4_HVCC_ATOM_H_
