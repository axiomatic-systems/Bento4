/*****************************************************************
|
|    AP4 - vpcC Atoms
|
|    Copyright 2002-2020 Axiomatic Systems, LLC
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
#include "Ap4VpccAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"
#include "Ap4DataBuffer.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_VpccAtom)

/*----------------------------------------------------------------------
|   AP4_VpccAtom::Create
+---------------------------------------------------------------------*/
AP4_VpccAtom*
AP4_VpccAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;

    unsigned int payload_size = size - AP4_FULL_ATOM_HEADER_SIZE;
    if (payload_size < 8) {
        return NULL;
    }

    AP4_UI08 profile;
    stream.ReadUI08(profile);
    AP4_UI08 level;
    stream.ReadUI08(level);
    AP4_UI08 byte;
    stream.ReadUI08(byte);
    AP4_UI08 bit_depth = (byte >> 4) & 0XF;
    AP4_UI08 chroma_subsampling = (byte >> 1) & 7;
    bool video_full_range_flag = (byte & 1) != 0;
    AP4_UI08 colour_primaries;
    stream.ReadUI08(colour_primaries);
    AP4_UI08 transfer_characteristics;
    stream.ReadUI08(transfer_characteristics);
    AP4_UI08 matrix_coefficients;
    stream.ReadUI08(matrix_coefficients);
    AP4_UI16 codec_initialization_data_size = 0;
    stream.ReadUI16(codec_initialization_data_size);
    if (codec_initialization_data_size > payload_size - 8) {
        return NULL;
    }
    AP4_DataBuffer codec_initialization_data;
    AP4_Result result = codec_initialization_data.SetDataSize(codec_initialization_data_size);
    if (AP4_FAILED(result)) {
        return NULL;
    }
    return new AP4_VpccAtom(profile,
                            level,
                            bit_depth,
                            chroma_subsampling,
                            video_full_range_flag,
                            colour_primaries,
                            transfer_characteristics,
                            matrix_coefficients,
                            codec_initialization_data.GetData(),
                            codec_initialization_data.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_VpccAtom::AP4_VpccAtom
+---------------------------------------------------------------------*/
AP4_VpccAtom::AP4_VpccAtom(AP4_UI08        profile,
                           AP4_UI08        level,
                           AP4_UI08        bit_depth,
                           AP4_UI08        chroma_subsampling,
                           bool            videofull_range_flag,
                           AP4_UI08        colour_primaries,
                           AP4_UI08        transfer_characteristics,
                           AP4_UI08        matrix_coefficients,
                           const AP4_UI08* codec_initialization_data,
                           unsigned int    codec_initialization_data_size) :
    AP4_Atom(AP4_ATOM_TYPE_VPCC, AP4_FULL_ATOM_HEADER_SIZE + 8 + codec_initialization_data_size, 1, 0),
    m_Profile(profile),
    m_Level(level),
    m_BitDepth(bit_depth),
    m_ChromaSubsampling(chroma_subsampling),
    m_VideoFullRangeFlag(videofull_range_flag),
    m_ColourPrimaries(colour_primaries),
    m_TransferCharacteristics(transfer_characteristics),
    m_MatrixCoefficients(matrix_coefficients)
{
    if (codec_initialization_data && codec_initialization_data_size) {
        m_CodecIntializationData.SetData(codec_initialization_data, codec_initialization_data_size);
    }
}

/*----------------------------------------------------------------------
|   AP4_VpccAtom::GetCodecString
+---------------------------------------------------------------------*/
void
AP4_VpccAtom::GetCodecString(AP4_UI32 container_type, AP4_String& codec)
{
    char type_name[5];
    AP4_FormatFourChars(type_name, container_type);
    char string[64];
    AP4_FormatString(string,
                     sizeof(string),
                     "%s.%02d.%02d.%02d.%02d.%02d.%02d.%02d.%02d",
                     type_name,
                     m_Profile,
                     m_Level,
                     m_BitDepth,
                     m_ChromaSubsampling,
                     m_ColourPrimaries,
                     m_TransferCharacteristics,
                     m_MatrixCoefficients,
                     m_VideoFullRangeFlag ? 1 : 0);
    codec = string;
}

/*----------------------------------------------------------------------
|   AP4_VpccAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_VpccAtom::WriteFields(AP4_ByteStream& stream)
{
    stream.WriteUI08(m_Profile);
    stream.WriteUI08(m_Level);
    stream.WriteUI08((m_BitDepth << 4) | (m_ChromaSubsampling << 1) | (m_VideoFullRangeFlag ? 1 : 0));
    stream.WriteUI08(m_ColourPrimaries);
    stream.WriteUI08(m_TransferCharacteristics);
    stream.WriteUI08(m_MatrixCoefficients);
    stream.WriteUI16((AP4_UI16)m_CodecIntializationData.GetDataSize());
    stream.Write(m_CodecIntializationData.GetData(), m_CodecIntializationData.GetDataSize());

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_VpccAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_VpccAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("profile", m_Profile);
    inspector.AddField("level", m_Level);
    inspector.AddField("bit depth", m_BitDepth);
    inspector.AddField("chroma subsampling", m_ChromaSubsampling);
    inspector.AddField("video full range flag", m_VideoFullRangeFlag);
    inspector.AddField("colour primaries", m_ColourPrimaries);
    inspector.AddField("matrix coefficients", m_MatrixCoefficients);
    inspector.AddField("codec initialization data",
                       m_CodecIntializationData.GetData(),
                       m_CodecIntializationData.GetDataSize());
    return AP4_SUCCESS;
}
