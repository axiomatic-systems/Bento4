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

#ifndef _AP4_DEC4_ATOM_H_
#define _AP4_DEC4_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Utils.h"
#include "Ap4Ac4Utils.h"

/*----------------------------------------------------------------------
|   AP4_Dac4Atom
+---------------------------------------------------------------------*/
class AP4_Dac4Atom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_Dac4Atom, AP4_Atom)

    // inner classes
    struct Ac4Dsi {
        struct Ac4BitrateDsi{
            AP4_UI08 bit_rate_mode;
            AP4_UI32 bit_rate;
            AP4_UI32 bit_rate_precision;
        
            // methods
            AP4_Result WriteBitrateDsi(AP4_BitWriter &bits);
        };
        struct Ac4AlternativeInfo{
            AP4_UI16 name_len;
            AP4_UI08 presentation_name[256]; // restrict to 256 char
            AP4_UI08 n_targets;
            AP4_UI08 target_md_compat[32];
            AP4_UI08 target_device_category[32];

            // methods
            AP4_Result WriteAlternativeInfo(AP4_BitWriter &bits);
        };
        struct SubStream{
            AP4_UI08 b_4_back_channels_present;
            AP4_UI08 b_centre_present;
            AP4_UI08 top_channels_present;
            AP4_UI08 b_lfe;
            AP4_UI08 dsi_sf_multiplier;
            AP4_UI08 b_substream_bitrate_indicator;
            AP4_UI08 substream_bitrate_indicator;
            AP4_UI08 ch_mode; // auxiliary information, used to calcuate pres_ch_mode
            AP4_UI32 dsi_substream_channel_mask;
            AP4_UI08 b_ajoc;
            AP4_UI08 b_static_dmx;
            AP4_UI08 n_dmx_objects_minus1;
            AP4_UI08 n_umx_objects_minus1;
            AP4_UI08 b_substream_contains_bed_objects;
            AP4_UI08 b_substream_contains_dynamic_objects;
            AP4_UI08 b_substream_contains_ISF_objects;

            // methods
            AP4_Result ParseSubstreamInfoChan(AP4_BitReader &bits, 
                                              unsigned int  presentation_version, 
                                              unsigned char defalut_presentation_flag,
                                              unsigned int  fs_idx,
                                              unsigned int  &speaker_index_mask,
                                              unsigned int  frame_rate_factor,
                                              unsigned int  b_substreams_present,
                                              unsigned char &dolby_atmos_indicator);

            AP4_Result ParseSubStreamInfoAjoc(AP4_BitReader &bits, 
                                              unsigned int  &channel_count,
                                              unsigned char defalut_presentation_flag,
                                              unsigned int  fs_idx,
                                              unsigned int  frame_rate_factor,
                                              unsigned int  b_substreams_present);

            AP4_Result ParseSubstreamInfoObj(AP4_BitReader &bits,
                                             unsigned int  &channel_count,
                                             unsigned char defalut_presentation_flag,
                                             unsigned int  fs_idx,
                                             unsigned int  frame_rate_factor,
                                             unsigned int  b_substreams_present);
            AP4_Result WriteSubstreamDsi    (AP4_BitWriter &bits, unsigned char b_channel_coded);
            AP4_Result GetChModeCore        (unsigned char b_channel_coded);
        private:
            AP4_Result ParseChMode          (AP4_BitReader &bits, int presentationVersion, unsigned char &dolby_atmos_indicator);
            AP4_Result ParseDsiSfMutiplier  (AP4_BitReader &bits, unsigned int fs_idx  );
            AP4_Result BedDynObjAssignment  (AP4_BitReader &bits, unsigned int nSignals, bool is_upmix);
            AP4_Result ParseSubstreamIdxInfo(AP4_BitReader &bits, unsigned int b_substreams_present);
            
            AP4_Result ParseBitrateIndicator(AP4_BitReader &bits);
            AP4_Result ParseOamdCommonData  (AP4_BitReader &bits);
            AP4_Result Trim                 (AP4_BitReader &bits);
            AP4_Result BedRendeInfo         (AP4_BitReader &bits);
            AP4_UI32   ObjNumFromIsfConfig  (unsigned char isf_config);
            AP4_UI32   BedNumFromAssignCode (unsigned char assign_code);
            AP4_UI32   BedNumFromNonStdMask (unsigned int  non_std_mask);
            AP4_UI32   BedNumFromStdMask    (unsigned int  std_mask);
        };
        struct SubStreamGroupV1 {
            union {
                struct 
                {
                    AP4_UI08 channel_mode;
                    AP4_UI08 dsi_sf_multiplier;
                    AP4_UI08 b_substream_bitrate_indicator;
                    AP4_UI08 substream_bitrate_indicator;
                    AP4_UI08 add_ch_base;
                    AP4_UI08 b_content_type;
                    AP4_UI08 content_classifier;
                    AP4_UI08 b_language_indicator;
                    AP4_UI08 n_language_tag_bytes;
                    AP4_UI08 language_tag_bytes[64];
                }v0;
                struct 
                {
                    AP4_UI08 b_substreams_present;
                    AP4_UI08 b_hsf_ext;
                    AP4_UI08 b_channel_coded;
                    AP4_UI08 n_substreams;
                    SubStream* substreams;
                    AP4_UI08 b_content_type;
                    AP4_UI08 content_classifier;
                    AP4_UI08 b_language_indicator;
                    AP4_UI08 n_language_tag_bytes;
                    AP4_UI08 language_tag_bytes[64]; // n_language_tag_bytes is 6 bits
                    AP4_UI08 dolby_atmos_indicator;
                }v1;
            }d;
            AP4_Result ParseSubstreamGroupInfo(AP4_BitReader &bits, 
                                               unsigned int  bitstream_version,
                                               unsigned int  presentation_version,
                                               unsigned char defalut_presentation_flag,
                                               unsigned int  frame_rate_factor,
                                               unsigned int  fs_idx,
                                               unsigned int  &channel_count,
                                               unsigned int  &speaker_index_mask,
                                               unsigned int  &b_obj_or_Ajoc);
            AP4_Result WriteSubstreamGroupDsi(AP4_BitWriter &bits);
        private:
            AP4_Result ParseOamdSubstreamInfo  (AP4_BitReader &bits);
            AP4_Result ParseHsfExtSubstreamInfo(AP4_BitReader &bits);
            AP4_Result ParseContentType        (AP4_BitReader &bits);
            AP4_Result WriteContentType        (AP4_BitWriter &bits);
        };
        struct PresentationV1 {
            AP4_UI08 presentation_version;
            union {
                struct {
                    AP4_UI08 presentation_config;
                    AP4_UI08 mdcompat;
                    AP4_UI08 presentation_id;
                    AP4_UI08 dsi_frame_rate_multiply_info;
                    AP4_UI08 presentation_emdf_version;
                    AP4_UI16 presentation_key_id;
                    AP4_UI32 presentation_channel_mask;
                    // more fields omitted, v0 is deprecated.
                } v0;
                struct {
                    AP4_UI08 presentation_config_v1;
                    AP4_UI08 mdcompat;
                    AP4_UI08 b_presentation_id;
                    AP4_UI08 presentation_id;
                    AP4_UI08 dsi_frame_rate_multiply_info;
                    AP4_UI08 dsi_frame_rate_fraction_info;
                    AP4_UI08 presentation_emdf_version;
                    AP4_UI16 presentation_key_id;
                    AP4_UI08 b_presentation_channel_coded;
                    AP4_UI08 dsi_presentation_ch_mode;
                    AP4_UI08 pres_b_4_back_channels_present;
                    AP4_UI08 pres_top_channel_pairs;
                    AP4_UI32 presentation_channel_mask_v1;
                    AP4_UI08 b_presentation_core_differs;
                    AP4_UI08 b_presentation_core_channel_coded;
                    AP4_UI08 dsi_presentation_channel_mode_core;
                    AP4_UI08 b_presentation_filter;
                    AP4_UI08 b_enable_presentation;
                    AP4_UI08 n_filter_bytes;

                    AP4_UI08 b_multi_pid;
                    AP4_UI08 n_substream_groups;
                    SubStreamGroupV1* substream_groups;
                    AP4_UI32* substream_group_indexs; // auxiliary information, not exist in DSI
                    AP4_UI08 n_skip_bytes;

                    AP4_UI08 b_pre_virtualized;
                    AP4_UI08 b_add_emdf_substreams;
                    AP4_UI08 n_add_emdf_substreams;
                    AP4_UI08 substream_emdf_version[128]; // n_add_emdf_substreams - 7 bits
                    AP4_UI16 substream_key_id[128];

                    AP4_UI08 b_presentation_bitrate_info;
                    Ac4BitrateDsi ac4_bitrate_dsi;
                    AP4_UI08 b_alternative;
                    Ac4AlternativeInfo alternative_info;
                    AP4_UI08 de_indicator;
                    AP4_UI08 dolby_atmos_indicator;
                    AP4_UI08 b_extended_presentation_id;
                    AP4_UI16 extended_presentation_id;
                } v1;
            } d;
            AP4_Result ParsePresentationV1Info(AP4_BitReader &bits, 
                                               unsigned int  bitstream_version,
                                               unsigned int  frame_rate_idx,
                                               unsigned int  pres_idx,
                                               unsigned int  &max_group_index,
                                               unsigned int  **first_pres_sg_index,
                                               unsigned int  &first_pres_sg_num);
            AP4_Result WritePresentationV1Dsi(AP4_BitWriter &bits);
        private:
            AP4_Result ParsePresentationVersion      (AP4_BitReader &bits, unsigned int bitstream_version);
            AP4_Result ParsePresentationConfigExtInfo(AP4_BitReader &bits, unsigned int bitstream_version);
            AP4_UI32   ParseAc4SgiSpecifier          (AP4_BitReader &bits, unsigned int bitstream_version) ;
            AP4_Result ParseDSIFrameRateMultiplyInfo (AP4_BitReader &bits, unsigned int frame_rate_idx);
            AP4_Result ParseDSIFrameRateFractionsInfo(AP4_BitReader &bits, unsigned int frame_rate_idx);
            AP4_Result ParseEmdInfo                  (AP4_BitReader &bits, AP4_Ac4EmdfInfo &emdf_info);
            AP4_Result ParsePresentationSubstreamInfo(AP4_BitReader &bits);
            AP4_Result GetPresentationChMode();
            AP4_Result GetPresentationChannelMask();
            AP4_Result GetPresB4BackChannelsPresent();
            AP4_Result GetPresTopChannelPairs();
            AP4_Result GetBPresentationCoreDiffers();
        };
        
        AP4_UI08 ac4_dsi_version;
        union {
            struct {
                AP4_UI08 bitstream_version;
                AP4_UI08 fs_index;
                AP4_UI32 fs;
                AP4_UI08 frame_rate_index;
                AP4_UI16 n_presentations;
                // more fields from `ac4_dsi` are not included here, v0 is deprecated.
            } v0;
            struct {
                AP4_UI08 bitstream_version;
                AP4_UI08 fs_index;
                AP4_UI32 fs;
                AP4_UI08 frame_rate_index;
                AP4_UI08 b_program_id;
                AP4_UI16 short_program_id;
                AP4_UI08 b_uuid;
                AP4_UI08 program_uuid[16];
                Ac4BitrateDsi ac4_bitrate_dsi;
                AP4_UI16        n_presentations;
                PresentationV1* presentations;
            } v1;
        } d;
    };
    
    // class methods
    static AP4_Dac4Atom* Create(AP4_Size size, AP4_ByteStream& stream);

    // constructors
    AP4_Dac4Atom(AP4_UI32 size, const Ac4Dsi* ac4Dsi);  // DSI vaiable initialize m_RawBytes (m_Dsi -> m_RawBytes)

    // destructor
    ~AP4_Dac4Atom();
    
    // methods
    virtual AP4_Result    InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result    WriteFields(AP4_ByteStream& stream);
    virtual AP4_Atom*     Clone() { return new AP4_Dac4Atom(m_Size32, m_RawBytes.GetData()); }
    virtual AP4_Dac4Atom* CloneConst() const { return new AP4_Dac4Atom(m_Size32, m_RawBytes.GetData()); }

    // accessors
    const AP4_DataBuffer& GetRawBytes() const { return m_RawBytes;   }
    const Ac4Dsi&         GetDsi()      const { return m_Dsi;        }

    // helpers
    void GetCodecString(AP4_String& codec);
    
private:
    // methods
    AP4_Dac4Atom(const AP4_Dac4Atom& other);
    AP4_Dac4Atom(AP4_UI32 size, const AP4_UI08* payload); // box data initialize m_Dsi (m_RawBytes -> m_Dsi)
    
    // members
    AP4_DataBuffer m_RawBytes;
    Ac4Dsi         m_Dsi;
};

#endif // _AP4_DAC4_ATOM_H_
