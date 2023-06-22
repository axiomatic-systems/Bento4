/*****************************************************************
|
|    AP4 - avcC Atoms 
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
#include "Ap4Av1cAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_Av1cAtom)

/*----------------------------------------------------------------------
|   AP4_Av1cAtom::GetProfileName
+---------------------------------------------------------------------*/
const char*
AP4_Av1cAtom::GetProfileName(AP4_UI08 profile)
{
    switch (profile) {
        case AP4_AV1_PROFILE_MAIN:         return "Main";
        case AP4_AV1_PROFILE_HIGH:         return "High";
        case AP4_AV1_PROFILE_PROFESSIONAL: return "Professional";
        default:                           return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_Av1cAtom::Create
+---------------------------------------------------------------------*/
AP4_Av1cAtom*
AP4_Av1cAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    if (payload_size < 4) {
        return NULL;
    }
    AP4_UI08 bits[4];
    AP4_Result result = stream.Read(bits, 4);
    if (AP4_FAILED(result)) {
        return NULL;
    }
    AP4_UI08 version                            = bits[0] & 0x7F;
    AP4_UI08 seq_profile                        = bits[1] >> 5;
    AP4_UI08 seq_level_idx_0                    = bits[1] & 0x1F;
    AP4_UI08 seq_tier_0                         = (bits[2] >> 7) & 1;
    AP4_UI08 high_bitdepth                      = (bits[2] >> 6) & 1;
    AP4_UI08 twelve_bit                         = (bits[2] >> 5) & 1;
    AP4_UI08 monochrome                         = (bits[2] >> 4) & 1;
    AP4_UI08 chroma_subsampling_x               = (bits[2] >> 3) & 1;
    AP4_UI08 chroma_subsampling_y               = (bits[2] >> 2) & 1;
    AP4_UI08 chroma_sample_position             = (bits[2]     ) & 3;
    AP4_UI08 initial_presentation_delay_present = (bits[3] >> 4) & 1;
    AP4_UI08 initial_presentation_delay_minus_one;
    if (initial_presentation_delay_present) {
        initial_presentation_delay_minus_one = (bits[3] >> 4) & 0x0F;
    } else {
        initial_presentation_delay_minus_one = 0;
    }
    
    AP4_DataBuffer config_obus;
    if (payload_size > 4) {
        config_obus.SetDataSize(payload_size - 4);
        result = stream.Read(config_obus.UseData(), payload_size - 4);
        if (AP4_FAILED(result)) {
            return NULL;
        }
    }
    return new AP4_Av1cAtom(version,
                            seq_profile,
                            seq_level_idx_0,
                            seq_tier_0,
                            high_bitdepth,
                            twelve_bit,
                            monochrome,
                            chroma_subsampling_x,
                            chroma_subsampling_y,
                            chroma_sample_position,
                            initial_presentation_delay_present,
                            initial_presentation_delay_minus_one,
                            config_obus.GetData(),
                            config_obus.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_Av1cAtom::AP4_Av1cAtom
+---------------------------------------------------------------------*/
AP4_Av1cAtom::AP4_Av1cAtom() :
    AP4_Atom(AP4_ATOM_TYPE_AV1C, AP4_ATOM_HEADER_SIZE),
    m_Version(1),
    m_SeqProfile(0),
    m_SeqLevelIdx0(0),
    m_SeqTier0(0),
    m_HighBitDepth(0),
    m_TwelveBit(0),
    m_Monochrome(0),
    m_ChromaSubsamplingX(0),
    m_ChromaSubsamplingY(0),
    m_ChromaSamplePosition(0),
    m_InitialPresentationDelayPresent(0),
    m_InitialPresentationDelayMinusOne(0)
{
    m_Size32 += 4;
}

/*----------------------------------------------------------------------
|   AP4_Av1cAtom::AP4_Av1cAtom
+---------------------------------------------------------------------*/
AP4_Av1cAtom::AP4_Av1cAtom(AP4_UI08        version,
                           AP4_UI08        seq_profile,
                           AP4_UI08        seq_level_idx_0,
                           AP4_UI08        seq_tier_0,
                           AP4_UI08        high_bitdepth,
                           AP4_UI08        twelve_bit,
                           AP4_UI08        monochrome,
                           AP4_UI08        chroma_subsampling_x,
                           AP4_UI08        chroma_subsampling_y,
                           AP4_UI08        chroma_sample_position,
                           AP4_UI08        initial_presentation_delay_present,
                           AP4_UI08        initial_presentation_delay_minus_one,
                           const AP4_UI08* config_obus,
                           AP4_Size        config_obus_size) :
    AP4_Atom(AP4_ATOM_TYPE_AV1C, AP4_ATOM_HEADER_SIZE),
    m_Version(version),
    m_SeqProfile(seq_profile),
    m_SeqLevelIdx0(seq_level_idx_0),
    m_SeqTier0(seq_tier_0),
    m_HighBitDepth(high_bitdepth),
    m_TwelveBit(twelve_bit),
    m_Monochrome(monochrome),
    m_ChromaSubsamplingX(chroma_subsampling_x),
    m_ChromaSubsamplingY(chroma_subsampling_y),
    m_ChromaSamplePosition(chroma_sample_position),
    m_InitialPresentationDelayPresent(initial_presentation_delay_present),
    m_InitialPresentationDelayMinusOne(initial_presentation_delay_minus_one)
{
    m_Size32 += 4 + config_obus_size;
    if (config_obus && config_obus_size) {
        m_ConfigObus.SetData(config_obus, config_obus_size);
    }
}

/*----------------------------------------------------------------------
|   AP4_Av1cAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Av1cAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_UI08 bits[4];
    bits[0] = (1 << 7) | m_Version;
    bits[1] = (m_SeqProfile << 5) | m_SeqLevelIdx0;
    bits[2] = (m_SeqTier0           << 7) |
              (m_HighBitDepth       << 6) |
              (m_TwelveBit          << 5) |
              (m_Monochrome         << 4) |
              (m_ChromaSubsamplingX << 3) |
              (m_ChromaSubsamplingY << 2) |
              (m_ChromaSamplePosition   );
    bits[3] = (m_InitialPresentationDelayPresent << 4) | m_InitialPresentationDelayMinusOne;
    AP4_Result result = stream.Write(bits, 4);
    if (AP4_FAILED(result)) return result;
    if (m_ConfigObus.GetDataSize()) {
        result = stream.Write(m_ConfigObus.GetData(), m_ConfigObus.GetDataSize());
        if (AP4_FAILED(result)) return result;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Av1cAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Av1cAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("version",                    m_Version);
    inspector.AddField("seq_profile",                m_SeqProfile);
    inspector.AddField("seq_level_idx_0",            m_SeqLevelIdx0);
    inspector.AddField("seq_tier_0",                 m_SeqTier0);
    inspector.AddField("high_bitdepth",              m_HighBitDepth);
    inspector.AddField("twelve_bit",                 m_TwelveBit);
    inspector.AddField("monochrome",                 m_Monochrome);
    inspector.AddField("chroma_subsampling_x",       m_ChromaSubsamplingX);
    inspector.AddField("chroma_subsampling_y",       m_ChromaSubsamplingY);
    inspector.AddField("chroma_sample_position",     m_ChromaSamplePosition);
    inspector.AddField("initial_presentation_delay", m_InitialPresentationDelayPresent ? m_InitialPresentationDelayMinusOne + 1 : 0);
    return AP4_SUCCESS;
}
