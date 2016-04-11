/*****************************************************************
|
|    AP4 - HEVC Parser
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
#include "Ap4HevcParser.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   debugging
+---------------------------------------------------------------------*/
#define AP4_HEVC_PARSER_ENABLE_DEBUG 1

#if defined(AP4_HEVC_PARSER_ENABLE_DEBUG)
#define DBG_PRINTF_0(_x0) printf(_x0)
#define DBG_PRINTF_1(_x0, _x1) printf(_x0, _x1)
#define DBG_PRINTF_2(_x0, _x1, _x2) printf(_x0, _x1, _x2)
#define DBG_PRINTF_3(_x0, _x1, _x2, _x3) printf(_x0, _x1, _x2, _x3)
#define DBG_PRINTF_4(_x0, _x1, _x2, _x3, _x4) printf(_x0, _x1, _x2, _x3, _x4)
#define DBG_PRINTF_5(_x0, _x1, _x2, _x3, _x4, _x5) printf(_x0, _x1, _x2, _x3, _x4, _x5)
#define DBG_PRINTF_6(_x0, _x1, _x2, _x3, _x4, _x5, _x6) printf(_x0, _x1, _x2, _x3, _x4, _x5, _x6)
#define DBG_PRINTF_7(_x0, _x1, _x2, _x3, _x4, _x5, _x6, _x7) printf(_x0, _x1, _x2, _x3, _x4, _x5, _x6, _x7)
#else
#define DBG_PRINTF_0(_x0)
#define DBG_PRINTF_1(_x0, _x1)
#define DBG_PRINTF_2(_x0, _x1, _x2)
#define DBG_PRINTF_3(_x0, _x1, _x2, _x3)
#define DBG_PRINTF_4(_x0, _x1, _x2, _x3, _x4)
#define DBG_PRINTF_5(_x0, _x1, _x2, _x3, _x4, _x5)
#define DBG_PRINTF_6(_x0, _x1, _x2, _x3, _x4, _x5, _x6)
#define DBG_PRINTF_7(_x0, _x1, _x2, _x3, _x4, _x5, _x6, _x7)
#endif

