/*****************************************************************
|
|    AP4 - dac4 Atoms
|
|    Copyright 2002-2018 Axiomatic Systems, LLC
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
#include "Ap4Dac4Atom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_Dac4Atom)

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Create
+---------------------------------------------------------------------*/
AP4_Dac4Atom*
AP4_Dac4Atom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;
    
    // check the version
    const AP4_UI08* payload = payload_data.GetData();
    return new AP4_Dac4Atom(size, payload);
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::AP4_Dac4Atom
+---------------------------------------------------------------------*/
AP4_Dac4Atom::AP4_Dac4Atom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_DAC4, size)
{
    // clear the DSI
    AP4_SetMemory(&m_Dsi, 0, sizeof(m_Dsi));
    
    // make a copy of our configuration bytes
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    m_RawBytes.SetData(payload, payload_size);

    // sanity check
    if (payload_size < 11) return;
    
    // parse the `ac4_dsi` or `ac4_dsi_v1` bits
    AP4_BitReader bits(payload, payload_size);
    m_Dsi.ac4_dsi_version = bits.ReadBits(3);
    if (m_Dsi.ac4_dsi_version == 0) {
        m_Dsi.d.v0.bitstream_version = bits.ReadBits(7);
        m_Dsi.d.v0.fs_index          = bits.ReadBits(1);
        m_Dsi.d.v0.frame_rate_index  = bits.ReadBits(4);
        m_Dsi.d.v0.n_presentations   = bits.ReadBits(9);

        // fill in computed fields
        m_Dsi.d.v0.fs = m_Dsi.d.v0.fs_index == 0 ? 44100 : 48000;
    } else if (m_Dsi.ac4_dsi_version == 1) {
        m_Dsi.d.v1.bitstream_version = bits.ReadBits(7);
        m_Dsi.d.v1.fs_index          = bits.ReadBits(1);
        m_Dsi.d.v1.frame_rate_index  = bits.ReadBits(4);
        m_Dsi.d.v1.n_presentations   = bits.ReadBits(9);
        if (m_Dsi.d.v1.bitstream_version > 1) {
            if (bits.ReadBit()) {
                m_Dsi.d.v1.short_program_id = bits.ReadBits(16);
                if (bits.ReadBit()) {
                    for (unsigned int i = 0; i < 16; i ++) {
                        m_Dsi.d.v1.program_uuid[i] = bits.ReadBits(8);
                    }
                }
            }
        }
        m_Dsi.d.v1.ac4_bitrate_dsi.bit_rate_mode      = bits.ReadBits(2);
        m_Dsi.d.v1.ac4_bitrate_dsi.bit_rate           = bits.ReadBits(32);
        m_Dsi.d.v1.ac4_bitrate_dsi.bit_rate_precision = bits.ReadBits(32);

        // byte align
        if (bits.GetBitsRead() % 8) {
            bits.SkipBits(8 - (bits.GetBitsRead() % 8));
        }
        
        // parse the presentations
        m_Dsi.d.v1.presentations = new Ac4Dsi::PresentationV1[m_Dsi.d.v1.n_presentations];
        AP4_SetMemory(m_Dsi.d.v1.presentations, 0, m_Dsi.d.v1.n_presentations * sizeof(m_Dsi.d.v1.presentations[0]));
        for (unsigned int i = 0; i < m_Dsi.d.v1.n_presentations; i++) {
            Ac4Dsi::PresentationV1& presentation = m_Dsi.d.v1.presentations[i];
            unsigned int bits_read_before_presentation = bits.GetBitsRead();
            
            presentation.presentation_version = bits.ReadBits(8);
            unsigned int pres_bytes           = bits.ReadBits(8);
            if (pres_bytes == 255) {
                pres_bytes += bits.ReadBits(16);
            }
            if (presentation.presentation_version == 0) {
                presentation.d.v0.presentation_config = bits.ReadBits(5);
                //bool b_add_emdf_substreams = false;
                if (presentation.d.v0.presentation_config == 0x06) {
                    //b_add_emdf_substreams = true;
                } else {
                    presentation.d.v0.mdcompat = bits.ReadBits(3);
                    if (bits.ReadBit()) { // b_presentation_group_index
                        presentation.d.v0.presentation_group_index = bits.ReadBits(5);
                    }
                    presentation.d.v0.dsi_frame_rate_multiply_info = bits.ReadBits(2);
                    presentation.d.v0.presentation_emdf_version    = bits.ReadBits(5);
                    presentation.d.v0.presentation_key_id          = bits.ReadBits(10);
                    presentation.d.v0.presentation_channel_mask    = bits.ReadBits(24);
                }
                // some fields skipped
            } else if (presentation.presentation_version == 1) {
                presentation.d.v1.presentation_config_v1 = bits.ReadBits(5);
                //bool b_add_emdf_substreams = false;
                if (presentation.d.v1.presentation_config_v1 == 0x06) {
                    //b_add_emdf_substreams = true;
                } else {
                    presentation.d.v1.mdcompat = bits.ReadBits(3);
                    if (bits.ReadBit()) { // b_presentation_group_index
                        presentation.d.v1.presentation_group_index = bits.ReadBits(5);
                    }
                    presentation.d.v1.dsi_frame_rate_multiply_info = bits.ReadBits(2);
                    presentation.d.v1.dsi_frame_rate_fraction_info = bits.ReadBits(2);
                    presentation.d.v1.presentation_emdf_version    = bits.ReadBits(5);
                    presentation.d.v1.presentation_key_id          = bits.ReadBits(10);
                    presentation.d.v1.b_presentation_channel_coded = bits.ReadBit();
                    if (presentation.d.v1.b_presentation_channel_coded) {
                        presentation.d.v1.dsi_presentation_ch_mode = bits.ReadBits(5);
                        if (presentation.d.v1.dsi_presentation_ch_mode == 11 ||
                            presentation.d.v1.dsi_presentation_ch_mode == 12 ||
                            presentation.d.v1.dsi_presentation_ch_mode == 13 ||
                            presentation.d.v1.dsi_presentation_ch_mode == 14) {
                            presentation.d.v1.pres_b_4_back_channels_present = bits.ReadBit();
                            presentation.d.v1.pres_top_channel_pairs         = bits.ReadBits(2);
                        }
                        presentation.d.v1.presentation_channel_mask_v1 = bits.ReadBits(24);
                    }
                }
                // some fields skipped
            }
            unsigned int bits_read_after_presentation = bits.GetBitsRead();
            unsigned int presentation_bytes = (bits_read_after_presentation - bits_read_before_presentation + 7) / 8;
            if (pres_bytes < presentation_bytes) {
                break;
            }
            unsigned int skip_bytes = pres_bytes - presentation_bytes;
            for (unsigned int skip = 0; skip < skip_bytes; skip++) {
                bits.SkipBits(8);
            }
        }
        
        // fill in computed fields
        m_Dsi.d.v1.fs = m_Dsi.d.v1.fs_index == 0 ? 44100 : 48000;
    } else {
        // not supported
        return;
    }
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::~AP4_Dac4Atom
+---------------------------------------------------------------------*/
AP4_Dac4Atom::~AP4_Dac4Atom()
{
    if (m_Dsi.ac4_dsi_version == 0) {
        //delete[] m_Dsi.d.v0.presentations;
    } else if (m_Dsi.ac4_dsi_version == 1) {
        delete[] m_Dsi.d.v1.presentations;
    }
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::WriteFields(AP4_ByteStream& stream)
{
    return stream.Write(m_RawBytes.GetData(), m_RawBytes.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::GetCodecString
+---------------------------------------------------------------------*/
void
AP4_Dac4Atom::GetCodecString(AP4_String& codec)
{
    AP4_UI08 bitstream_version    = 0;
    AP4_UI08 presentation_version = 0;
    AP4_UI08 mdcompat             = 0;
    if (m_Dsi.ac4_dsi_version == 0) {
        bitstream_version = m_Dsi.d.v0.bitstream_version;
    } else if (m_Dsi.ac4_dsi_version == 1) {
        bitstream_version = m_Dsi.d.v1.bitstream_version;
        if (m_Dsi.d.v1.n_presentations) {
            presentation_version = m_Dsi.d.v1.presentations[0].presentation_version;
            if (presentation_version == 0) {
                mdcompat = m_Dsi.d.v1.presentations[0].d.v0.mdcompat;
            } else if (presentation_version == 1) {
                mdcompat = m_Dsi.d.v1.presentations[0].d.v1.mdcompat;
            }
        }
    }
    char string[64];
    AP4_FormatString(string,
                     sizeof(string),
                     "ac-4.%02x.%02x.%02x",
                     bitstream_version,
                     presentation_version,
                     mdcompat);
    codec = string;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("ac4_dsi_version",    m_Dsi.ac4_dsi_version);
    if (m_Dsi.ac4_dsi_version == 0) {
        inspector.AddField("bitstream_version",  m_Dsi.d.v0.bitstream_version);
        inspector.AddField("fs_index",           m_Dsi.d.v0.fs_index);
        inspector.AddField("fs",                 m_Dsi.d.v0.fs);
        inspector.AddField("frame_rate_index",   m_Dsi.d.v0.frame_rate_index);
    } else if (m_Dsi.ac4_dsi_version == 1) {
        inspector.AddField("bitstream_version",  m_Dsi.d.v1.bitstream_version);
        inspector.AddField("fs_index",           m_Dsi.d.v1.fs_index);
        inspector.AddField("fs",                 m_Dsi.d.v1.fs);
        inspector.AddField("frame_rate_index",   m_Dsi.d.v1.frame_rate_index);
        if (m_Dsi.d.v1.bitstream_version > 1) {
            inspector.AddField("short_program_id", m_Dsi.d.v1.short_program_id);
            inspector.AddField("program_uuid",     m_Dsi.d.v1.program_uuid, 16, AP4_AtomInspector::HINT_HEX);
        }
        inspector.AddField("bit_rate_mode",      m_Dsi.d.v1.ac4_bitrate_dsi.bit_rate_mode);
        inspector.AddField("bit_rate",           m_Dsi.d.v1.ac4_bitrate_dsi.bit_rate);
        inspector.AddField("bit_rate_precision", m_Dsi.d.v1.ac4_bitrate_dsi.bit_rate_precision);
        
        for (unsigned int i = 0; i < m_Dsi.d.v1.n_presentations; i++) {
            Ac4Dsi::PresentationV1& presentation = m_Dsi.d.v1.presentations[i];
            char field_name[64];
            AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_version", i);
            inspector.AddField(field_name, presentation.presentation_version);
            if (presentation.presentation_version == 0) {
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_config", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_config);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].mdcompat", i);
                inspector.AddField(field_name, presentation.d.v0.mdcompat);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_group_index", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_group_index);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].dsi_frame_rate_multiply_info", i);
                inspector.AddField(field_name, presentation.d.v0.dsi_frame_rate_multiply_info);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_emdf_version", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_emdf_version);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_key_id", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_key_id);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_channel_mask", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_channel_mask, AP4_AtomInspector::HINT_HEX);
            } else if (presentation.presentation_version == 1) {
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_config_v1", i);
                inspector.AddField(field_name, presentation.d.v1.presentation_config_v1);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].mdcompat", i);
                inspector.AddField(field_name, presentation.d.v1.mdcompat);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_group_index", i);
                inspector.AddField(field_name, presentation.d.v1.presentation_group_index);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].dsi_frame_rate_multiply_info", i);
                inspector.AddField(field_name, presentation.d.v1.dsi_frame_rate_multiply_info);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].dsi_frame_rate_fraction_info", i);
                inspector.AddField(field_name, presentation.d.v1.dsi_frame_rate_fraction_info);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_emdf_version", i);
                inspector.AddField(field_name, presentation.d.v1.presentation_emdf_version);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_key_id", i);
                inspector.AddField(field_name, presentation.d.v1.presentation_key_id);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].b_presentation_channel_coded", i);
                inspector.AddField(field_name, presentation.d.v1.b_presentation_channel_coded);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].dsi_presentation_ch_mode", i);
                inspector.AddField(field_name, presentation.d.v1.dsi_presentation_ch_mode);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].pres_b_4_back_channels_present", i);
                inspector.AddField(field_name, presentation.d.v1.pres_b_4_back_channels_present);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].pres_top_channel_pairs", i);
                inspector.AddField(field_name, presentation.d.v1.pres_top_channel_pairs);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_channel_mask_v1", i);
                inspector.AddField(field_name, presentation.d.v1.presentation_channel_mask_v1, AP4_AtomInspector::HINT_HEX);
            }
        }
    }
    return AP4_SUCCESS;
}
