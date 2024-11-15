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

#include <cmath>

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
AP4_Dac4Atom::AP4_Dac4Atom(AP4_UI32 size, const Ac4Dsi* ac4Dsi):
    AP4_Atom(AP4_ATOM_TYPE_DAC4, AP4_ATOM_HEADER_SIZE){
    AP4_BitWriter bits(size);
    bits.Write(ac4Dsi->ac4_dsi_version, 3);
    bits.Write(ac4Dsi->d.v1.bitstream_version, 7);
    bits.Write(ac4Dsi->d.v1.fs_index, 1);
    bits.Write(ac4Dsi->d.v1.frame_rate_index, 4);

    // IMS presentation shall has the legacy presentation
    unsigned int add_n_presentations = 0;
    for (unsigned int idx = 0; idx < ac4Dsi->d.v1.n_presentations; idx++){
        if (ac4Dsi->d.v1.presentations[idx].presentation_version == 2) { 
            add_n_presentations++; 
        }
    }
    
    // Assume the total presentation numbers is less than 512 after adding legacy presentation
    bits.Write(ac4Dsi->d.v1.n_presentations + add_n_presentations, 9);

    if (ac4Dsi->d.v1.bitstream_version > 1){
        bits.Write(ac4Dsi->d.v1.b_program_id, 1);
        if (ac4Dsi->d.v1.b_program_id == 1){
            bits.Write(ac4Dsi->d.v1.short_program_id, 16);
            bits.Write(ac4Dsi->d.v1.b_uuid, 1);
            if (ac4Dsi->d.v1.b_uuid == 1) {
                for (unsigned int idx = 0; idx < 16; idx ++) {
                    bits.Write(ac4Dsi->d.v1.program_uuid[idx], 8);
                }
            }
        }
    }
    // ac4_bitrate_dsi()
    AP4_Dac4Atom::Ac4Dsi::Ac4BitrateDsi ac4_bitrate_dsi = ac4Dsi->d.v1.ac4_bitrate_dsi;
    ac4_bitrate_dsi.WriteBitrateDsi(bits);

    AP4_ByteAlign(bits);
    for (unsigned int idx = 0; idx < ac4Dsi->d.v1.n_presentations; idx ++) {
        unsigned int default_pres_bytes = 36;   // random value
        AP4_Dac4Atom::Ac4Dsi::PresentationV1 &presentation = ac4Dsi->d.v1.presentations[idx];
        bits.Write(presentation.presentation_version, 8);
        bits.Write(default_pres_bytes, 8);      // pres_bytes, need to be updated later
        //TODO: if (pres_bytes == 255), shall not happen now. Need the memory move function for bits
        unsigned int pres_bytes_idx = bits.GetBitCount() / 8 - 1;
        if (ac4Dsi->d.v1.n_presentations != 1 && presentation.d.v1.b_presentation_id == 0 && presentation.d.v1.b_extended_presentation_id == 0) {
            fprintf(stderr, "WARN: Need presentation_id for multiple presnetaion signal. The presentation of Presentation Index (PI) is %d miss presentation_id.\n", idx + 1);
        }
        if (presentation.presentation_version == 0){
            // TODO: support presentation version = 0
        }else if (presentation.presentation_version == 1 || presentation.presentation_version == 2){
            presentation.WritePresentationV1Dsi(bits);
            Ap4_Ac4UpdatePresBytes(bits.GetData(), pres_bytes_idx, bits.GetBitCount()/8 - pres_bytes_idx - 1);
        } else {
            Ap4_Ac4UpdatePresBytes(bits.GetData(), pres_bytes_idx, 0);
        }

        //legacy presentation for IMS
        if (presentation.presentation_version == 2) {
            AP4_Dac4Atom::Ac4Dsi::PresentationV1 legacy_presentation = presentation;
            if (legacy_presentation.d.v1.b_presentation_id == 0 && legacy_presentation.d.v1.b_extended_presentation_id == 0) {
                fprintf(stderr, "WARN: Need presentation_id for IMS signal.\n");
            }
            legacy_presentation.presentation_version = 1;
            legacy_presentation.d.v1.b_pre_virtualized = 0;
            legacy_presentation.d.v1.dolby_atmos_indicator = 0;

            bits.Write(legacy_presentation.presentation_version, 8);
            bits.Write(default_pres_bytes, 8);    // pres_bytes, need to be updated later
            //TODO: if (pres_bytes == 255), shall not happen now
            unsigned int pres_bytes_idx = bits.GetBitCount() / 8 - 1;
            legacy_presentation.WritePresentationV1Dsi(bits);
            Ap4_Ac4UpdatePresBytes(bits.GetData(), pres_bytes_idx, bits.GetBitCount()/8 - pres_bytes_idx - 1);
        }
    }
    m_RawBytes.SetData(bits.GetData(), bits.GetBitCount() / 8);
    m_Size32 += m_RawBytes.GetDataSize();
    // clear the DSI
    AP4_SetMemory(&m_Dsi, 0, sizeof(m_Dsi));
    m_Dsi.ac4_dsi_version = -1;
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
            
            presentation.presentation_version = bits.ReadBits(8);
            unsigned int pres_bytes           = bits.ReadBits(8);
            if (pres_bytes == 255) {
                pres_bytes += bits.ReadBits(16);
            }
            unsigned int bits_read_before_presentation = bits.GetBitsRead();
            if (presentation.presentation_version == 0) {
                presentation.d.v0.presentation_config = bits.ReadBits(5);
                //bool b_add_emdf_substreams = false;
                if (presentation.d.v0.presentation_config == 0x06) {
                    //b_add_emdf_substreams = true;
                } else {
                    presentation.d.v0.mdcompat = bits.ReadBits(3);
                    if (bits.ReadBit()) { // b_presentation_id
                        presentation.d.v0.presentation_id = bits.ReadBits(5);
                    }
                    presentation.d.v0.dsi_frame_rate_multiply_info = bits.ReadBits(2);
                    presentation.d.v0.presentation_emdf_version    = bits.ReadBits(5);
                    presentation.d.v0.presentation_key_id          = bits.ReadBits(10);
                    presentation.d.v0.presentation_channel_mask    = bits.ReadBits(24);
                }
                // some fields skipped
                if (bits.GetBitsRead() % 8) {
                    bits.SkipBits(8 - (bits.GetBitsRead() % 8));
                }
            } else if (presentation.presentation_version == 1 || presentation.presentation_version == 2) {
                presentation.d.v1.presentation_config_v1 = bits.ReadBits(5);
                if (presentation.d.v1.presentation_config_v1 == 0x06) {
                    presentation.d.v1.b_add_emdf_substreams = 1;
                } else {
                    presentation.d.v1.mdcompat = bits.ReadBits(3);
                    presentation.d.v1.b_presentation_id = bits.ReadBit();
                    if (presentation.d.v1.b_presentation_id) { 
                        presentation.d.v1.presentation_id = bits.ReadBits(5);
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
                    }else {
                        presentation.d.v1.presentation_channel_mask_v1 = 0x800000;
                    }
                    presentation.d.v1.b_presentation_core_differs = bits.ReadBit();
                    if (presentation.d.v1.b_presentation_core_differs) {
                        presentation.d.v1.b_presentation_core_channel_coded = bits.ReadBit();
                        if (presentation.d.v1.b_presentation_core_channel_coded) {
                            presentation.d.v1.dsi_presentation_channel_mode_core = bits.ReadBits(2);
                        }
                    }
                    presentation.d.v1.b_presentation_filter = bits.ReadBit();
                    if (presentation.d.v1.b_presentation_filter) {
                        presentation.d.v1.b_enable_presentation = bits.ReadBit();
                        presentation.d.v1.n_filter_bytes = bits.ReadBits(8);
                        for (int idx = 0; idx < presentation.d.v1.n_filter_bytes; idx ++) { bits.SkipBits(8); }
                    }
                    bool parse_substream_groups = true;
                    if (presentation.d.v1.presentation_config_v1 == 0x1f){
                        presentation.d.v1.n_substream_groups = 1;
                    } else {
                        presentation.d.v1.b_multi_pid = bits.ReadBit();
                        switch (presentation.d.v1.presentation_config_v1){
                            case 0:
                            case 1:
                            case 2:
                                presentation.d.v1.n_substream_groups = 2;
                                break;
                            case 3:
                            case 4:
                                presentation.d.v1.n_substream_groups = 3;
                                break;
                            case 5:
                                presentation.d.v1.n_substream_groups =  bits.ReadBits(3) + 2;
                                break;
                            default:
                                parse_substream_groups = false;
                                presentation.d.v1.n_skip_bytes = bits.ReadBits(7);
                                for (int idx = 0; idx < presentation.d.v1.n_skip_bytes; idx ++) { bits.SkipBits(8); }
                        }
                        
                    }
                    // Start to parse substream_groups
                    if (parse_substream_groups) {
                        presentation.d.v1.substream_groups = new Ac4Dsi::SubStreamGroupV1[presentation.d.v1.n_substream_groups];
                        AP4_SetMemory(presentation.d.v1.substream_groups, 0, presentation.d.v1.n_substream_groups * sizeof(presentation.d.v1.substream_groups[0]));
                        for (int cnt = 0; cnt < presentation.d.v1.n_substream_groups; cnt ++){
                             Ac4Dsi::SubStreamGroupV1& substreamgroup = presentation.d.v1.substream_groups[cnt];
                             substreamgroup.d.v1.b_substreams_present = bits.ReadBit();
                             substreamgroup.d.v1.b_hsf_ext            = bits.ReadBit();
                             substreamgroup.d.v1.b_channel_coded      = bits.ReadBit();
                             substreamgroup.d.v1.n_substreams         = bits.ReadBits(8);

                             substreamgroup.d.v1.substreams = new Ac4Dsi::SubStream[substreamgroup.d.v1.n_substreams];
                             AP4_SetMemory(substreamgroup.d.v1.substreams, 0, substreamgroup.d.v1.n_substreams * sizeof(substreamgroup.d.v1.substreams[0]));
                             for (int sus = 0; sus < substreamgroup.d.v1.n_substreams; sus ++){
                                 Ac4Dsi::SubStream& substream = substreamgroup.d.v1.substreams[sus];
                                 substream.dsi_sf_multiplier = bits.ReadBits(2);
                                 substream.b_substream_bitrate_indicator = bits.ReadBit();
                                 if (substream.b_substream_bitrate_indicator) {
                                    substream.substream_bitrate_indicator = bits.ReadBits(5);
                                 }
                                 if (substreamgroup.d.v1.b_channel_coded){
                                    substream.dsi_substream_channel_mask = bits.ReadBits(24);
                                 } else {
                                    substream.b_ajoc = bits.ReadBit();
                                    if (substream.b_ajoc) {
                                        substream.b_static_dmx = bits.ReadBit();
                                        if (substream.b_static_dmx == 0) {
                                            substream.n_dmx_objects_minus1 = bits.ReadBits(4);
                                        }
                                        substream.n_umx_objects_minus1 = bits.ReadBits(6);
                                    }
                                    substream.b_substream_contains_bed_objects = bits.ReadBit();
                                    substream.b_substream_contains_dynamic_objects = bits.ReadBit();
                                    substream.b_substream_contains_ISF_objects = bits.ReadBit();
                                    bits.SkipBit(); // reserved
                                }
                             }
                             substreamgroup.d.v1.b_content_type = bits.ReadBit();
                             if (substreamgroup.d.v1.b_content_type) {
                                 substreamgroup.d.v1.content_classifier = bits.ReadBits(3);
                                 substreamgroup.d.v1.b_language_indicator = bits.ReadBit();
                                 if (substreamgroup.d.v1.b_language_indicator) {
                                     substreamgroup.d.v1.n_language_tag_bytes = bits.ReadBits(6);
                                     for (int l = 0; l < substreamgroup.d.v1.n_language_tag_bytes; l++) {
                                         substreamgroup.d.v1.language_tag_bytes[l] = bits.ReadBits(8);
                                     }
                                 }
                             }
                        }
                    }
                    presentation.d.v1.b_pre_virtualized = bits.ReadBit();
                    presentation.d.v1.b_add_emdf_substreams = bits.ReadBit();
                }
                if (presentation.d.v1.b_add_emdf_substreams) {
                    presentation.d.v1.n_add_emdf_substreams = bits.ReadBits(7);
                    for (int j = 0; j < presentation.d.v1.n_add_emdf_substreams; j ++) {
                        presentation.d.v1.substream_emdf_version[j] = bits.ReadBits(5);
                        presentation.d.v1.substream_key_id[j] = bits.ReadBits(10);
                    }
                }
                presentation.d.v1.b_presentation_bitrate_info = bits.ReadBit();
                if (presentation.d.v1.b_presentation_bitrate_info) {
                    presentation.d.v1.ac4_bitrate_dsi.bit_rate_mode      = bits.ReadBits(2);
                    presentation.d.v1.ac4_bitrate_dsi.bit_rate           = bits.ReadBits(32);
                    presentation.d.v1.ac4_bitrate_dsi.bit_rate_precision = bits.ReadBits(32);
                }
                presentation.d.v1.b_alternative = bits.ReadBit();
                if (presentation.d.v1.b_alternative) {
                    // byte align
                    if (bits.GetBitsRead() % 8) {
                        bits.SkipBits(8 - (bits.GetBitsRead() % 8));
                    }

                    presentation.d.v1.alternative_info.name_len = bits.ReadBits(16);
                    for (int n = 0; n < presentation.d.v1.alternative_info.name_len; n++) { 
                        presentation.d.v1.alternative_info.presentation_name[n] = bits.ReadBits(8);
                    }
                    presentation.d.v1.alternative_info.n_targets = bits.ReadBits(5);
                    for (int t = 0; t < presentation.d.v1.alternative_info.n_targets; t++ ){
                        presentation.d.v1.alternative_info.target_md_compat[t] = bits.ReadBits(3);
                        presentation.d.v1.alternative_info.target_device_category[t] = bits.ReadBits(8); 
                    }
                }
                // byte align
                if (bits.GetBitsRead() % 8) {
                    bits.SkipBits(8 - (bits.GetBitsRead() % 8));
                }
                presentation.d.v1.de_indicator = bits.ReadBit();
                presentation.d.v1.dolby_atmos_indicator = bits.ReadBit();
                bits.SkipBits(4);
                presentation.d.v1.b_extended_presentation_id = bits.ReadBit();
                if (presentation.d.v1.b_extended_presentation_id){
                    presentation.d.v1.extended_presentation_id = bits.ReadBits(9);
                } else {
                    bits.SkipBit();
                }
            }
            unsigned int bits_read_after_presentation = bits.GetBitsRead();
            unsigned int presentation_bytes = (bits_read_after_presentation - bits_read_before_presentation) / 8;
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
        for (int i = 0; i < m_Dsi.d.v1.n_presentations; i++) {
            for (int j = 0; j < m_Dsi.d.v1.presentations[i].d.v1.n_substream_groups; j++) {
                delete[] m_Dsi.d.v1.presentations[i].d.v1.substream_groups[j].d.v1.substreams;
            }
            delete[] m_Dsi.d.v1.presentations[i].d.v1.substream_groups;
            delete[] m_Dsi.d.v1.presentations[i].d.v1.substream_group_indexs;
        }
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
            } else if (presentation_version == 1 || presentation_version == 2) {
                mdcompat = m_Dsi.d.v1.presentations[0].d.v1.mdcompat;
                for (int idx = 0; idx < m_Dsi.d.v1.n_presentations; idx ++){
                    if (mdcompat > m_Dsi.d.v1.presentations[idx].d.v1.mdcompat) {
                        mdcompat = m_Dsi.d.v1.presentations[idx].d.v1.mdcompat;
                    }
                }   
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
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_id", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_id);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].dsi_frame_rate_multiply_info", i);
                inspector.AddField(field_name, presentation.d.v0.dsi_frame_rate_multiply_info);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_emdf_version", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_emdf_version);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_key_id", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_key_id);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_channel_mask", i);
                inspector.AddField(field_name, presentation.d.v0.presentation_channel_mask, AP4_AtomInspector::HINT_HEX);
            } else if (presentation.presentation_version == 1 || presentation.presentation_version == 2) {
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_config_v1", i);
                inspector.AddField(field_name, presentation.d.v1.presentation_config_v1);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].mdcompat", i);
                inspector.AddField(field_name, presentation.d.v1.mdcompat);
                AP4_FormatString(field_name, sizeof(field_name), "[%02d].presentation_id", i);
                inspector.AddField(field_name, presentation.d.v1.presentation_id);
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

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::Ac4BitrateDsi::WriteBitrateDsi
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::Ac4BitrateDsi::WriteBitrateDsi(AP4_BitWriter &bits)
{
    bits.Write(bit_rate_mode, 2);
    bits.Write(bit_rate, 32);
    bits.Write(bit_rate_precision, 32);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::Ac4AlternativeInfo::WriteAlternativeInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::Ac4AlternativeInfo::WriteAlternativeInfo(AP4_BitWriter &bits)
{
    bits.Write(name_len, 16);
    for (unsigned int l = 0; l < name_len; l ++) {
        bits.Write(presentation_name[l], 8);
    }
    bits.Write(n_targets, 5);
    for (unsigned int t = 0; t < n_targets; t ++) {
        bits.Write(target_md_compat[t], 3);
        bits.Write(target_device_category[t], 8);
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ParseSubstreamInfoChan
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStream::ParseSubstreamInfoChan(AP4_BitReader &bits, 
                                                        unsigned int  presentation_version, 
                                                        unsigned char defalut_presentation_flag,
                                                        unsigned int  fs_idx,
                                                        unsigned int  &speaker_index_mask,
                                                        unsigned int  frame_rate_factor,
                                                        unsigned int  b_substreams_present,
                                                        unsigned char &dolby_atmos_indicator)
{
    ch_mode = ParseChMode(bits, presentation_version, dolby_atmos_indicator); 
    int substreamSpeakerGroupIndexMask = AC4_SPEAKER_GROUP_INDEX_MASK_BY_CH_MODE[ch_mode];
    if ((ch_mode >= CH_MODE_7_0_4) && (ch_mode <= CH_MODE_9_1_4)) {
        if (!(b_4_back_channels_present = bits.ReadBit())) {    // b_4_back_channels_present false
            substreamSpeakerGroupIndexMask &= ~0x8;             // Remove back channels (Lb,Rb) from mask
        }
        if (!(b_centre_present = bits.ReadBit())) {             // b_centre_present false
            substreamSpeakerGroupIndexMask &= ~0x2;             // Remove centre channel (C) from mask
        }
        switch (top_channels_present = bits.ReadBits(2)) {      // top_channels_present
            case 0:
                substreamSpeakerGroupIndexMask &= ~0x30;        // Remove top channels (Tfl,Tfr,Tbl,Tbr) from mask
                break;
            case 1:
            case 2:
                substreamSpeakerGroupIndexMask &= ~0x30;        // Remove top channels (Tfl,Tfr,Tbl,Tbr) from mask
                substreamSpeakerGroupIndexMask |=  0x80;        // Add top channels (Tl, Tr) from mask;
                break;
        }
    }
    dsi_substream_channel_mask = substreamSpeakerGroupIndexMask;
    // Only combine channel masks of substream groups that are part of the first/default presentation
    if (defalut_presentation_flag) {
        speaker_index_mask |= substreamSpeakerGroupIndexMask;
    }

    ParseDsiSfMutiplier(bits, fs_idx);

    b_substream_bitrate_indicator = bits.ReadBit();
    if (b_substream_bitrate_indicator) {    // b_bitrate_info
        // bitrate_indicator()
        ParseBitrateIndicator(bits);
    }

    if (ch_mode >= CH_MODE_70_52 && ch_mode <= CH_MODE_71_322) {
        bits.ReadBit();                     // add_ch_base
    }
    for (unsigned int i = 0; i < frame_rate_factor; i++) {
        bits.ReadBit();                     // b_audio_ndot
    }

    ParseSubstreamIdxInfo(bits, b_substreams_present);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ParseSubStreamInfoAjoc
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStream::ParseSubStreamInfoAjoc(AP4_BitReader &bits, 
                                                        unsigned int  &channel_count,
                                                        unsigned char defalut_presentation_flag,
                                                        unsigned int  fs_idx,
                                                        unsigned int  frame_rate_factor,
                                                        unsigned int  b_substreams_present)
{
    b_lfe = bits.ReadBit();     // b_lfe
    b_static_dmx = bits.ReadBit();
    if (b_static_dmx) {         // b_static_dmx
        if (defalut_presentation_flag) {
            channel_count += 5;
        }
    } else {
        n_dmx_objects_minus1 = bits.ReadBits(4);
        unsigned int nFullbandDmxSignals = n_dmx_objects_minus1 + 1;
        BedDynObjAssignment(bits, nFullbandDmxSignals, false);
        if (defalut_presentation_flag) {
            channel_count += nFullbandDmxSignals;
        }
    }
    if (bits.ReadBit()) {       // b_oamd_common_data_present
        // oamd_common_data()
        ParseOamdCommonData(bits);
    }
    int nFullbandUpmixSignalsMinus = bits.ReadBits(4);
    int nFullbandUpmixSignals = nFullbandUpmixSignalsMinus + 1;
    if (nFullbandUpmixSignals == 16) {
        nFullbandUpmixSignals += AP4_Ac4VariableBits(bits, 3);
    }
    n_umx_objects_minus1 = nFullbandUpmixSignals - 1;

    BedDynObjAssignment(bits, nFullbandUpmixSignals, true);
    ParseDsiSfMutiplier(bits, fs_idx);

    b_substream_bitrate_indicator = bits.ReadBit();
    if (b_substream_bitrate_indicator) {    // b_bitrate_info
        ParseBitrateIndicator(bits);
    }
    for (unsigned int i = 0; i< frame_rate_factor; i++) {
        bits.ReadBit();                     // b_audio_ndot
    }
    ParseSubstreamIdxInfo(bits, b_substreams_present);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ParseSubstreamInfoObj
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStream::ParseSubstreamInfoObj(AP4_BitReader &bits,
                                                       unsigned int  &channel_count,
                                                       unsigned char defalut_presentation_flag,
                                                       unsigned int  fs_idx,
                                                       unsigned int  frame_rate_factor,
                                                       unsigned int  b_substreams_present)
{
    int nObjectsCode = bits.ReadBits(3);
    if (defalut_presentation_flag) {
        switch (nObjectsCode) {
            case 0:
            case 1:
            case 2:
            case 3:
                channel_count += nObjectsCode;
                break;
            case 4:
                channel_count += 5;
                break;
            default:
                break;
                //TODO: Error
        }
    }
    if (bits.ReadBit()) {                       // b_dynamic_objects
        b_substream_contains_dynamic_objects = 1;
        unsigned int b_lfe = bits.ReadBit();    // b_lfe
        if (defalut_presentation_flag && b_lfe) { channel_count += 1; }
    }else {
        if (bits.ReadBit()) {                   // b_bed_objects
            b_substream_contains_bed_objects = 1;
            if (bits.ReadBit()) {               // b_bed_start
                if (bits.ReadBit()) {           // b_ch_assign_code
                    bits.ReadBits(3);           // bed_chan_assign_code
                }
                else {
                    if (bits.ReadBit()) {       // b_nonstd_bed_channel_assignment
                        bits.ReadBits(17);      // nonstd_bed_channel_assignment_mask
                    }
                    else {
                        bits.ReadBits(10);      // std_bed_channel_assignment_mask
                    }
                }
            }
        }else {
            if (bits.ReadBit()) {               // b_isf
                b_substream_contains_ISF_objects = 1;
                if (bits.ReadBit()) {           // b_isf_start
                    bits.ReadBits(3);           // isf_config
                }
            }else {
                int resBytes = bits.ReadBits(4);
                bits.ReadBits(resBytes * 8);
            }
        }
    }

    ParseDsiSfMutiplier(bits, fs_idx);

    b_substream_bitrate_indicator = bits.ReadBit();
    if (b_substream_bitrate_indicator) {        // b_bitrate_info
        ParseBitrateIndicator(bits);
    }
    for (unsigned int i = 0; i< frame_rate_factor; i++) {
        bits.ReadBit();                         // b_audio_ndot
    }
    ParseSubstreamIdxInfo(bits, b_substreams_present);
    return AP4_SUCCESS; 
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::WriteSubstreamDsi
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStream::WriteSubstreamDsi(AP4_BitWriter &bits, unsigned char b_channel_coded)
{
    bits.Write(dsi_sf_multiplier, 2);
    bits.Write(b_substream_bitrate_indicator, 1);
    if (b_substream_bitrate_indicator == 1) {
        bits.Write(substream_bitrate_indicator, 5);
    }
    if (b_channel_coded == 1) {
        bits.Write(dsi_substream_channel_mask, 24);
    }else {
        bits.Write(b_ajoc, 1);
        if (b_ajoc == 1) {
            bits.Write(b_static_dmx, 1);
            if (b_static_dmx == 0) {
                bits.Write(n_dmx_objects_minus1, 4);
            }
            bits.Write(n_umx_objects_minus1, 6);
        }
        // substream composition information
        bits.Write(b_substream_contains_bed_objects, 1);
        bits.Write(b_substream_contains_dynamic_objects, 1);
        bits.Write(b_substream_contains_ISF_objects, 1);
        bits.Write(0, 1); //reserved bit
    }
    return AP4_SUCCESS; 
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ParseChMode
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStream::ParseChMode(AP4_BitReader &bits, int presentationVersion, unsigned char &dolby_atmos_indicator) 
{
    int channel_mode_code = 0;

    channel_mode_code = bits.ReadBit();
    if (channel_mode_code == 0) {   // Mono 0b0
        return CH_MODE_MONO;
    }
    channel_mode_code = (channel_mode_code << 1) | bits.ReadBit();
    if (channel_mode_code == 2) {   // Stereo  0b10
        return CH_MODE_STEREO;
    }
    channel_mode_code = (channel_mode_code << 2) | bits.ReadBits(2);
    switch (channel_mode_code) {
        case 12:                    // 3.0 0b1100
            return CH_MODE_3_0;
        case 13:                    // 5.0 0b1101
            return CH_MODE_5_0;
        case 14:                    // 5.1 0b1110
            return CH_MODE_5_1;
    }
    channel_mode_code = (channel_mode_code << 3) | bits.ReadBits(3);
    switch (channel_mode_code) {
        case 120:                   // 0b1111000
            if (presentationVersion == 2) { // IMS (all content)
                return CH_MODE_STEREO;
            }
            else {                  // 7.0: 3/4/0
                return CH_MODE_70_34;
            }
        case 121:                   // 0b1111001
            if (presentationVersion == 2) { // IMS (Atmos content)
                dolby_atmos_indicator |= 1;
                return CH_MODE_STEREO;
            }
            else {                  // 7.1: 3/4/0.1
                return CH_MODE_71_34;
            }
        case 122:                   // 7.0: 5/2/0   0b1111010
            return CH_MODE_70_52;
        case 123:                   // 7.1: 5/2/0.1 0b1111011
            return CH_MODE_71_52;
        case 124:                   // 7.0: 3/2/2   0b1111100
            return CH_MODE_70_322; 
        case 125:                   // 7.1: 3/2/2.1 0b1111101
            return CH_MODE_71_322;
    }
    channel_mode_code = (channel_mode_code << 1) | bits.ReadBit();
    switch (channel_mode_code) {
        case 252:                   // 7.0.4 0b11111100
            return CH_MODE_7_0_4;
        case 253:                   // 7.1.4 0b11111101
            return CH_MODE_7_1_4;
    }
    channel_mode_code = (channel_mode_code << 1) | bits.ReadBit();
    switch (channel_mode_code) {
        case 508:                   // 9.0.4 0b111111100
            return CH_MODE_9_0_4;
        case 509:                   // 9.1.4 0b111111101
            return CH_MODE_9_1_4;
        case 510:                   // 22.2 0b111111110
            return CH_MODE_22_2;
        case 511:                   // Reserved, escape value 0b111111111
        default:
            AP4_Ac4VariableBits(bits, 2);
            return CH_MODE_RESERVED;
    }
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ParseDsiSfMutiplier
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStream::ParseDsiSfMutiplier(AP4_BitReader &bits,  
                                                     unsigned int  fs_idx)
{
    if (fs_idx == 1) {
        if (bits.ReadBit()) {                       // b_sf_miultiplier
            // 96 kHz or 192 kHz
            dsi_sf_multiplier = bits.ReadBit() + 1; // sf_multiplier
        }else{
            // 48 kHz
            dsi_sf_multiplier = 0; 
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::BedDynObjAssignment
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStream::BedDynObjAssignment(AP4_BitReader &bits, 
                                                     unsigned int  nSignals,
                                                     bool          is_upmix) 
{
    if (!bits.ReadBit()) {      // b_dyn_objects_only
        if (bits.ReadBit()) {   // b_isf
            unsigned char isf_config = bits.ReadBits(3); 
            if (is_upmix) {
                b_substream_contains_ISF_objects |= 1;
                if (nSignals > ObjNumFromIsfConfig(isf_config)) {
                    b_substream_contains_dynamic_objects |= 1;
                }
            }
        }else {
            if (bits.ReadBit()) {           // b_ch_assign_code
                unsigned char bed_chan_assign_code = bits.ReadBits(3); 
                if (is_upmix) {
                    b_substream_contains_bed_objects |= 1;
                    if (nSignals > BedNumFromAssignCode(bed_chan_assign_code)) {
                        b_substream_contains_dynamic_objects |= 1;
                    }
                }
            }else {
                if (bits.ReadBit()) {       // b_chan_assign_mask
                    if (bits.ReadBit()) {   // b_nonstd_bed_channel_assignment
                        unsigned int nonstd_bed_channel_assignment_mask = bits.ReadBits(17); 
                        if (is_upmix) {
                            unsigned int bed_num = BedNumFromNonStdMask(nonstd_bed_channel_assignment_mask);
                            if (bed_num > 0) { b_substream_contains_bed_objects |= 1; }
                            if (nSignals > bed_num) {
                                b_substream_contains_dynamic_objects |= 1;
                            }
                        }
                    }
                    else {
                        unsigned int std_bed_channel_assignment_mask = bits.ReadBits(10);
                        if (is_upmix) {
                            unsigned int bed_num = BedNumFromStdMask(std_bed_channel_assignment_mask);
                            if (bed_num > 0) { b_substream_contains_bed_objects |= 1; }
                            if (nSignals > bed_num) {
                                b_substream_contains_dynamic_objects |= 1;
                            }
                        }
                    }
                }else {
                    unsigned int nBedSignals = 0;
                    if (nSignals > 1) {
                        int bedChBits = (int) ceil(log((float)nSignals)/log((float)2));
                        nBedSignals = bits.ReadBits(bedChBits) + 1;
                    }
                    else {
                        nBedSignals = 1;
                    }
                    for (unsigned int b = 0; b < nBedSignals; b++) {
                        bits.ReadBits(4);   // nonstd_bed_channel_assignment
                    }
                    if (is_upmix) {
                        b_substream_contains_bed_objects |= 1;
                        if (nSignals > nBedSignals){ b_substream_contains_dynamic_objects |= 1; }
                    }
                }
            }
        }
    }else {
        if (is_upmix) {
            b_substream_contains_dynamic_objects |= 1;
            b_substream_contains_bed_objects     |= 0;
            b_substream_contains_ISF_objects     |= 0;
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ParseSubstreamIdxInfo
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStream::ParseSubstreamIdxInfo(AP4_BitReader &bits, unsigned int b_substreams_present)
{
    if (b_substreams_present == 1) {
        if (bits.ReadBits(2) == 3) {    // substream_index
            ss_idx = AP4_Ac4VariableBits(bits, 2);
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ParseBitrateIndicator
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStream::ParseBitrateIndicator(AP4_BitReader &bits)
{
    substream_bitrate_indicator = bits.ReadBits(3);
    if ((substream_bitrate_indicator & 0x1) == 1) { // bitrate_indicator
        substream_bitrate_indicator = (substream_bitrate_indicator << 2) + bits.ReadBits(2);
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ParseOamdCommonData
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStream::ParseOamdCommonData(AP4_BitReader &bits)
{
    if (bits.ReadBit() == 0) {  // b_default_screen_size_ratio
        bits.ReadBits(5);       // master_screen_size_ratio_code
    }
    bits.ReadBit();             // b_bed_object_chan_distribute
    if (bits.ReadBit()) {       // b_additional_data
        int addDataBytes = bits.ReadBit() + 1;
        if (addDataBytes == 2) {
            addDataBytes += AP4_Ac4VariableBits(bits, 2);
        }
        unsigned int bits_used = Trim(bits);
        bits_used += BedRendeInfo(bits);
        bits.ReadBits(addDataBytes * 8 - bits_used);
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::Trim
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStream::Trim(AP4_BitReader &bits)
{
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::BedRendeInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStream::BedRendeInfo(AP4_BitReader &bits)
{
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::ObjNumFromIsfConfig
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Dac4Atom::Ac4Dsi::SubStream::ObjNumFromIsfConfig(unsigned char isf_config)
{
    unsigned int obj_num = 0;
    switch (isf_config){
        case 0: obj_num = 4 ; break;
        case 1: obj_num = 8 ; break;
        case 2: obj_num = 10; break;
        case 3: obj_num = 14; break;
        case 4: obj_num = 15; break;
        case 5: obj_num = 30; break;
        default: obj_num = 0;
    }
    return obj_num;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::BedNumFromAssignCode
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Dac4Atom::Ac4Dsi::SubStream::BedNumFromAssignCode(unsigned char assign_code)
{
    unsigned int bed_num = 0;
    switch (assign_code){
        case 0: bed_num = 2 ; break;
        case 1: bed_num = 3 ; break;
        case 2: bed_num = 6 ; break;
        case 3: bed_num = 8 ; break;
        case 4: bed_num = 10; break;
        case 5: bed_num = 8 ; break;
        case 6: bed_num = 10; break;
        case 7: bed_num = 12; break;
        default: bed_num = 0;
    }
    return bed_num;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::BedNumFromNonStdMask
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Dac4Atom::Ac4Dsi::SubStream::BedNumFromNonStdMask(unsigned int non_std_mask)
{
    unsigned int bed_num = 0;
    // Table 85: nonstd_bed_channel_assignment AC-4 part-2 v1.2.1
    for (unsigned int idx = 0; idx < 17; idx ++) {
        if ((non_std_mask >> idx) & 0x1){
            bed_num ++;
        }
    }
    return bed_num;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::BedNumFromStdMask
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Dac4Atom::Ac4Dsi::SubStream::BedNumFromStdMask(unsigned int std_mask)
{
    unsigned int bed_num = 0;
    // Table 86 std_bed_channel_assignment_flag[] AC-4 part-2 v1.2.1
    for (unsigned int idx = 0; idx < 10; idx ++) {
        if ((std_mask >> idx) & 0x1){
            if ((idx == 1) || (idx == 2) || (idx == 9)) { bed_num ++;}
            else { bed_num += 2; }
        }
    }
    return bed_num;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStream::GetChModeCore
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStream::GetChModeCore(unsigned char b_channel_coded)
{
    int ch_mode_core = -1;
    if (b_channel_coded == 0 && 
        b_ajoc          == 1 && 
        b_static_dmx    == 1 && 
        b_lfe           == 0){
        ch_mode_core = 3 ;
    }else if (b_channel_coded == 0 && 
              b_ajoc          == 1 && 
              b_static_dmx    == 1 && 
              b_lfe           == 1){
              ch_mode_core = 4 ;
    }else if ((b_channel_coded == 1) && 
              (ch_mode == 11 || ch_mode == 13)){
              ch_mode_core = 5 ;
    }else if ((b_channel_coded == 1) && 
              (ch_mode == 12 || ch_mode == 14)){
              ch_mode_core = 6 ;
    }
    return ch_mode_core;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::ParseSubstreamGroupInfo
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::ParseSubstreamGroupInfo(AP4_BitReader& bits,
                                                                unsigned int   bitstream_version,
                                                                unsigned int   presentation_version,
                                                                unsigned char  defalut_presentation_flag,
                                                                unsigned int   frame_rate_factor,
                                                                unsigned int   fs_idx,
                                                                unsigned int&  channel_count,
                                                                unsigned int&  speaker_index_mask,
                                                                unsigned int&  b_obj_or_Ajoc)
{
    d.v1.b_substreams_present = bits.ReadBit();
    d.v1.b_hsf_ext = bits.ReadBit();
    if (bits.ReadBit()) {
        d.v1.n_substreams = 1;
    } else{
        d.v1.n_substreams = bits.ReadBits(2) + 2;
        if (d.v1.n_substreams == 5) {
            d.v1.n_substreams += AP4_Ac4VariableBits(bits, 2);
        }  
    }
    d.v1.substreams = new AP4_Dac4Atom::Ac4Dsi::SubStream[d.v1.n_substreams];
    AP4_SetMemory(d.v1.substreams, 0, d.v1.n_substreams * sizeof(d.v1.substreams[0]));
    d.v1.b_channel_coded = bits.ReadBit();
    if (d.v1.b_channel_coded) {
        for (unsigned int sus = 0; sus < d.v1.n_substreams; sus++) {
            AP4_Dac4Atom::Ac4Dsi::SubStream& substream = d.v1.substreams[sus];
            if (bitstream_version == 1) { 
                bits.ReadBit(); // sus_ver
            }

            // ac4_substream_info_chan()
            substream.ParseSubstreamInfoChan(bits,
                                             presentation_version, 
                                             defalut_presentation_flag,
                                             fs_idx,
                                             speaker_index_mask,
                                             frame_rate_factor,
                                             d.v1.b_substreams_present,
                                             d.v1.dolby_atmos_indicator);
            
            if (d.v1.b_hsf_ext) {
                // ac4_hsf_ext_substream_info()
                ParseHsfExtSubstreamInfo(bits);
            }
        }
    } else {     // b_channel_coded == 0
        b_obj_or_Ajoc = 1;
        if (bits.ReadBit()) {       // b_oamd_substream
                //oamd_substream_info()
            ParseOamdSubstreamInfo(bits);
        }
        for (int sus = 0; sus < d.v1.n_substreams; sus++) {
            AP4_Dac4Atom::Ac4Dsi::SubStream& substream = d.v1.substreams[sus];
            substream.b_ajoc = bits.ReadBit();
            unsigned int local_channel_count = 0;
            if (substream.b_ajoc) { // b_ajoc
                // ac4_substream_info_ajoc()
                substream.ParseSubStreamInfoAjoc(bits,
                                                 local_channel_count,
                                                 defalut_presentation_flag,
                                                 fs_idx,
                                                 frame_rate_factor,
                                                 d.v1.b_substreams_present);

                if (d.v1.b_hsf_ext) {
                    // ac4_hsf_ext_substream_info()
                    ParseHsfExtSubstreamInfo(bits);
                }
            }else {
                // ac4_substream_info_obj()
                substream.ParseSubstreamInfoObj(bits,
                                                local_channel_count,
                                                defalut_presentation_flag,
                                                fs_idx,
                                                frame_rate_factor,
                                                d.v1.b_substreams_present);

                if (d.v1.b_hsf_ext) {
                    // ac4_hsf_ext_substream_info()
                    ParseHsfExtSubstreamInfo(bits);
                }
            }
            if (channel_count < local_channel_count) { channel_count = local_channel_count;}
        }
    }

    d.v1.b_content_type = bits.ReadBit();
    if (d.v1.b_content_type) { 
        // content_type()
        ParseContentType(bits);
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::WriteSubstreamGroupDsi
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::WriteSubstreamGroupDsi(AP4_BitWriter &bits)
{
    bits.Write(d.v1.b_substreams_present, 1);
    bits.Write(d.v1.b_hsf_ext, 1);
    bits.Write(d.v1.b_channel_coded, 1);
    bits.Write(d.v1.n_substreams, 8);
    for (unsigned int sg = 0; sg < d.v1.n_substreams; sg++ ){
        d.v1.substreams[sg].WriteSubstreamDsi(bits, d.v1.b_channel_coded);
    }
    WriteContentType(bits);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::ParseOamdSubstreamInfo
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::ParseOamdSubstreamInfo(AP4_BitReader &bits)
{
    bits.ReadBit();                     // b_oamd_ndot
    if (d.v1.b_substreams_present == 1) {
        if (bits.ReadBits(2) == 3) {    // substream_index
            AP4_Ac4VariableBits(bits, 2);
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::ParseHsfExtSubstreamInfo
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::ParseHsfExtSubstreamInfo(AP4_BitReader &bits)
{
    if (d.v1.b_substreams_present == 1) {
        if (bits.ReadBits(2) == 3) {    // substream_index
            AP4_Ac4VariableBits(bits, 2);
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::ParseContentType
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::ParseContentType(AP4_BitReader &bits)
{
    d.v1.content_classifier = bits.ReadBits(3); // content_classifier
    d.v1.b_language_indicator = bits.ReadBit();
    if (d.v1.b_language_indicator == 1) {       // b_language_indicator
        if (bits.ReadBit()) {                   // b_serialized_language_tag
            bits.ReadBits(17);                  // b_start_tag, language_tag_chunk
        }
        else {
            d.v1.n_language_tag_bytes= bits.ReadBits(6);
            for (unsigned int l = 0; l < d.v1.n_language_tag_bytes; l++){
                d.v1.language_tag_bytes[l] = bits.ReadBits(8);
            }
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::WriteContentType
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1::WriteContentType(AP4_BitWriter &bits)
{
    bits.Write(d.v1.b_content_type, 1);
    if (d.v1.b_content_type == 1){
        bits.Write(d.v1.content_classifier, 3);
        bits.Write(d.v1.b_language_indicator, 1);
        if (d.v1.b_language_indicator == 1){
            bits.Write(d.v1.n_language_tag_bytes, 6);
            for (unsigned int idx = 0; idx < d.v1.n_language_tag_bytes; idx++ ){
                bits.Write(d.v1.language_tag_bytes[idx], 8);
            }
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::Parse
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParsePresentationV1Info(AP4_BitReader &bits, 
                                                              unsigned int  bitstream_version,
                                                              unsigned int  frame_rate_idx,
                                                              unsigned int  pres_idx,
                                                              unsigned int  &max_group_index,
                                                              unsigned int  **first_pres_sg_index,
                                                              unsigned int  &first_pres_sg_num)
{
    unsigned int *substreamGroupIndexes = new unsigned int[AP4_AC4_SUBSTREAM_GROUP_NUM];
    unsigned int b_single_substream_group = bits.ReadBit();
    if (b_single_substream_group != 1){
        d.v1.presentation_config_v1 = bits.ReadBits(3);
        if (d.v1.presentation_config_v1 == 7) {
            d.v1.presentation_config_v1 += AP4_Ac4VariableBits(bits, 2);
        }
    }else {
        d.v1.presentation_config_v1 = 0x1f;
    }

    // presentation_version()
    ParsePresentationVersion(bits, bitstream_version);

    AP4_Ac4EmdfInfo tmp_emdf_info;
    if (b_single_substream_group != 1 && d.v1.presentation_config_v1 == 6){
        d.v1.b_add_emdf_substreams = 1;
    }else{
        if (bitstream_version != 1){
            d.v1.mdcompat = bits.ReadBits(3);
        }
        d.v1.b_presentation_id = bits.ReadBit();
        if (d.v1.b_presentation_id) {
            d.v1.presentation_id = AP4_Ac4VariableBits(bits, 2);
        }
        // frame_rate_multiply_info()
        ParseDSIFrameRateMultiplyInfo(bits, frame_rate_idx);

        // frame_rate_fractions_info()
        ParseDSIFrameRateFractionsInfo(bits, frame_rate_idx);

        // emdf_info()
        ParseEmdInfo(bits, tmp_emdf_info);
        d.v1.presentation_emdf_version = tmp_emdf_info.emdf_version;
        d.v1.presentation_key_id = tmp_emdf_info.key_id;
           
        // b_presentation_filter
        d.v1.b_presentation_filter = bits.ReadBit();
        if (d.v1.b_presentation_filter == 1){
            d.v1.b_enable_presentation = bits.ReadBit();
        }
        if (b_single_substream_group == 1){
            substreamGroupIndexes[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
            max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[0], max_group_index);
            d.v1.n_substream_groups = 1;
            d.v1.substream_group_indexs = substreamGroupIndexes;
        }else {
            d.v1.b_multi_pid = bits.ReadBit();
            switch (d.v1.presentation_config_v1){
                case 0: // M&E + D
                case 2: // Main + Assoc
                    substreamGroupIndexes[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    substreamGroupIndexes[1] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[0], max_group_index);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[1], max_group_index);
                    d.v1.n_substream_groups = 2;
                    d.v1.substream_group_indexs = substreamGroupIndexes;
                    break;
                case 1: // Main + DE
                    // shall return same substream group index, need stream to verify
                    substreamGroupIndexes[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    substreamGroupIndexes[1] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[0], max_group_index);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[1], max_group_index);
                    d.v1.n_substream_groups = 2;
                    d.v1.substream_group_indexs = substreamGroupIndexes;
                    break;
                case 3: // M&E + D + Assoc
                    substreamGroupIndexes[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    substreamGroupIndexes[1] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    substreamGroupIndexes[2] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[0], max_group_index);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[1], max_group_index);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[2], max_group_index);
                    d.v1.n_substream_groups = 3;
                    d.v1.substream_group_indexs = substreamGroupIndexes;
                    break;
                case 4: // Main + DE + Assoc
                    // shall return only two substream group index, need stream to verify
                    substreamGroupIndexes[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    substreamGroupIndexes[1] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    substreamGroupIndexes[2] = ParseAc4SgiSpecifier(bits, bitstream_version);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[0], max_group_index);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[1], max_group_index);
                    max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[2], max_group_index);
                    d.v1.n_substream_groups = 3;
                    d.v1.substream_group_indexs = substreamGroupIndexes;
                    break;
                case 5:
                    d.v1.n_substream_groups = bits.ReadBits(2) + 2;
                    if (d.v1.n_substream_groups == 5) {
                        d.v1.n_substream_groups += AP4_Ac4VariableBits(bits, 2);
                    }
                    delete[] substreamGroupIndexes;
                    substreamGroupIndexes = new unsigned int[d.v1.n_substream_groups];
                    for (unsigned int sg = 0; sg < d.v1.n_substream_groups; sg ++){
                        substreamGroupIndexes[sg] = ParseAc4SgiSpecifier(bits, bitstream_version);
                        max_group_index = AP4_SetMaxGroupIndex(substreamGroupIndexes[sg], max_group_index);
                    }
                    d.v1.substream_group_indexs = substreamGroupIndexes;
                    break;
                default:
                    // presentation_config_ext_info()
                    ParsePresentationConfigExtInfo(bits, bitstream_version);
                    break;
            }
        }
        // b_pre_virtualized
        d.v1.b_pre_virtualized     = bits.ReadBit();
        d.v1.b_add_emdf_substreams = bits.ReadBit();
        // ac4_presentation_substream_info()
        ParsePresentationSubstreamInfo(bits);
    }

    if (d.v1.b_add_emdf_substreams){
        d.v1.n_add_emdf_substreams = bits.ReadBits(2);
        if (d.v1.n_add_emdf_substreams == 0){
            d.v1.n_add_emdf_substreams = AP4_Ac4VariableBits(bits, 2) + 4;
        }
        for (unsigned int cnt = 0; cnt < d.v1.n_add_emdf_substreams; cnt ++){
            ParseEmdInfo(bits, tmp_emdf_info);
            d.v1.substream_emdf_version[cnt] = tmp_emdf_info.emdf_version;
            d.v1.substream_key_id[cnt]       = tmp_emdf_info.key_id;
        }
    }
    if (pres_idx == 0){
        *first_pres_sg_index = substreamGroupIndexes;
        first_pres_sg_num = d.v1.n_substream_groups;
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::WritePresentationV1Dsi
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::WritePresentationV1Dsi(AP4_BitWriter &bits)
{
    bits.Write(d.v1.presentation_config_v1, 5);
    if (d.v1.presentation_config_v1 == 0x06) {
        d.v1.b_add_emdf_substreams = 1;
    }else {
        bits.Write(d.v1.mdcompat, 3);
        bits.Write(d.v1.b_presentation_id, 1);
        if (d.v1.b_presentation_id == 1){
            bits.Write(d.v1.presentation_id, 5);
        }
        bits.Write(d.v1.dsi_frame_rate_multiply_info, 2);
        bits.Write(d.v1.dsi_frame_rate_fraction_info, 2);
        bits.Write(d.v1.presentation_emdf_version, 5);
        bits.Write(d.v1.presentation_key_id, 10);
        d.v1.b_presentation_channel_coded = ((GetPresentationChMode() == -1) ? 0: 1);
        bits.Write(d.v1.b_presentation_channel_coded, 1);
        if (d.v1.b_presentation_channel_coded == 1) {
            d.v1.dsi_presentation_ch_mode = GetPresentationChMode();
            bits.Write(d.v1.dsi_presentation_ch_mode, 5);
            if (d.v1.dsi_presentation_ch_mode >= 11 && d.v1.dsi_presentation_ch_mode <= 14){
                GetPresB4BackChannelsPresent();
                GetPresTopChannelPairs();
                bits.Write(d.v1.pres_b_4_back_channels_present, 1);
                bits.Write(d.v1.pres_top_channel_pairs, 2);
                if (d.v1.pres_top_channel_pairs) {
                    d.v1.dolby_atmos_indicator = 1;
                }
            }
            d.v1.presentation_channel_mask_v1 = GetPresentationChannelMask();
            bits.Write(d.v1.presentation_channel_mask_v1, 24);
        }
        int pres_ch_mode_core = GetBPresentationCoreDiffers();
        bits.Write(d.v1.b_presentation_core_differs = ((pres_ch_mode_core == -1) ? 0: 1), 1);
        if (d.v1.b_presentation_core_differs == 1) {
            bits.Write(d.v1.b_presentation_core_channel_coded = ((pres_ch_mode_core == -1) ? 0: 1), 1);
            if (d.v1.b_presentation_core_channel_coded == 1) {
                bits.Write(d.v1.dsi_presentation_channel_mode_core = (pres_ch_mode_core - 3), 2);
            }
        }
        bits.Write(d.v1.b_presentation_filter, 1);
        if (d.v1.b_presentation_filter == 1) {
            bits.Write(d.v1.b_enable_presentation, 1);
            // Encoders implemented according to the present document shall write the value 0
            d.v1.n_filter_bytes = 0; 
            bits.Write(d.v1.n_filter_bytes, 8);
            //TODO: filter_data if n_filter_bytes > 0
        }

        if (d.v1.presentation_config_v1 == 0x1f){
            d.v1.substream_groups[0].WriteSubstreamGroupDsi(bits);
        }else {
            bits.Write(d.v1.b_multi_pid, 1);
            if (d.v1.presentation_config_v1 <= 2){
                d.v1.substream_groups[0].WriteSubstreamGroupDsi(bits);
                d.v1.substream_groups[1].WriteSubstreamGroupDsi(bits);
            }
            if (d.v1.presentation_config_v1 >= 3 && d.v1.presentation_config_v1 <= 4){
                d.v1.substream_groups[0].WriteSubstreamGroupDsi(bits);
                d.v1.substream_groups[1].WriteSubstreamGroupDsi(bits);
                d.v1.substream_groups[2].WriteSubstreamGroupDsi(bits);
            }
            if (d.v1.presentation_config_v1 == 5) {
                bits.Write(d.v1.n_substream_groups - 2, 3);
                for (unsigned int sg = 0; sg < d.v1.n_substream_groups; sg ++) {
                    d.v1.substream_groups[sg].WriteSubstreamGroupDsi(bits);
                }
            }
            if (d.v1.presentation_config_v1 > 5) {
                d.v1.n_skip_bytes = 0;
                bits.Write(d.v1.n_skip_bytes, 7);
                //TODO: skip_data if n_skip_bytes > 0
            }
        }
        
        // IMS shall set the b_pre_virtualized = 1
        if (presentation_version == 2) {
            d.v1.b_pre_virtualized = 1;
        }
        bits.Write(d.v1.b_pre_virtualized, 1);
        bits.Write(d.v1.b_add_emdf_substreams, 1);
    }
    if (d.v1.b_add_emdf_substreams == 1){
        bits.Write(d.v1.n_add_emdf_substreams, 7);
        for (unsigned int j = 0; j < d.v1.n_add_emdf_substreams; j++){
            bits.Write(d.v1.substream_emdf_version[j], 5);
            bits.Write(d.v1.substream_key_id[j], 10);
        }
    }
    // TODO: Not implement, can't get the informaiton from TOC, need calcuate by the muxer
    bits.Write(d.v1.b_presentation_bitrate_info, 1);
    if (d.v1.b_presentation_bitrate_info == 1) {
        d.v1.ac4_bitrate_dsi.WriteBitrateDsi(bits);
    }

    bits.Write(d.v1.b_alternative, 1);
    if (d.v1.b_alternative == 1) {
        AP4_ByteAlign(bits);
        // TODO: Not implement, need the information from ac4_presentation_substream
        d.v1.alternative_info.WriteAlternativeInfo(bits);        
    }
    AP4_ByteAlign(bits);

    /*
     * TODO: Not implement, need the information from ac4_substream.
     * Currently just set the value to 1 according to Dolby's internal discussion.
     */
    d.v1.de_indicator = 1;
    bits.Write(d.v1.de_indicator, 1);

    bits.Write(d.v1.dolby_atmos_indicator, 1);
    bits.Write(0, 4);       //reserved bits

    if (d.v1.presentation_id > 31) {
        d.v1.b_extended_presentation_id = 1;
        d.v1.extended_presentation_id = d.v1.presentation_id;
    }
    bits.Write(d.v1.b_extended_presentation_id, 1);
    if (d.v1.b_extended_presentation_id == 1) {
        bits.Write(d.v1.extended_presentation_id, 9);
    }else {
        bits.Write(0, 1);   //reserved bit
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParsePresentationVersion
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParsePresentationVersion(AP4_BitReader& bits, unsigned int bitstream_version)
{
    presentation_version = 0;
    if (bitstream_version != 1){
        while(bits.ReadBit() == 1){
            presentation_version ++;
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParsePresentationConfigExtInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParsePresentationConfigExtInfo(AP4_BitReader &bits, unsigned int bitstream_version)
{
    unsigned int nSkipBytes = bits.ReadBits(5);
    if (bits.ReadBit()) {  // b_more_skip_bytes
        nSkipBytes += (AP4_Ac4VariableBits(bits, 2) << 5);
    }
    if (bitstream_version == 1 && d.v1.presentation_config_v1 == 7) {
        // TODO: refer to chapte 6.2.1.5 - TS 103 190-2
    }
    bits.ReadBits(nSkipBytes * 8);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParseAc4SgiSpecifier
+---------------------------------------------------------------------*/
AP4_UI32 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParseAc4SgiSpecifier(AP4_BitReader &bits, unsigned int bitstream_version) 
{
    if (bitstream_version  == 1) {
        // Error
        return 0;
    } else{
        unsigned int groupIndex = bits.ReadBits(3);
        if (groupIndex == 7) {
            groupIndex += AP4_Ac4VariableBits(bits, 2);
        }
        return groupIndex;
    }
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParseDSIFrameRateMultiplyInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParseDSIFrameRateMultiplyInfo(AP4_BitReader &bits, unsigned int frame_rate_idx)
{
    switch (frame_rate_idx) {
        case 2:
        case 3:
        case 4:
            if (bits.ReadBit()) {                               // b_multiplier
                unsigned int multiplier_bit = bits.ReadBit();   //multiplier_bit
                d.v1.dsi_frame_rate_multiply_info = (multiplier_bit == 0)? 1: 2;
            }
            else {
                d.v1.dsi_frame_rate_multiply_info = 0;
            }
            break;
        case 0:
        case 1:
        case 7:
        case 8:
        case 9:
            if (bits.ReadBit()) {
                d.v1.dsi_frame_rate_multiply_info = 1;
            }else {
                d.v1.dsi_frame_rate_multiply_info = 0;
            }
            break;
        default:
            d.v1.dsi_frame_rate_multiply_info = 0;
            break;
    }
    return AP4_SUCCESS;
}


/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParseDSIFrameRateFractionsInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParseDSIFrameRateFractionsInfo(AP4_BitReader &bits, unsigned int frame_rate_idx)
{
     if (frame_rate_idx >= 5 && frame_rate_idx <= 9) {
        if (bits.ReadBit() == 1) {      // b_frame_rate_fraction
            d.v1.dsi_frame_rate_fraction_info = 1;
        }else{
            d.v1.dsi_frame_rate_fraction_info = 0;
        }
    }else if (frame_rate_idx >= 10 && frame_rate_idx <= 12){
        if (bits.ReadBit() == 1) {      // b_frame_rate_fraction
            if (bits.ReadBit() == 1) {  // b_frame_rate_fraction_is_4
                d.v1.dsi_frame_rate_fraction_info = 2;
            } else {
                d.v1.dsi_frame_rate_fraction_info = 1;
            }
        
        }else{
            d.v1.dsi_frame_rate_fraction_info = 0;
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParseEmdInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParseEmdInfo(AP4_BitReader &bits, AP4_Ac4EmdfInfo &emdf_info)
{
    emdf_info.emdf_version = bits.ReadBits(2);
    if (emdf_info.emdf_version == 3) { 
        emdf_info.emdf_version += AP4_Ac4VariableBits(bits, 2);
    }
    emdf_info.key_id = bits.ReadBits(3);
    if (emdf_info.key_id == 7) { 
        emdf_info.key_id += AP4_Ac4VariableBits(bits, 3);
    }
    emdf_info.b_emdf_payloads_substream_info = bits.ReadBit();
    if (emdf_info.b_emdf_payloads_substream_info == 1) {
        // emdf_payloads_substream_info ()
        if (bits.ReadBits(2) == 3) {    // substream_index
            AP4_Ac4VariableBits(bits, 2);
        }
    }
    emdf_info.protectionLengthPrimary   = bits.ReadBits(2);
    emdf_info.protectionLengthSecondary = bits.ReadBits(2);
    // protection_bits_primary
    switch (emdf_info.protectionLengthPrimary) { 
        case 1:
            emdf_info.protection_bits_primary[0] = bits.ReadBits(8);
            break;
        case 2:
            for (unsigned idx = 0; idx < 4; idx ++)  { emdf_info.protection_bits_primary[idx] = bits.ReadBits(8); }
            break;
        case 3:
            for (unsigned idx = 0; idx < 16; idx ++) { emdf_info.protection_bits_primary[idx] = bits.ReadBits(8); }
            break;
        default:
            // Error
            break;
    }
    // protection_bits_secondary
    switch (emdf_info.protectionLengthSecondary) { 
        case 0:
            break;
        case 1:
            emdf_info.protection_bits_Secondary[0] = bits.ReadBits(8);
            break;
        case 2:
            for (unsigned idx = 0; idx < 4; idx ++)  { emdf_info.protection_bits_Secondary[idx] = bits.ReadBits(8); }
            break;
        case 3:
            for (unsigned idx = 0; idx < 16; idx ++) { emdf_info.protection_bits_Secondary[idx] = bits.ReadBits(8); }
            break;
        default:
            // TODO: Error
            break;
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParsePresentationSubstreamInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dac4Atom::Ac4Dsi::PresentationV1::ParsePresentationSubstreamInfo(AP4_BitReader &bits)
{
    d.v1.b_alternative = bits.ReadBit();
    /* unsigned int b_pres_ndot = */ bits.ReadBit();          // b_pres_ndot;
    unsigned int substream_index = bits.ReadBits(2);    //substream_index
    if (substream_index == 3){
        AP4_Ac4VariableBits(bits, 2);
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetPresentationChMode
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetPresentationChMode()
{
    int pres_ch_mode = -1;
    char b_obj_or_ajoc = 0;
    // TODO: n_substream_groups
    for (unsigned int sg = 0; sg < d.v1.n_substream_groups; sg++){
        AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 &substream_group = d.v1.substream_groups[sg];
        unsigned int n_substreams = d.v1.substream_groups[sg].d.v1.n_substreams;
        for (unsigned int sus = 0; sus < n_substreams; sus++){
            AP4_Dac4Atom::Ac4Dsi::SubStream &substream = substream_group.d.v1.substreams[sus];
            if (substream_group.d.v1.b_channel_coded){
                pres_ch_mode = AP4_Ac4SuperSet(pres_ch_mode, substream.ch_mode);
            }else {
                b_obj_or_ajoc = 1;
            }
        }
    }
    if (b_obj_or_ajoc == 1) { pres_ch_mode = -1; }
    return pres_ch_mode;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetPresentationChannelMask
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetPresentationChannelMask()
{
    unsigned int channel_mask = 0;
    char b_obj_or_ajoc = 0;
    // TODO: n_substream_groups 
    for (unsigned int sg = 0; sg < d.v1.n_substream_groups; sg ++){
        AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 &substream_group = d.v1.substream_groups[sg];
        unsigned int n_substreams = d.v1.substream_groups[sg].d.v1.n_substreams;
        for (unsigned int sus = 0; sus < n_substreams; sus ++){
            AP4_Dac4Atom::Ac4Dsi::SubStream &substream = substream_group.d.v1.substreams[sus];
            if (substream_group.d.v1.b_channel_coded){
                channel_mask |= substream.dsi_substream_channel_mask;
            }else {
                b_obj_or_ajoc = 1;
            }
        }
    }
    //Modify the mask for headphone presentation with b_pre_virtualized = 1
    /*
    if (presentation_version == 2 || d.v1.b_pre_virtualized == 1){
        if (channel_mask == 0x03) { channel_mask = 0x01;}
    }
    */
    // TODO: temporary solution according to Dolby's internal discussion
    if (channel_mask == 0x03) { channel_mask = 0x01;}

    // If one substream contains Tfl, Tfr, Tbl, Tbr, Tl and Tr shall be removed.
    if ((channel_mask & 0x30) && (channel_mask & 0x80))  { channel_mask &= ~0x80;}

    // objective channel mask
    if (b_obj_or_ajoc == 1) { channel_mask = 0x800000; }
    return channel_mask;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetPresB4BackChannelsPresent
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetPresB4BackChannelsPresent()
{
    for (unsigned int sg = 0; sg < d.v1.n_substream_groups; sg ++){
        AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 &substream_group = d.v1.substream_groups[sg];
        for (unsigned int sus = 0; sus < substream_group.d.v1.n_substreams; sus ++){
            d.v1.pres_b_4_back_channels_present |=  substream_group.d.v1.substreams[sus].b_4_back_channels_present;
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetPresTopChannelPairs
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetPresTopChannelPairs()
{
    unsigned char tmp_pres_top_channel_pairs = 0;
    for (unsigned int sg = 0; sg < d.v1.n_substream_groups; sg ++){
        AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 &substream_group = d.v1.substream_groups[sg];
        for (unsigned int sus = 0; sus < substream_group.d.v1.n_substreams; sus ++){
            if (tmp_pres_top_channel_pairs < substream_group.d.v1.substreams[sus].top_channels_present) {
                tmp_pres_top_channel_pairs = substream_group.d.v1.substreams[sus].top_channels_present;
            }
        }
    }
    switch (tmp_pres_top_channel_pairs){
        case 0: 
            d.v1.pres_top_channel_pairs = 0; break;
        case 1:
        case 2:
            d.v1.pres_top_channel_pairs = 1; break;
        case 3:
            d.v1.pres_top_channel_pairs = 2; break;
        default:
            d.v1.pres_top_channel_pairs = 0; break;
    }
    return AP4_SUCCESS;
}


/*----------------------------------------------------------------------
|   AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetBPresentationCoreDiffers
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Dac4Atom::Ac4Dsi::PresentationV1::GetBPresentationCoreDiffers()
{
    int pres_ch_mode_core = -1;
    char b_obj_or_ajoc_adaptive = 0;
    for (unsigned int sg = 0; sg < d.v1.n_substream_groups; sg ++){
        AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 &substream_group = d.v1.substream_groups[sg];
        unsigned int n_substreams = d.v1.substream_groups[sg].d.v1.n_substreams;
        for (unsigned int sus = 0; sus < n_substreams; sus ++){
            AP4_Dac4Atom::Ac4Dsi::SubStream &substream = substream_group.d.v1.substreams[sus];
            if (substream_group.d.v1.b_channel_coded){
                pres_ch_mode_core = AP4_Ac4SuperSet(pres_ch_mode_core, substream.GetChModeCore(substream_group.d.v1.b_channel_coded));
            }else {
                if (substream.b_ajoc){
                    if (substream.b_static_dmx){
                        pres_ch_mode_core = AP4_Ac4SuperSet(pres_ch_mode_core, substream.GetChModeCore(substream_group.d.v1.b_channel_coded));
                    }else{
                        b_obj_or_ajoc_adaptive = 1;
                    }
                }else{
                    b_obj_or_ajoc_adaptive = 1;
                }
            }
        }
    }
    if (b_obj_or_ajoc_adaptive) {
        pres_ch_mode_core = -1;
    }
    if (pres_ch_mode_core == GetPresentationChMode()) {
        pres_ch_mode_core = -1;
    }
    return pres_ch_mode_core;
}