/*----------------------------------------------------------------------
|   AP4_HevcNalParser::NaluTypeName
+---------------------------------------------------------------------*/
const char*
AP4_HevcNalParser::NaluTypeName(unsigned int nalu_type)
{
    switch (nalu_type) {
        case  0: return "TRAIL_N - Coded slice segment of a non-TSA, non-STSA trailing picture";
        case  1: return "TRAIL_R - Coded slice segment of a non-TSA, non-STSA trailing picture";
        case  2: return "TSA_N - Coded slice segment of a TSA picture";
        case  3: return "TSA_R - Coded slice segment of a TSA picture";
        case  4: return "STSA_N - Coded slice segment of an STSA picture";
        case  5: return "STSA_R - Coded slice segment of an STSA picture";
        case  6: return "RADL_N - Coded slice segment of a RADL picture";
        case  7: return "RADL_R - Coded slice segment of a RADL picture";
        case  8: return "RASL_N - Coded slice segment of a RASL picture";
        case  9: return "RASL_R - Coded slice segment of a RASL picture";
        case 10: return "RSV_VCL_N10 - Reserved non-IRAP sub-layer non-reference";
        case 12: return "RSV_VCL_N12 - Reserved non-IRAP sub-layer non-reference";
        case 14: return "RSV_VCL_N14 - Reserved non-IRAP sub-layer non-reference";
        case 11: return "RSV_VCL_R11 - Reserved non-IRAP sub-layer reference";
        case 13: return "RSV_VCL_R13 - Reserved non-IRAP sub-layer reference";
        case 15: return "RSV_VCL_R15 - Reserved non-IRAP sub-layer reference";
        case 16: return "BLA_W_LP - Coded slice segment of a BLA picture";
        case 17: return "BLA_W_RADL - Coded slice segment of a BLA picture";
        case 18: return "BLA_N_LP - Coded slice segment of a BLA picture";
        case 19: return "IDR_W_RADL - Coded slice segment of an IDR picture";
        case 20: return "IDR_N_LP - Coded slice segment of an IDR picture";
        case 21: return "CRA_NUT - Coded slice segment of a CRA picture";
        case 22: return "RSV_IRAP_VCL22 - Reserved IRAP";
        case 23: return "RSV_IRAP_VCL23 - Reserved IRAP";
        case 24: return "RSV_VCL24 - Reserved non-IRAP";
        case 25: return "RSV_VCL25 - Reserved non-IRAP";
        case 26: return "RSV_VCL26 - Reserved non-IRAP";
        case 27: return "RSV_VCL27 - Reserved non-IRAP";
        case 28: return "RSV_VCL28 - Reserved non-IRAP";
        case 29: return "RSV_VCL29 - Reserved non-IRAP";
        case 30: return "RSV_VCL30 - Reserved non-IRAP";
        case 31: return "RSV_VCL31 - Reserved non-IRAP";
        case 32: return "VPS_NUT - Video parameter set";
        case 33: return "SPS_NUT - Sequence parameter set";
        case 34: return "PPS_NUT - Picture parameter set";
        case 35: return "AUD_NUT - Access unit delimiter";
        case 36: return "EOS_NUT - End of sequence";
        case 37: return "EOB_NUT - End of bitstream";
        case 38: return "FD_NUT - Filler data";
        case 39: return "PREFIX_SEI_NUT - Supplemental enhancement information";
        case 40: return "SUFFIX_SEI_NUT - Supplemental enhancement information";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcNalParser::PicTypeName
+---------------------------------------------------------------------*/
const char*
AP4_HevcNalParser::PicTypeName(unsigned int primary_pic_type)
{
	switch (primary_pic_type) {
        case 0: return "I";
        case 1: return "I, P";
        case 2: return "I, P, B";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcNalParser::SliceTypeName
+---------------------------------------------------------------------*/
const char*
AP4_HevcNalParser::SliceTypeName(unsigned int slice_type)
{
	switch (slice_type) {
        case 0: return "B";
        case 1: return "P";
        case 2: return "I";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   ReadGolomb
+---------------------------------------------------------------------*/
static unsigned int
ReadGolomb(AP4_BitReader& bits)
{
    unsigned int leading_zeros = 0;
    while (bits.ReadBit() == 0) {
        leading_zeros++;
        if (leading_zeros > 32) return 0; // safeguard
    }
    if (leading_zeros) {
        return (1<<leading_zeros)-1+bits.ReadBits(leading_zeros);
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcNalParser::AP4_HevcNalParser
+---------------------------------------------------------------------*/
AP4_HevcNalParser::AP4_HevcNalParser() :
    AP4_NalParser()
{
}

/*----------------------------------------------------------------------
|   AP4_HevcSliceSegmentHeader
+---------------------------------------------------------------------*/
struct AP4_HevcSliceSegmentHeader {
    AP4_HevcSliceSegmentHeader() {} // leave members uninitialized on purpose
    
    AP4_Result Parse(const AP4_UI08*                data,
                     unsigned int                   data_size,
                     unsigned int                   nal_unit_type,
                     AP4_HevcPictureParameterSet**  picture_parameter_sets,
                     AP4_HevcSequenceParameterSet** sequence_parameter_sets);

    unsigned int first_slice_segment_in_pic_flag;
    unsigned int no_output_of_prior_pics_flag;
    unsigned int slice_pic_parameter_set_id;
    unsigned int dependent_slice_segment_flag;
    unsigned int slice_segment_address;
    unsigned int slice_type;
    unsigned int pic_output_flag;
    unsigned int colour_plane_id;
    unsigned int slice_pic_order_cnt_lsb;
    unsigned int short_term_ref_pic_set_sps_flag;
};

/*----------------------------------------------------------------------
|   AP4_HevcSliceSegmentHeader::Parse
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcSliceSegmentHeader::Parse(const AP4_UI08*                data,
                                  unsigned int                   data_size,
                                  unsigned int                   nal_unit_type,
                                  AP4_HevcPictureParameterSet**  picture_parameter_sets,
                                  AP4_HevcSequenceParameterSet** sequence_parameter_sets) {
    // initialize all members to 0
    AP4_SetMemory(this, 0, sizeof(*this));
    
    AP4_DataBuffer unescaped(data, data_size);
    AP4_NalParser::Unescape(unescaped);
    AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());

    bits.SkipBits(16); // NAL Unit Header
    
    first_slice_segment_in_pic_flag = bits.ReadBit();
    if (nal_unit_type >= AP4_HEVC_NALU_TYPE_BLA_W_LP && nal_unit_type <= AP4_HEVC_NALU_TYPE_RSV_IRAP_VCL23) {
        no_output_of_prior_pics_flag = bits.ReadBit();
    }
    slice_pic_parameter_set_id = ReadGolomb(bits);
    if (slice_pic_parameter_set_id > AP4_HEVC_PPS_MAX_ID) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    const AP4_HevcPictureParameterSet* pps = picture_parameter_sets[slice_pic_parameter_set_id];
    if (pps == NULL) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    const AP4_HevcSequenceParameterSet* sps = sequence_parameter_sets[pps->pps_seq_parameter_set_id];
    if (sps == NULL) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    
    if (!first_slice_segment_in_pic_flag) {
        if (pps->dependent_slice_segments_enabled_flag) {
            dependent_slice_segment_flag = bits.ReadBit();
        }
        
        // compute how many bits the next field:
        // n_bits = Ceil( Log2( PicSizeInCtbsY ) )
        // PicSizeInCtbsY = PicWidthInCtbsY * PicHeightInCtbsY
        // PicWidthInCtbsY = Ceil( pic_width_in_luma_samples / CtbSizeY )
        // PicHeightInCtbsY = Ceil( pic_height_in_luma_samples / CtbSizeY )
        // CtbSizeY = 1 << CtbLog2SizeY
        // CtbLog2SizeY = MinCbLog2SizeY + log2_diff_max_min_luma_coding_block_size
        // MinCbLog2SizeY = log2_min_luma_coding_block_size_minus3 + 3
        unsigned int MinCbLog2SizeY   = sps->log2_min_luma_coding_block_size_minus3 + 3;
        unsigned int CtbLog2SizeY     = MinCbLog2SizeY + sps->log2_diff_max_min_luma_coding_block_size;
        unsigned int CtbSizeY         = 1 << CtbLog2SizeY;
        unsigned int PicWidthInCtbsY  = (sps->pic_width_in_luma_samples + CtbSizeY - 1) / CtbSizeY;
        unsigned int PicHeightInCtbsY = (sps->pic_height_in_luma_samples + CtbSizeY - 1) / CtbSizeY;
        unsigned int PicSizeInCtbsY   = PicWidthInCtbsY * PicHeightInCtbsY;
        unsigned int bits_needed = 1;
        while (PicSizeInCtbsY > (unsigned int)(1 << bits_needed)) {
            ++bits_needed;
        }
        slice_segment_address = bits.ReadBits(bits_needed);
    }
    
    if (!dependent_slice_segment_flag) {
        bits.ReadBits(pps->num_extra_slice_header_bits);
    }
    
    slice_type = ReadGolomb(bits);
    if (pps->output_flag_present_flag) {
        pic_output_flag = bits.ReadBit();
    }
    if (sps->separate_colour_plane_flag) {
        colour_plane_id = bits.ReadBits(2);
    }
    if (nal_unit_type != AP4_HEVC_NALU_TYPE_IDR_W_RADL && nal_unit_type != AP4_HEVC_NALU_TYPE_IDR_N_LP) {
        slice_pic_order_cnt_lsb = bits.ReadBits(sps->log2_max_pic_order_cnt_lsb_minus4+4);
        short_term_ref_pic_set_sps_flag = bits.ReadBit();
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_HevcProfileTierLevel::AP4_HevcProfileTierLevel
+---------------------------------------------------------------------*/
AP4_HevcProfileTierLevel::AP4_HevcProfileTierLevel() :
    general_profile_space(0),
    general_tier_flag(0),
    general_profile_idc(0),
    general_profile_compatibility_flags(0),
    general_constraint_indicator_flags(0),
    general_level_idc(0)
{
    AP4_SetMemory(&sub_layer_info[0], 0, sizeof(sub_layer_info));
}

/*----------------------------------------------------------------------
|   AP4_HevcProfileTierLevel::Parse
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcProfileTierLevel::Parse(AP4_BitReader& bits, unsigned int max_num_sub_layers_minus_1)
{
    // profile_tier_level
    general_profile_space               = bits.ReadBits(2);
    general_tier_flag                   = bits.ReadBit();
    general_profile_idc                 = bits.ReadBits(5);
    general_profile_compatibility_flags = bits.ReadBits(32);
    
    general_constraint_indicator_flags  = ((AP4_UI64)bits.ReadBits(16)) << 32;
    general_constraint_indicator_flags |= bits.ReadBits(32);
    
    general_level_idc                   = bits.ReadBits(8);
    for (unsigned int i = 0; i < max_num_sub_layers_minus_1; i++) {
        sub_layer_info[i].sub_layer_profile_present_flag = bits.ReadBit();
        sub_layer_info[i].sub_layer_level_present_flag   = bits.ReadBit();
    }
    if (max_num_sub_layers_minus_1) {
        for (unsigned int i = max_num_sub_layers_minus_1; i < 8; i++) {
            bits.ReadBits(2); // reserved_zero_2bits[i]
        }
    }
    for (unsigned int i = 0; i < max_num_sub_layers_minus_1; i++) {
        if (sub_layer_info[i].sub_layer_profile_present_flag) {
            sub_layer_info[i].sub_layer_profile_space               = bits.ReadBits(2);
            sub_layer_info[i].sub_layer_tier_flag                   = bits.ReadBit();
            sub_layer_info[i].sub_layer_profile_idc                 = bits.ReadBits(5);
            sub_layer_info[i].sub_layer_profile_compatibility_flags = bits.ReadBits(32);
            sub_layer_info[i].sub_layer_progressive_source_flag     = bits.ReadBit();
            sub_layer_info[i].sub_layer_interlaced_source_flag      = bits.ReadBit();
            sub_layer_info[i].sub_layer_non_packed_constraint_flag  = bits.ReadBit();
            sub_layer_info[i].sub_layer_frame_only_constraint_flag  = bits.ReadBit();
            bits.ReadBits(32); bits.ReadBits(12); // sub_layer_reserved_zero_44bits
        }
        if (sub_layer_info[i].sub_layer_level_present_flag) {
            sub_layer_info[i].sub_layer_level_idc = bits.ReadBits(8);
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_HevcPictureParameterSet::AP4_HevcPictureParameterSet
+---------------------------------------------------------------------*/
AP4_HevcPictureParameterSet::AP4_HevcPictureParameterSet() :
    pps_pic_parameter_set_id(0),
    pps_seq_parameter_set_id(0),
    dependent_slice_segments_enabled_flag(0),
    output_flag_present_flag(0),
    num_extra_slice_header_bits(0)
{
}

/*----------------------------------------------------------------------
|   AP4_HevcPictureParameterSet::Parse
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcPictureParameterSet::Parse(const unsigned char* data, unsigned int data_size)
{
    raw_bytes.SetData(data, data_size);

    AP4_DataBuffer unescaped(data, data_size);
    AP4_NalParser::Unescape(unescaped);
    AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());

    bits.SkipBits(16); // NAL Unit Header

    pps_pic_parameter_set_id = ReadGolomb(bits);
    if (pps_pic_parameter_set_id > AP4_HEVC_PPS_MAX_ID) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    pps_seq_parameter_set_id = ReadGolomb(bits);
    if (pps_seq_parameter_set_id > AP4_HEVC_SPS_MAX_ID) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    dependent_slice_segments_enabled_flag = bits.ReadBit();
    output_flag_present_flag              = bits.ReadBit();
    num_extra_slice_header_bits           = bits.ReadBits(3);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_HevcSequenceParameterSet::AP4_HevcSequenceParameterSet
+---------------------------------------------------------------------*/
AP4_HevcSequenceParameterSet::AP4_HevcSequenceParameterSet() :
    sps_video_parameter_set_id(0),
    sps_max_sub_layers_minus1(0),
    sps_temporal_id_nesting_flag(0),
    sps_seq_parameter_set_id(0),
    chroma_format_idc(0),
    separate_colour_plane_flag(0),
    pic_width_in_luma_samples(0),
    pic_height_in_luma_samples(0),
    conformance_window_flag(0),
    conf_win_left_offset(0),
    conf_win_right_offset(0),
    conf_win_top_offset(0),
    conf_win_bottom_offset(0),
    bit_depth_luma_minus8(0),
    bit_depth_chroma_minus8(0),
    log2_max_pic_order_cnt_lsb_minus4(0),
    sps_sub_layer_ordering_info_present_flag(0),
    log2_min_luma_coding_block_size_minus3(0),
    log2_diff_max_min_luma_coding_block_size(0),
    log2_min_transform_block_size_minus2(0),
    log2_diff_max_min_transform_block_size(0),
    max_transform_hierarchy_depth_inter(0),
    max_transform_hierarchy_depth_intra(0),
    scaling_list_enabled_flag(0),
    sps_scaling_list_data_present_flag(0),
    amp_enabled_flag(0),
    sample_adaptive_offset_enabled_flag(0),
    pcm_enabled_flag(0),
    pcm_sample_bit_depth_luma_minus1(0),
    pcm_sample_bit_depth_chroma_minus1(0),
    log2_min_pcm_luma_coding_block_size_minus3(0),
    log2_diff_max_min_pcm_luma_coding_block_size(0),
    pcm_loop_filter_disabled_flag(0),
    num_short_term_ref_pic_sets(0)
{
    AP4_SetMemory(&profile_tier_level, 0, sizeof(profile_tier_level));
    for (unsigned int i=0; i<8; i++) {
        sps_max_dec_pic_buffering_minus1[i] = 0;
        sps_max_num_reorder_pics[i]         = 0;
        sps_max_latency_increase_plus1[i]   = 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcSequenceParameterSet::Parse
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcSequenceParameterSet::Parse(const unsigned char* data, unsigned int data_size)
{
    raw_bytes.SetData(data, data_size);
    
    AP4_DataBuffer unescaped(data, data_size);
    AP4_NalParser::Unescape(unescaped);
    AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());

    bits.SkipBits(16); // NAL Unit Header

    sps_video_parameter_set_id   = bits.ReadBits(4);
    sps_max_sub_layers_minus1    = bits.ReadBits(3);
    sps_temporal_id_nesting_flag = bits.ReadBit();
    
    AP4_Result result = profile_tier_level.Parse(bits, sps_max_sub_layers_minus1);
    if (AP4_FAILED(result)) {
        return result;
    }
    
    sps_seq_parameter_set_id = ReadGolomb(bits);
    if (sps_seq_parameter_set_id > AP4_HEVC_SPS_MAX_ID) {
        return AP4_ERROR_INVALID_FORMAT;
    }

    chroma_format_idc = ReadGolomb(bits);
    if (chroma_format_idc == 3) {
        separate_colour_plane_flag = bits.ReadBit();
    }
    pic_width_in_luma_samples  = ReadGolomb(bits);
    pic_height_in_luma_samples = ReadGolomb(bits);
    conformance_window_flag    = bits.ReadBit();
    
    if (conformance_window_flag) {
        conf_win_left_offset    = ReadGolomb(bits);
        conf_win_right_offset   = ReadGolomb(bits);
        conf_win_top_offset     = ReadGolomb(bits);
        conf_win_bottom_offset  = ReadGolomb(bits);
    }
    bit_depth_luma_minus8                    = ReadGolomb(bits);
    bit_depth_chroma_minus8                  = ReadGolomb(bits);
    log2_max_pic_order_cnt_lsb_minus4        = ReadGolomb(bits);
    if (log2_max_pic_order_cnt_lsb_minus4 > 16) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    sps_sub_layer_ordering_info_present_flag = bits.ReadBit();
    for (unsigned int i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1);
                      i <= sps_max_sub_layers_minus1;
                      i++) {
        sps_max_dec_pic_buffering_minus1[i] = ReadGolomb(bits);
        sps_max_num_reorder_pics[i]         = ReadGolomb(bits);
        sps_max_latency_increase_plus1[i]   = ReadGolomb(bits);
    }
    log2_min_luma_coding_block_size_minus3   = ReadGolomb(bits);
    log2_diff_max_min_luma_coding_block_size = ReadGolomb(bits);
    log2_min_transform_block_size_minus2     = ReadGolomb(bits);
    log2_diff_max_min_transform_block_size   = ReadGolomb(bits);
    max_transform_hierarchy_depth_inter      = ReadGolomb(bits);
    max_transform_hierarchy_depth_intra      = ReadGolomb(bits);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_HevcSequenceParameterSet::GetInfo
+---------------------------------------------------------------------*/
void
AP4_HevcSequenceParameterSet::GetInfo(unsigned int& width, unsigned int& height)
{
    width  = pic_width_in_luma_samples;
    height = pic_height_in_luma_samples;
}

/*----------------------------------------------------------------------
|   AP4_HevcVideoParameterSet::AP4_HevcVideoParameterSet
+---------------------------------------------------------------------*/
AP4_HevcVideoParameterSet::AP4_HevcVideoParameterSet() :
    vps_video_parameter_set_id(0),
    vps_max_layers_minus1(0),
    vps_max_sub_layers_minus1(0),
    vps_temporal_id_nesting_flag(0),
    vps_sub_layer_ordering_info_present_flag(0),
    vps_max_layer_id(0),
    vps_num_layer_sets_minus1(0),
    vps_timing_info_present_flag(0),
    vps_num_units_in_tick(0),
    vps_time_scale(0),
    vps_poc_proportional_to_timing_flag(0),
    vps_num_ticks_poc_diff_one_minus1(0)
{
    AP4_SetMemory(&profile_tier_level, 0, sizeof(profile_tier_level));
    for (unsigned int i=0; i<8; i++) {
        vps_max_dec_pic_buffering_minus1[i] = 0;
        vps_max_num_reorder_pics[i]         = 0;
        vps_max_latency_increase_plus1[i]   = 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcVideoParameterSet::Parse
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcVideoParameterSet::Parse(const unsigned char* data, unsigned int data_size)
{
    raw_bytes.SetData(data, data_size);

    AP4_DataBuffer unescaped(data, data_size);
    AP4_NalParser::Unescape(unescaped);
    AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());

    bits.SkipBits(16); // NAL Unit Header

    vps_video_parameter_set_id     = bits.ReadBits(4);
    /* vps_reserved_three_2bits */   bits.ReadBits(2);
    vps_max_layers_minus1          = bits.ReadBits(6);
    vps_max_sub_layers_minus1      = bits.ReadBits(3);
    vps_temporal_id_nesting_flag   = bits.ReadBit();
    /* vps_reserved_0xffff_16bits */ bits.ReadBits(16);
    profile_tier_level.Parse(bits, vps_max_sub_layers_minus1);
    vps_sub_layer_ordering_info_present_flag = bits.ReadBit();
    for (unsigned int i = (vps_sub_layer_ordering_info_present_flag ? 0 : vps_max_sub_layers_minus1);
                      i <= vps_max_sub_layers_minus1;
                      i++) {
        vps_max_dec_pic_buffering_minus1[i] = ReadGolomb(bits);
        vps_max_num_reorder_pics[i]         = ReadGolomb(bits);
        vps_max_latency_increase_plus1[i]   = ReadGolomb(bits);
    }
    vps_max_layer_id          = bits.ReadBits(6);
    vps_num_layer_sets_minus1 = ReadGolomb(bits);
    for (unsigned int i = 1; i <= vps_num_layer_sets_minus1; i++) {
        for (unsigned int j = 0; j <= vps_max_layer_id; j++) {
            bits.ReadBit();
        }
    }
    vps_timing_info_present_flag = bits.ReadBit();
    if (vps_timing_info_present_flag) {
        vps_num_units_in_tick               = bits.ReadBits(32);
        vps_time_scale                      = bits.ReadBits(32);
        vps_poc_proportional_to_timing_flag = bits.ReadBit();
        if (vps_poc_proportional_to_timing_flag) {
            vps_num_ticks_poc_diff_one_minus1 = ReadGolomb(bits);
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_HevcFrameParser::AP4_HevcFrameParser
+---------------------------------------------------------------------*/
AP4_HevcFrameParser::AP4_HevcFrameParser() :
    m_CurrentSlice(NULL),
    m_CurrentNalUnitType(0),
    m_CurrentTemporalId(0),
    m_TotalNalUnitCount(0),
    m_TotalAccessUnitCount(0),
    m_AccessUnitFlags(0),
    m_VclNalUnitsInAccessUnit(0),
    m_PrevTid0Pic_PicOrderCntMsb(0),
    m_PrevTid0Pic_PicOrderCntLsb(0)
{
    for (unsigned int i=0; i<=AP4_HEVC_PPS_MAX_ID; i++) {
        m_PPS[i] = NULL;
    }
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        m_SPS[i] = NULL;
    }
    for (unsigned int i=0; i<=AP4_HEVC_VPS_MAX_ID; i++) {
        m_VPS[i] = NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcFrameParser::~AP4_HevcFrameParser
+---------------------------------------------------------------------*/
AP4_HevcFrameParser::~AP4_HevcFrameParser()
{
    delete m_CurrentSlice;
    
    for (unsigned int i=0; i<=AP4_HEVC_PPS_MAX_ID; i++) {
        delete m_PPS[i];
    }
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        delete m_SPS[i];
    }
    for (unsigned int i=0; i<=AP4_HEVC_VPS_MAX_ID; i++) {
        delete m_VPS[i];
    }
    
    // cleanup any un-transfered buffers
    for (unsigned int i=0; i<m_AccessUnitData.ItemCount(); i++) {
        delete m_AccessUnitData[i];
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcFrameParser::AppendNalUnitData
+---------------------------------------------------------------------*/
void
AP4_HevcFrameParser::AppendNalUnitData(const unsigned char* data, unsigned int data_size)
{
    m_AccessUnitData.Append(new AP4_DataBuffer(data, data_size));
}

/*----------------------------------------------------------------------
|   AP4_HevcFrameParser::CheckIfAccessUnitIsCompleted
+---------------------------------------------------------------------*/
void
AP4_HevcFrameParser::CheckIfAccessUnitIsCompleted(AccessUnitInfo& access_unit_info)
{
    if (!m_VclNalUnitsInAccessUnit || !m_CurrentSlice) {
        return;
    }
    DBG_PRINTF_0("\n>>>>>>> New Access Unit\n");
    
    AP4_HevcSequenceParameterSet* sps = m_SPS[m_CurrentSlice->slice_pic_parameter_set_id];
    if (sps == NULL) return;
    
    unsigned int MaxPicOrderCntLsb = (1 << (sps->log2_max_pic_order_cnt_lsb_minus4+4));
    bool NoRaslOutputFlag = false;
    if (m_AccessUnitFlags & AP4_HEVC_ACCESS_UNIT_FLAG_IS_IRAP) {
        if ((m_AccessUnitFlags & AP4_HEVC_ACCESS_UNIT_FLAG_IS_IDR) ||
            (m_AccessUnitFlags & AP4_HEVC_ACCESS_UNIT_FLAG_IS_BLA)
            /* TODO: check for end-of-sequence */) {
            NoRaslOutputFlag = true;
        }
    }
    unsigned int PrevPicOrderCntLsb = 0;
    unsigned int PrevPicOrderCntMsb = 0;
    if (!((m_AccessUnitFlags & AP4_HEVC_ACCESS_UNIT_FLAG_IS_IRAP) && NoRaslOutputFlag)) {
        PrevPicOrderCntLsb = m_PrevTid0Pic_PicOrderCntLsb;
        PrevPicOrderCntMsb = m_PrevTid0Pic_PicOrderCntMsb;
    }
    unsigned int PicOrderCntMsb = 0;
    if (m_CurrentSlice->slice_pic_order_cnt_lsb < PrevPicOrderCntLsb &&
        (PrevPicOrderCntLsb - m_CurrentSlice->slice_pic_order_cnt_lsb) >= (MaxPicOrderCntLsb / 2)) {
        PicOrderCntMsb = PrevPicOrderCntMsb + MaxPicOrderCntLsb;
    } else if (m_CurrentSlice->slice_pic_order_cnt_lsb > PrevPicOrderCntLsb &&
               (m_CurrentSlice->slice_pic_order_cnt_lsb - PrevPicOrderCntLsb) > (MaxPicOrderCntLsb / 2)) {
        PicOrderCntMsb = PrevPicOrderCntMsb - MaxPicOrderCntLsb;
    } else {
        PicOrderCntMsb = PrevPicOrderCntMsb;
    }
    
    if (m_CurrentNalUnitType == AP4_HEVC_NALU_TYPE_BLA_N_LP ||
        m_CurrentNalUnitType == AP4_HEVC_NALU_TYPE_BLA_W_LP ||
        m_CurrentNalUnitType == AP4_HEVC_NALU_TYPE_BLA_W_RADL) {
        PicOrderCntMsb = 0;
    }
    unsigned int PicOrderCntVal = PicOrderCntMsb + m_CurrentSlice->slice_pic_order_cnt_lsb;

    if (m_CurrentTemporalId == 0 && (
        !(m_AccessUnitFlags & AP4_HEVC_ACCESS_UNIT_FLAG_IS_RADL) ||
        !(m_AccessUnitFlags & AP4_HEVC_ACCESS_UNIT_FLAG_IS_RASL) ||
        !(m_AccessUnitFlags & AP4_HEVC_ACCESS_UNIT_FLAG_IS_SUBLAYER_NON_REF))) {
        m_PrevTid0Pic_PicOrderCntLsb = m_CurrentSlice->slice_pic_order_cnt_lsb;
        m_PrevTid0Pic_PicOrderCntMsb = PicOrderCntMsb;
    }
    
    // emit the access unit (transfer ownership)
    access_unit_info.nal_units        = m_AccessUnitData;
    access_unit_info.decode_order     = m_TotalAccessUnitCount;
    access_unit_info.is_random_access = (m_AccessUnitFlags & AP4_HEVC_ACCESS_UNIT_FLAG_IS_IRAP) ? true : false;
    access_unit_info.display_order    = PicOrderCntVal;
    m_AccessUnitData.Clear();
    m_VclNalUnitsInAccessUnit  = 0;
    m_AccessUnitFlags          = 0;
    delete m_CurrentSlice;
    m_CurrentSlice = NULL;
    ++m_TotalAccessUnitCount;
}

/*----------------------------------------------------------------------
|   AP4_HevcFrameParser::Feed
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcFrameParser::Feed(const void*     data,
                          AP4_Size        data_size,
                          AP4_Size&       bytes_consumed,
                          AccessUnitInfo& access_unit_info,
                          bool            eos)
{
    const AP4_DataBuffer* nal_unit = NULL;

    // default return values
    access_unit_info.Reset();
    
    // feed the NAL unit parser
    AP4_Result result = m_NalParser.Feed(data, data_size, bytes_consumed, nal_unit, eos);
    if (AP4_FAILED(result)) {
        return result;
    }
    if (nal_unit && nal_unit->GetDataSize()) {
        const unsigned char* nal_unit_payload = (const unsigned char*)nal_unit->GetData();
        unsigned int         nal_unit_size    = nal_unit->GetDataSize();
        unsigned int         nal_unit_type    = (nal_unit_payload[0] >> 1) & 0x3F;
        unsigned int         nuh_layer_id     = (((nal_unit_payload[0] & 1) << 5) | (nal_unit_payload[1] >> 3));
        unsigned int         nuh_temporal_id  = nal_unit_payload[1] & 0x7;
        
        if (nuh_temporal_id-- == 0) {
            // illegal value, ignore this NAL unit
            return AP4_SUCCESS;
        }
        
        m_CurrentNalUnitType = nal_unit_type;
        m_CurrentTemporalId  = nuh_temporal_id;
        const char* nal_unit_type_name = AP4_HevcNalParser::NaluTypeName(nal_unit_type);
        if (nal_unit_type_name == NULL) nal_unit_type_name = "UNKNOWN";
        DBG_PRINTF_6("NALU %5d: layer_id=%d, temporal_id=%d, size=%5d, type=%02d (%s) ",
               m_TotalNalUnitCount,
               nuh_layer_id,
               nuh_temporal_id,
               nal_unit_size,
               nal_unit_type,
               nal_unit_type_name);
        
        // parse the NAL unit details and react accordingly
        if (nal_unit_type < AP4_HEVC_NALU_TYPE_VPS_NUT) {
            // this is a VCL NAL Unit
            AP4_HevcSliceSegmentHeader* slice_header = new AP4_HevcSliceSegmentHeader;
            result = slice_header->Parse(nal_unit_payload, nal_unit_size, nal_unit_type, &m_PPS[0], &m_SPS[0]);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_1("VLC parsing failed (%d)", result);
                return AP4_ERROR_INVALID_FORMAT;
            }
            
#if defined(AP4_HEVC_PARSER_ENABLE_DEBUG)
            const char* slice_type_name = AP4_HevcNalParser::SliceTypeName(slice_header->slice_type);
            if (slice_type_name == NULL) slice_type_name = "?";
            DBG_PRINTF_4(" pps_id=%d, first=%s, slice_type=%d (%s), ",
                slice_header->slice_pic_parameter_set_id,
                slice_header->first_slice_segment_in_pic_flag?"YES":"NO",
                slice_header->slice_type,
                slice_type_name);
#endif
            if (slice_header->first_slice_segment_in_pic_flag) {
                CheckIfAccessUnitIsCompleted(access_unit_info);
            }
            
            // compute access unit flags
            m_AccessUnitFlags = 0;
            if (nal_unit_type >= AP4_HEVC_NALU_TYPE_BLA_W_LP && nal_unit_type <= AP4_HEVC_NALU_TYPE_RSV_IRAP_VCL23) {
                m_AccessUnitFlags |= AP4_HEVC_ACCESS_UNIT_FLAG_IS_IRAP;
            }
            if (nal_unit_type == AP4_HEVC_NALU_TYPE_IDR_W_RADL || nal_unit_type == AP4_HEVC_NALU_TYPE_IDR_N_LP) {
                m_AccessUnitFlags |= AP4_HEVC_ACCESS_UNIT_FLAG_IS_IDR;
            }
            if (nal_unit_type >= AP4_HEVC_NALU_TYPE_BLA_W_LP && nal_unit_type <= AP4_HEVC_NALU_TYPE_BLA_N_LP) {
                m_AccessUnitFlags |= AP4_HEVC_ACCESS_UNIT_FLAG_IS_BLA;
            }
            if (nal_unit_type == AP4_HEVC_NALU_TYPE_RADL_N || nal_unit_type == AP4_HEVC_NALU_TYPE_RADL_R) {
                m_AccessUnitFlags |= AP4_HEVC_ACCESS_UNIT_FLAG_IS_RADL;
            }
            if (nal_unit_type == AP4_HEVC_NALU_TYPE_RASL_N || nal_unit_type == AP4_HEVC_NALU_TYPE_RASL_R) {
                m_AccessUnitFlags |= AP4_HEVC_ACCESS_UNIT_FLAG_IS_RASL;
            }
            if (nal_unit_type <= AP4_HEVC_NALU_TYPE_RSV_VCL_R15 && ((nal_unit_type & 1) == 0)) {
                m_AccessUnitFlags |= AP4_HEVC_ACCESS_UNIT_FLAG_IS_SUBLAYER_NON_REF;
            }
            
            // make this the current slice if this is the first slice in the access unit
            if (m_CurrentSlice == NULL) {
                m_CurrentSlice = slice_header;
            }
            
            // buffer this NAL unit
            AppendNalUnitData(nal_unit_payload, nal_unit_size);
            ++m_VclNalUnitsInAccessUnit;
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_AUD_NUT) {
            unsigned int pic_type = (nal_unit_payload[1]>>5);
            const char*  pic_type_name = AP4_HevcNalParser::PicTypeName(pic_type);
            if (pic_type_name == NULL) pic_type_name = "UNKNOWN";
            DBG_PRINTF_2("[%d:%s]\n", pic_type, pic_type_name);

            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_PPS_NUT) {
            AP4_HevcPictureParameterSet* pps = new AP4_HevcPictureParameterSet;
            result = pps->Parse(nal_unit_payload, nal_unit_size);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_0("PPS ERROR!!!");
                delete pps;
                return AP4_ERROR_INVALID_FORMAT;
            }
            delete m_PPS[pps->pps_pic_parameter_set_id];
            m_PPS[pps->pps_pic_parameter_set_id] = pps;
            DBG_PRINTF_2("PPS pps_id=%d, sps_id=%d", pps->pps_pic_parameter_set_id, pps->pps_seq_parameter_set_id);
            
            // keep the PPS with the NAL unit (this is optional)
            AppendNalUnitData(nal_unit_payload, nal_unit_size);
            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_SPS_NUT) {
            AP4_HevcSequenceParameterSet* sps = new AP4_HevcSequenceParameterSet;
            result = sps->Parse(nal_unit_payload, nal_unit_size);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_0("SPS ERROR!!!\n");
                delete sps;
                return AP4_ERROR_INVALID_FORMAT;
            }
            delete m_SPS[sps->sps_seq_parameter_set_id];
            m_SPS[sps->sps_seq_parameter_set_id] = sps;
            DBG_PRINTF_2("SPS sps_id=%d, vps_id=%d", sps->sps_seq_parameter_set_id, sps->sps_video_parameter_set_id);
            
            // keep the SPS with the NAL unit (this is optional)
            AppendNalUnitData(nal_unit_payload, nal_unit_size);
            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_VPS_NUT) {
            AP4_HevcVideoParameterSet* vps = new AP4_HevcVideoParameterSet;
            result = vps->Parse(nal_unit_payload, nal_unit_size);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_0("VPS ERROR!!!\n");
                delete vps;
                return AP4_ERROR_INVALID_FORMAT;
            }
            delete m_VPS[vps->vps_video_parameter_set_id];
            m_VPS[vps->vps_video_parameter_set_id] = vps;
            DBG_PRINTF_1("VPS vps_id=%d", vps->vps_video_parameter_set_id);
            
            // keep the VPS with the NAL unit (this is optional)
            AppendNalUnitData(nal_unit_payload, nal_unit_size);
            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_EOS_NUT ||
                   nal_unit_type == AP4_HEVC_NALU_TYPE_EOB_NUT) {
            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_PREFIX_SEI_NUT) {
            CheckIfAccessUnitIsCompleted(access_unit_info);
            AppendNalUnitData(nal_unit_payload, nal_unit_size);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_SUFFIX_SEI_NUT){
            AppendNalUnitData(nal_unit_payload, nal_unit_size);
        }
        DBG_PRINTF_0("\n");
        m_TotalNalUnitCount++;
    }
    
    // flush if needed
    if (eos && bytes_consumed == data_size && access_unit_info.nal_units.ItemCount() == 0) {
        DBG_PRINTF_0("------ last unit\n");
        CheckIfAccessUnitIsCompleted(access_unit_info);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_HevFrameParser::AccessUnitInfo::Reset
+---------------------------------------------------------------------*/
void
AP4_HevcFrameParser::AccessUnitInfo::Reset()
{
    for (unsigned int i=0; i<nal_units.ItemCount(); i++) {
        delete nal_units[i];
    }
    nal_units.Clear();
    is_random_access = false;
    decode_order = 0;
    display_order = 0;
}
