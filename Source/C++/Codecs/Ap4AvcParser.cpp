/*****************************************************************
|
|    AP4 - AVC Parser
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
#include "Ap4AvcParser.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   debugging
+---------------------------------------------------------------------*/
//#define AP4_AVC_PARSER_ENABLE_DEBUG
#if defined(AP4_AVC_PARSER_ENABLE_DEBUG)
#define DBG_PRINTF_0(_x0) printf(_x0)
#define DBG_PRINTF_1(_x0, _x1) printf(_x0, _x1)
#define DBG_PRINTF_2(_x0, _x1, _x2) printf(_x0, _x1, _x2)
#define DBG_PRINTF_3(_x0, _x1, _x2, _x3) printf(_x0, _x1, _x2, _x3)
#define DBG_PRINTF_4(_x0, _x1, _x2, _x3, _x4) printf(_x0, _x1, _x2, _x3, _x4)
#define DBG_PRINTF_5(_x0, _x1, _x2, _x3, _x4, _x5) printf(_x0, _x1, _x2, _x3, _x4, _x5)
#else
#define DBG_PRINTF_0(_x0)
#define DBG_PRINTF_1(_x0, _x1)
#define DBG_PRINTF_2(_x0, _x1, _x2)
#define DBG_PRINTF_3(_x0, _x1, _x2, _x3)
#define DBG_PRINTF_4(_x0, _x1, _x2, _x3, _x4)
#define DBG_PRINTF_5(_x0, _x1, _x2, _x3, _x4, _x5)
#endif

/*----------------------------------------------------------------------
|   AP4_AvcNalParser::NaluTypeName
+---------------------------------------------------------------------*/
const char*
AP4_AvcNalParser::NaluTypeName(unsigned int nalu_type)
{
    switch (nalu_type) {
        case  0: return "Unspecified";
        case  1: return "Coded slice of a non-IDR picture";
        case  2: return "Coded slice data partition A"; 
        case  3: return "Coded slice data partition B";
        case  4: return "Coded slice data partition C";
        case  5: return "Coded slice of an IDR picture";
        case  6: return "Supplemental enhancement information (SEI)";
        case  7: return "Sequence parameter set";
        case  8: return "Picture parameter set";
        case  9: return "Access unit delimiter";
        case 10: return "End of sequence";
        case 11: return "End of stream";
        case 12: return "Filler data";
        case 13: return "Sequence parameter set extension";
        case 14: return "Prefix NAL unit in scalable extension";
        case 15: return "Subset sequence parameter set";
        case 19: return "Coded slice of an auxiliary coded picture without partitioning";
        case 20: return "Coded slice in scalable extension";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_AvcNalParser::PrimaryPicTypeName
+---------------------------------------------------------------------*/
const char*
AP4_AvcNalParser::PrimaryPicTypeName(unsigned int primary_pic_type)
{
	switch (primary_pic_type) {
        case 0: return "I";
        case 1: return "I, P";
        case 2: return "I, P, B";
        case 3: return "SI";
        case 4: return "SI, SP";
        case 5: return "I, SI";
        case 6: return "I, SI, P, SP";
        case 7: return "I, SI, P, SP, B";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_AvcNalParser::SliceTypeName
+---------------------------------------------------------------------*/
const char*
AP4_AvcNalParser::SliceTypeName(unsigned int slice_type)
{
	switch (slice_type) {
        case 0: return "P";
        case 1: return "B";
        case 2: return "I";
        case 3:	return "SP";
        case 4: return "SI";
        case 5: return "P";
        case 6: return "B";
        case 7: return "I";
        case 8:	return "SP";
        case 9: return "SI";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_AvcNalParser::AP4_AvcNalParser
+---------------------------------------------------------------------*/
AP4_AvcNalParser::AP4_AvcNalParser() :
    AP4_NalParser()
{
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::AP4_AvcFrameParser
+---------------------------------------------------------------------*/
AP4_AvcFrameParser::AP4_AvcFrameParser() :
    m_NalUnitType(0),
    m_NalRefIdc(0),
    m_SliceHeader(NULL),
    m_AccessUnitVclNalUnitCount(0),
    m_TotalNalUnitCount(0),
    m_TotalAccessUnitCount(0),
    m_PrevFrameNum(0),
    m_PrevFrameNumOffset(0),
    m_PrevPicOrderCntMsb(0),
    m_PrevPicOrderCntLsb(0)
{
    for (unsigned int i=0; i<256; i++) {
        m_PPS[i] = NULL;
        m_SPS[i] = NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::~AP4_AvcFrameParser
+---------------------------------------------------------------------*/
AP4_AvcFrameParser::~AP4_AvcFrameParser()
{
    for (unsigned int i=0; i<256; i++) {
        delete m_PPS[i];
        delete m_SPS[i];
    }
    
    delete m_SliceHeader;
    
    // cleanup any un-transfered buffers
    for (unsigned int i=0; i<m_AccessUnitData.ItemCount(); i++) {
        delete m_AccessUnitData[i];
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
|   SignedGolomb
+---------------------------------------------------------------------*/
static int
SignedGolomb(unsigned int code_num)
{
    if (code_num % 2) {
        return (code_num+1)/2;
    } else {
        return -((int)code_num/2);
    }
}

/*----------------------------------------------------------------------
|   AP4_AvcSequenceParameterSet::AP4_AvcSequenceParameterSet
+---------------------------------------------------------------------*/
AP4_AvcSequenceParameterSet::AP4_AvcSequenceParameterSet() :
    profile_idc(0),
    constraint_set0_flag(0),
    constraint_set1_flag(0),
    constraint_set2_flag(0),
    constraint_set3_flag(0),
    level_idc(0),
    seq_parameter_set_id(0),
    chroma_format_idc(1),
    separate_colour_plane_flag(0),
    bit_depth_luma_minus8(0),
    bit_depth_chroma_minus8(0),
    qpprime_y_zero_transform_bypass_flag(0),
    seq_scaling_matrix_present_flag(0),
    log2_max_frame_num_minus4(0),
    pic_order_cnt_type(0),
    log2_max_pic_order_cnt_lsb_minus4(0),
    delta_pic_order_always_zero_flags(0),
    offset_for_non_ref_pic(0),
    offset_for_top_to_bottom_field(0),
    num_ref_frames_in_pic_order_cnt_cycle(0),
    num_ref_frames(0),
    gaps_in_frame_num_value_allowed_flag(0),
    pic_width_in_mbs_minus1(0),
    pic_height_in_map_units_minus1(0),
    frame_mbs_only_flag(0),
    mb_adaptive_frame_field_flag(0),
    direct_8x8_inference_flag(0),
    frame_cropping_flag(0),
    frame_crop_left_offset(0),
    frame_crop_right_offset(0),
    frame_crop_top_offset(0),
    frame_crop_bottom_offset(0)
{
    AP4_SetMemory(scaling_list_4x4, 0, sizeof(scaling_list_4x4));
    AP4_SetMemory(use_default_scaling_matrix_4x4, 0, sizeof(use_default_scaling_matrix_4x4));
    AP4_SetMemory(scaling_list_8x8, 0, sizeof(scaling_list_8x8));
    AP4_SetMemory(use_default_scaling_matrix_8x8, 0, sizeof(use_default_scaling_matrix_8x8));
    AP4_SetMemory(offset_for_ref_frame, 0, sizeof(offset_for_ref_frame));
}

/*----------------------------------------------------------------------
|   AP4_AvcSequenceParameterSet::GetInfo
+---------------------------------------------------------------------*/
void
AP4_AvcSequenceParameterSet::GetInfo(unsigned int& width, unsigned int& height)
{
    width = (pic_width_in_mbs_minus1+1) * 16;
	height = (2-frame_mbs_only_flag) * (pic_height_in_map_units_minus1+1) * 16;

    if (frame_cropping_flag) {
        unsigned int crop_h = 2*(frame_crop_left_offset+frame_crop_right_offset);
        unsigned int crop_v = 2*(frame_crop_top_offset+frame_crop_bottom_offset)*(2-frame_mbs_only_flag);
		if (crop_h < width) width   -= crop_h;
		if (crop_v < height) height -= crop_v;
	}
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::ParseSPS
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcFrameParser::ParseSPS(const unsigned char*         data,
                             unsigned int                 data_size,
                             AP4_AvcSequenceParameterSet& sps)
{
    sps.raw_bytes.SetData(data, data_size);
    AP4_DataBuffer unescaped(data, data_size);
    AP4_NalParser::Unescape(unescaped);
    AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());

    bits.SkipBits(8); // NAL Unit Type

    sps.profile_idc = bits.ReadBits(8);
    sps.constraint_set0_flag = bits.ReadBit();
    sps.constraint_set1_flag = bits.ReadBit();
    sps.constraint_set2_flag = bits.ReadBit();
    sps.constraint_set3_flag = bits.ReadBit();
    bits.SkipBits(4);
    sps.level_idc = bits.ReadBits(8);
    sps.seq_parameter_set_id = ReadGolomb(bits);
    if (sps.seq_parameter_set_id > AP4_AVC_SPS_MAX_ID) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    if (sps.profile_idc  ==  100  ||
        sps.profile_idc  ==  110  ||
        sps.profile_idc  ==  122  ||
        sps.profile_idc  ==  244  ||
        sps.profile_idc  ==  44   ||
        sps.profile_idc  ==  83   ||
        sps.profile_idc  ==  86) {
        sps.chroma_format_idc = ReadGolomb(bits);
        sps.separate_colour_plane_flag = 0;
        if (sps.chroma_format_idc == 3) {
            sps.separate_colour_plane_flag = bits.ReadBit();
        }
        sps.bit_depth_luma_minus8 = ReadGolomb(bits);
        sps.bit_depth_chroma_minus8 = ReadGolomb(bits);
        sps.qpprime_y_zero_transform_bypass_flag = bits.ReadBit();
        sps.seq_scaling_matrix_present_flag = bits.ReadBit();
        if (sps.seq_scaling_matrix_present_flag) {
            for (int i=0; i<(sps.chroma_format_idc != 3 ? 8 : 12); i++) {
                unsigned int seq_scaling_list_present_flag = bits.ReadBit();
                if (seq_scaling_list_present_flag) {
                    if (i<6) {
                        int last_scale = 8;
                        int next_scale = 8;
                        for (unsigned int j=0; j<16; j++) {
                            if (next_scale) {
                                int delta_scale = SignedGolomb(ReadGolomb(bits));
                                next_scale = (last_scale + delta_scale + 256) % 256;
                                sps.use_default_scaling_matrix_4x4[i] = (j == 0 && next_scale == 0);
                            }
                            sps.scaling_list_4x4[i].scale[j] = (next_scale == 0 ? last_scale : next_scale);
                            last_scale = sps.scaling_list_4x4[i].scale[j];
                        }
                    } else {
                        int last_scale = 8;
                        int next_scale = 8;
                        for (unsigned int j=0; j<64; j++) {
                            if (next_scale) {
                                int delta_scale = SignedGolomb(ReadGolomb(bits));
                                next_scale = (last_scale + delta_scale + 256) % 256;
                                sps.use_default_scaling_matrix_8x8[i-6] = (j == 0 && next_scale == 0);
                            }
                            sps.scaling_list_8x8[i-6].scale[j] = (next_scale == 0 ? last_scale : next_scale);
                            last_scale = sps.scaling_list_8x8[i-6].scale[j];
                        }
                    }
                }
            }
        }
    }
    sps.log2_max_frame_num_minus4 = ReadGolomb(bits);
    sps.pic_order_cnt_type = ReadGolomb(bits);
    if (sps.pic_order_cnt_type > 2) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    if (sps.pic_order_cnt_type == 0) {
        sps.log2_max_pic_order_cnt_lsb_minus4 = ReadGolomb(bits);
    } else if (sps.pic_order_cnt_type == 1) {
        sps.delta_pic_order_always_zero_flags = bits.ReadBit();
        sps.offset_for_non_ref_pic = SignedGolomb(ReadGolomb(bits));
        sps.offset_for_top_to_bottom_field = SignedGolomb(ReadGolomb(bits));
        sps.num_ref_frames_in_pic_order_cnt_cycle = ReadGolomb(bits);
        if (sps.num_ref_frames_in_pic_order_cnt_cycle > AP4_AVC_SPS_MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE) {
            return AP4_ERROR_INVALID_FORMAT;
        }
        for (unsigned int i=0; i<sps.num_ref_frames_in_pic_order_cnt_cycle; i++) {
            sps.offset_for_ref_frame[i] = SignedGolomb(ReadGolomb(bits));
        }
    }
    sps.num_ref_frames                       = ReadGolomb(bits);
    sps.gaps_in_frame_num_value_allowed_flag = bits.ReadBit();
    sps.pic_width_in_mbs_minus1              = ReadGolomb(bits);
    sps.pic_height_in_map_units_minus1       = ReadGolomb(bits);
    sps.frame_mbs_only_flag                  = bits.ReadBit();
    if (!sps.frame_mbs_only_flag) {
        sps.mb_adaptive_frame_field_flag = bits.ReadBit();
    }
    sps.direct_8x8_inference_flag = bits.ReadBit();
    sps.frame_cropping_flag       = bits.ReadBit();
    if (sps.frame_cropping_flag) {
        sps.frame_crop_left_offset   = ReadGolomb(bits);
        sps.frame_crop_right_offset  = ReadGolomb(bits);
        sps.frame_crop_top_offset    = ReadGolomb(bits);
        sps.frame_crop_bottom_offset = ReadGolomb(bits);
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AvcPictureParameterSet::AP4_PictureParameterSet
+---------------------------------------------------------------------*/
AP4_AvcPictureParameterSet::AP4_AvcPictureParameterSet() :
    pic_parameter_set_id(0),
    seq_parameter_set_id(0),
    entropy_coding_mode_flag(0),
    pic_order_present_flag(0),
    num_slice_groups_minus1(0),
    slice_group_map_type(0),
    slice_group_change_direction_flag(0),
    slice_group_change_rate_minus1(0),
    pic_size_in_map_units_minus1(0),
    num_ref_idx_10_active_minus1(0),
    num_ref_idx_11_active_minus1(0),
    weighted_pred_flag(0),
    weighted_bipred_idc(0),
    pic_init_qp_minus26(0),
    pic_init_qs_minus26(0),
    chroma_qp_index_offset(0),
    deblocking_filter_control_present_flag(0),
    constrained_intra_pred_flag(0),
    redundant_pic_cnt_present_flag(0)
{
    AP4_SetMemory(run_length_minus1, 0, sizeof(run_length_minus1));
    AP4_SetMemory(top_left, 0, sizeof(top_left));
    AP4_SetMemory(bottom_right, 0, sizeof(bottom_right));
 }

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::ParsePPS
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcFrameParser::ParsePPS(const unsigned char*        data,
                             unsigned int                data_size,
                             AP4_AvcPictureParameterSet& pps)
{
    pps.raw_bytes.SetData(data, data_size);
    AP4_DataBuffer unescaped(data, data_size);
    AP4_NalParser::Unescape(unescaped);
    AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());
    
    bits.SkipBits(8); // NAL Unit Type

    pps.pic_parameter_set_id     = ReadGolomb(bits);
    if (pps.pic_parameter_set_id > AP4_AVC_PPS_MAX_ID) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    pps.seq_parameter_set_id     = ReadGolomb(bits);
    if (pps.seq_parameter_set_id > AP4_AVC_SPS_MAX_ID) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    pps.entropy_coding_mode_flag = bits.ReadBit();
    pps.pic_order_present_flag   = bits.ReadBit();
    pps.num_slice_groups_minus1  = ReadGolomb(bits);
    if (pps.num_slice_groups_minus1 >= AP4_AVC_PPS_MAX_SLICE_GROUPS) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    if (pps.num_slice_groups_minus1 > 0) {
        pps.slice_group_map_type = ReadGolomb(bits);
        if (pps.slice_group_map_type == 0) {
            for (unsigned int i=0; i<=pps.num_slice_groups_minus1; i++) {
                pps.run_length_minus1[i] = ReadGolomb(bits);
            }
        } else if (pps.slice_group_map_type == 2) {
            for (unsigned int i=0; i<pps.num_slice_groups_minus1; i++) {
                pps.top_left[i] = ReadGolomb(bits);
                pps.bottom_right[i] = ReadGolomb(bits);
            }
        } else if (pps.slice_group_map_type == 3 ||
                   pps.slice_group_map_type == 4 ||
                   pps.slice_group_map_type == 5) {
            pps.slice_group_change_direction_flag = bits.ReadBit();
            pps.slice_group_change_rate_minus1 = ReadGolomb(bits);
        } else if (pps.slice_group_map_type == 6) {
            pps.pic_size_in_map_units_minus1 = ReadGolomb(bits);
            if (pps.pic_size_in_map_units_minus1 >= AP4_AVC_PPS_MAX_PIC_SIZE_IN_MAP_UNITS) {
                return AP4_ERROR_INVALID_FORMAT;
            }
            unsigned int num_bits_per_slice_group_id;
            if (pps.num_slice_groups_minus1 + 1 > 4) {
                num_bits_per_slice_group_id = 3;
            } else if (pps.num_slice_groups_minus1 + 1 > 2) {
                num_bits_per_slice_group_id = 2;
            } else {
                num_bits_per_slice_group_id = 1;
            }
            for (unsigned int i=0; i<=pps.pic_size_in_map_units_minus1; i++) {
                /*pps.slice_group_id[i] =*/ bits.ReadBits(num_bits_per_slice_group_id);
            }
        }
    }
    pps.num_ref_idx_10_active_minus1 = ReadGolomb(bits);
    pps.num_ref_idx_11_active_minus1 = ReadGolomb(bits);
    pps.weighted_pred_flag           = bits.ReadBit();
    pps.weighted_bipred_idc          = bits.ReadBits(2);
    pps.pic_init_qp_minus26          = SignedGolomb(ReadGolomb(bits));
    pps.pic_init_qs_minus26          = SignedGolomb(ReadGolomb(bits));
    pps.chroma_qp_index_offset       = SignedGolomb(ReadGolomb(bits));
    pps.deblocking_filter_control_present_flag = bits.ReadBit();
    pps.constrained_intra_pred_flag            = bits.ReadBit();
    pps.redundant_pic_cnt_present_flag         = bits.ReadBit();
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AvcSliceHeader::AP4_AvcSliceHeader
+---------------------------------------------------------------------*/
AP4_AvcSliceHeader::AP4_AvcSliceHeader() :
    size(0),
    first_mb_in_slice(0),
    slice_type(0),
    pic_parameter_set_id(0),
    colour_plane_id(0),
    frame_num(0),
    field_pic_flag(0),
    bottom_field_flag(0),
    idr_pic_id(0),
    pic_order_cnt_lsb(0),
    redundant_pic_cnt(0),
    direct_spatial_mv_pred_flag(0),
    num_ref_idx_active_override_flag(0),
    num_ref_idx_l0_active_minus1(0),
    num_ref_idx_l1_active_minus1(0),
    ref_pic_list_reordering_flag_l0(0),
    reordering_of_pic_nums_idc(0),
    abs_diff_pic_num_minus1(0),
    long_term_pic_num(0),
    ref_pic_list_reordering_flag_l1(0),
    luma_log2_weight_denom(0),
    chroma_log2_weight_denom(0),
    cabac_init_idc(0),
    slice_qp_delta(0),
    sp_for_switch_flag(0),
    slice_qs_delta(0),
    disable_deblocking_filter_idc(0),
    slice_alpha_c0_offset_div2(0),
    slice_beta_offset_div2(0),
    slice_group_change_cycle(0),
    no_output_of_prior_pics_flag(0),
    long_term_reference_flag(0),
    difference_of_pic_nums_minus1(0),
    long_term_frame_idx(0),
    max_long_term_frame_idx_plus1(0)
{
    delta_pic_order_cnt[0] = delta_pic_order_cnt[1] = 0;
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::ParseSliceHeader
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcFrameParser::ParseSliceHeader(const AP4_UI08*     data,
                                     unsigned int        data_size,
                                     unsigned int        nal_unit_type,
                                     unsigned int        nal_ref_idc,
                                     AP4_AvcSliceHeader& slice_header)
{
    AP4_DataBuffer unescaped(data, data_size);
    AP4_NalParser::Unescape(unescaped);
    AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());

    // init the computer fields
    slice_header.size = 0;
    
    slice_header.first_mb_in_slice    = ReadGolomb(bits);
    slice_header.slice_type           = ReadGolomb(bits);
    slice_header.pic_parameter_set_id = ReadGolomb(bits);
    if (slice_header.pic_parameter_set_id > AP4_AVC_PPS_MAX_ID) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    const AP4_AvcPictureParameterSet* pps = m_PPS[slice_header.pic_parameter_set_id];
    if (pps == NULL) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    const AP4_AvcSequenceParameterSet* sps = m_SPS[pps->seq_parameter_set_id];
    if (sps == NULL) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    if (sps->separate_colour_plane_flag) {
        slice_header.colour_plane_id = bits.ReadBits(2);
    }
    slice_header.frame_num = bits.ReadBits(sps->log2_max_frame_num_minus4 + 4);
    if (!sps->frame_mbs_only_flag) {
        slice_header.field_pic_flag = bits.ReadBit();
        if (slice_header.field_pic_flag) {
            slice_header.bottom_field_flag = bits.ReadBit();
        }
    }
    if (nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE) {
        slice_header.idr_pic_id = ReadGolomb(bits);
    }
    if (sps->pic_order_cnt_type == 0) {
        slice_header.pic_order_cnt_lsb = bits.ReadBits(sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pps->pic_order_present_flag && !slice_header.field_pic_flag) {
            slice_header.delta_pic_order_cnt[0] = SignedGolomb(ReadGolomb(bits));
        }
    }
    if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flags) {
        slice_header.delta_pic_order_cnt[0] = SignedGolomb(ReadGolomb(bits));
        if (pps->pic_order_present_flag && !slice_header.field_pic_flag) {
            slice_header.delta_pic_order_cnt[1] = SignedGolomb(ReadGolomb(bits));
        }
    }
    if (pps->redundant_pic_cnt_present_flag) {
        slice_header.redundant_pic_cnt = ReadGolomb(bits);
    }
    
    unsigned int slice_type = slice_header.slice_type % 5; // this seems to be implicit in the spec
    
    if (slice_type == AP4_AVC_SLICE_TYPE_B) {
        slice_header.direct_spatial_mv_pred_flag = bits.ReadBit();
    }
    
    if (slice_type == AP4_AVC_SLICE_TYPE_P  ||
        slice_type == AP4_AVC_SLICE_TYPE_SP ||
        slice_type == AP4_AVC_SLICE_TYPE_B) {
        slice_header.num_ref_idx_active_override_flag = bits.ReadBit();
        
        if (slice_header.num_ref_idx_active_override_flag) {
            slice_header.num_ref_idx_l0_active_minus1 = ReadGolomb(bits);
            if ((slice_header.slice_type % 5) == AP4_AVC_SLICE_TYPE_B) {
                slice_header.num_ref_idx_l1_active_minus1 = ReadGolomb(bits);
            }
        } else {
            slice_header.num_ref_idx_l0_active_minus1 = pps->num_ref_idx_10_active_minus1;
            slice_header.num_ref_idx_l1_active_minus1 = pps->num_ref_idx_11_active_minus1;
        }
    }
    
    // ref_pic_list_reordering
    if ((slice_header.slice_type % 5) != 2 && (slice_header.slice_type % 5) != 4) {
        slice_header.ref_pic_list_reordering_flag_l0 = bits.ReadBit();
        if (slice_header.ref_pic_list_reordering_flag_l0) {
            do {
                slice_header.reordering_of_pic_nums_idc = ReadGolomb(bits);
                if (slice_header.reordering_of_pic_nums_idc == 0 ||
					slice_header.reordering_of_pic_nums_idc == 1) {
                    slice_header.abs_diff_pic_num_minus1 = ReadGolomb(bits);
                } else if (slice_header.reordering_of_pic_nums_idc == 2) {
                    slice_header.long_term_pic_num = ReadGolomb(bits);
                }
            } while (slice_header.reordering_of_pic_nums_idc != 3);
        }
    }
    if ((slice_header.slice_type % 5) == 1) {
        slice_header.ref_pic_list_reordering_flag_l1 = bits.ReadBit();
        if (slice_header.ref_pic_list_reordering_flag_l1) {
            do {
                slice_header.reordering_of_pic_nums_idc = ReadGolomb(bits);
                if (slice_header.reordering_of_pic_nums_idc == 0 ||
					slice_header.reordering_of_pic_nums_idc == 1) {
                    slice_header.abs_diff_pic_num_minus1 = ReadGolomb(bits);
                } else if (slice_header.reordering_of_pic_nums_idc == 2) {
                    slice_header.long_term_pic_num = ReadGolomb(bits);
                }
            } while (slice_header.reordering_of_pic_nums_idc != 3);
        }
    }
    
    if ((pps->weighted_pred_flag &&
        (slice_type == AP4_AVC_SLICE_TYPE_P || slice_type == AP4_AVC_SLICE_TYPE_SP)) ||
		(pps->weighted_bipred_idc == 1 && slice_type == AP4_AVC_SLICE_TYPE_B)) {
        // pred_weight_table
        slice_header.luma_log2_weight_denom = ReadGolomb(bits);
        
        if (sps->chroma_format_idc != 0) {
            slice_header.chroma_log2_weight_denom = ReadGolomb(bits);
        }
        
        for (unsigned int i=0; i<=slice_header.num_ref_idx_l0_active_minus1; i++) {
            unsigned int luma_weight_l0_flag = bits.ReadBit();
            if (luma_weight_l0_flag) {
                /* slice_header.luma_weight_l0[i] = SignedGolomb( */ ReadGolomb(bits);
                /* slice_header.luma_offset_l0[i] = SignedGolomb( */ ReadGolomb(bits);
            }
            if (sps->chroma_format_idc != 0) {
                unsigned int chroma_weight_l0_flag = bits.ReadBit();
                if (chroma_weight_l0_flag) {
                    for (unsigned int j=0; j<2; j++) {
                        /* slice_header.chroma_weight_l0[i][j] = SignedGolomb( */ ReadGolomb(bits);
                        /* slice_header.chroma_offset_l0[i][j] = SignedGolomb( */ ReadGolomb(bits);
                    }
                }
            }
        }
        if ((slice_header.slice_type % 5) == 1) {
            for (unsigned int i=0; i<=slice_header.num_ref_idx_l1_active_minus1; i++) {
                unsigned int luma_weight_l1_flag = bits.ReadBit();
                if (luma_weight_l1_flag) {
                    /* slice_header.luma_weight_l1[i] = SignedGolomb( */ ReadGolomb(bits);
                    /* slice_header.luma_offset_l1[i] = SignedGolomb( */ ReadGolomb(bits);
                }
                if (sps->chroma_format_idc != 0) {
                    unsigned int chroma_weight_l1_flag = bits.ReadBit();
                    if (chroma_weight_l1_flag) {
                        for (unsigned int j=0; j<2; j++) {
                            /* slice_header.chroma_weight_l1[i][j] = SignedGolomb( */ ReadGolomb(bits);
                            /* slice_header.chroma_offset_l1[i][j] = SignedGolomb( */ ReadGolomb(bits);
                        }
                    }
                }
            }
        }
    }

    if (nal_ref_idc != 0) {
        // dec_ref_pic_marking
        if (nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE) {
            slice_header.no_output_of_prior_pics_flag = bits.ReadBit();
            slice_header.long_term_reference_flag     = bits.ReadBit();
        } else {
            unsigned int adaptive_ref_pic_marking_mode_flag = bits.ReadBit();
            if (adaptive_ref_pic_marking_mode_flag) {
                unsigned int memory_management_control_operation = 0;
                do {
                    memory_management_control_operation = ReadGolomb(bits);
                    if (memory_management_control_operation == 1 || memory_management_control_operation == 3) {
                        slice_header.difference_of_pic_nums_minus1 = ReadGolomb(bits);
                    }
                    if (memory_management_control_operation == 2) {
                        slice_header.long_term_pic_num = ReadGolomb(bits);
                    }
                    if (memory_management_control_operation == 3 || memory_management_control_operation == 6) {
                        slice_header.long_term_frame_idx = ReadGolomb(bits);
                    }
                    if (memory_management_control_operation == 4) {
                        slice_header.max_long_term_frame_idx_plus1 = ReadGolomb(bits);
                    }
                } while (memory_management_control_operation != 0);
            }
        }
    }
    if (pps->entropy_coding_mode_flag && slice_type != AP4_AVC_SLICE_TYPE_I && slice_type != AP4_AVC_SLICE_TYPE_SI) {
        slice_header.cabac_init_idc = ReadGolomb(bits);
    }
    slice_header.slice_qp_delta = ReadGolomb(bits);
    if (slice_type == AP4_AVC_SLICE_TYPE_SP || slice_type == AP4_AVC_SLICE_TYPE_SI) {
        if (slice_type == AP4_AVC_SLICE_TYPE_SP) {
            slice_header.sp_for_switch_flag = bits.ReadBit();
        }
        slice_header.slice_qs_delta = SignedGolomb(ReadGolomb(bits));
    }
    if (pps->deblocking_filter_control_present_flag) {
        slice_header.disable_deblocking_filter_idc = ReadGolomb(bits);
        if (slice_header.disable_deblocking_filter_idc != 1) {
            slice_header.slice_alpha_c0_offset_div2 = SignedGolomb(ReadGolomb(bits));
            slice_header.slice_beta_offset_div2     = SignedGolomb(ReadGolomb(bits));
        }
    }
    if (pps->num_slice_groups_minus1 > 0 &&
        pps->slice_group_map_type >= 3   &&
        pps->slice_group_map_type <= 5) {
        slice_header.slice_group_change_cycle = ReadGolomb(bits);
    }

    /* compute the size */
    slice_header.size = bits.GetBitsRead();
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::GetSliceSPS
+---------------------------------------------------------------------*/
AP4_AvcSequenceParameterSet*
AP4_AvcFrameParser::GetSliceSPS(AP4_AvcSliceHeader& sh)
{
    // lookup the SPS
    AP4_AvcPictureParameterSet*  pps = m_PPS[sh.pic_parameter_set_id];
    if (pps == NULL) return NULL;
    return m_SPS[pps->seq_parameter_set_id];
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::SameFrame
|   14496-10 7.4.1.2.4 Detection of the first VCL NAL unit of a primary coded picture
+---------------------------------------------------------------------*/
bool
AP4_AvcFrameParser::SameFrame(unsigned int nal_unit_type_1, unsigned int nal_ref_idc_1, AP4_AvcSliceHeader& sh1,
                              unsigned int nal_unit_type_2, unsigned int nal_ref_idc_2, AP4_AvcSliceHeader& sh2)
{
    if (sh1.frame_num != sh2.frame_num) {
        return false;
    }
    
    if (sh1.pic_parameter_set_id != sh2.pic_parameter_set_id) {
        return false;
    }
    
    if (sh1.field_pic_flag != sh2.field_pic_flag) {
        return false;
    }

    if (sh1.field_pic_flag) {
        if (sh1.bottom_field_flag != sh2.bottom_field_flag) {
            return false;
        }
    }
    
    if ((nal_ref_idc_1 == 0 || nal_ref_idc_2 == 0) && (nal_ref_idc_1 != nal_ref_idc_2)) {
        return false;
    }
    
    // lookup the SPS
    AP4_AvcSequenceParameterSet* sps = GetSliceSPS(sh1);
    if (sps == NULL) return false;
    
    if (sps->pic_order_cnt_type == 0) {
        if (sh1.pic_order_cnt_lsb      != sh2.pic_order_cnt_lsb ||
            sh1.delta_pic_order_cnt[0] != sh2.delta_pic_order_cnt[0]) {
            return false;
        }
    }
    
    if (sps->pic_order_cnt_type == 1) {
        if (sh1.delta_pic_order_cnt[0] != sh2.delta_pic_order_cnt[0] ||
            sh1.delta_pic_order_cnt[1] != sh2.delta_pic_order_cnt[1]) {
            return false;
        }
    }
    
    if (nal_unit_type_1 == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE ||
        nal_unit_type_2 == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE) {
        if (nal_unit_type_1 != nal_unit_type_2) {
            return false;
        }
    }
    
    if (nal_unit_type_1 == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE &&
        nal_unit_type_2 == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE) {
        if (sh1.idr_pic_id != sh2.idr_pic_id) {
            return false;
        }
    }
    
    return true;
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::CheckIfAccessUnitIsCompleted
+---------------------------------------------------------------------*/
void
AP4_AvcFrameParser::CheckIfAccessUnitIsCompleted(AccessUnitInfo& access_unit_info)
{
    if (m_SliceHeader == NULL) {
        return;
    }
    if (!m_AccessUnitVclNalUnitCount) {
        return;
    }
    DBG_PRINTF_0(">>>>>>> New Access Unit\n");
    m_AccessUnitVclNalUnitCount = 0;
    
    AP4_AvcSequenceParameterSet* sps = GetSliceSPS(*m_SliceHeader);
    if (sps == NULL) return;
    int max_frame_num = 1 << (sps->log2_max_frame_num_minus4 + 4);
    
    // compute the picture type
    enum {
        AP4_AVC_PIC_TYPE_FRAME,
        AP4_AVC_PIC_TYPE_TOP_FIELD,
        AP4_AVC_PIC_TYPE_BOTTOM_FIELD
    } pic_type;
	if (sps->frame_mbs_only_flag || !m_SliceHeader->field_pic_flag) {
        pic_type = AP4_AVC_PIC_TYPE_FRAME;
    } else if (m_SliceHeader->bottom_field_flag) {
        pic_type = AP4_AVC_PIC_TYPE_BOTTOM_FIELD;
    } else {
        pic_type = AP4_AVC_PIC_TYPE_TOP_FIELD;
    }

    // prepare to compute the picture order count
    int top_field_pic_order_cnt = 0;
    int bottom_field_pic_order_cnt = 0;
    unsigned int frame_num_offset = 0;
    unsigned int frame_num = m_SliceHeader->frame_num;
    if (m_NalUnitType == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE) {
        m_PrevPicOrderCntMsb = 0;
        m_PrevPicOrderCntLsb = 0;
    } else {
        if (frame_num < m_PrevFrameNum) {
            // wrap around
            frame_num_offset = m_PrevFrameNumOffset + max_frame_num;
        } else {
            frame_num_offset = m_PrevFrameNumOffset;
        }
    }
    
    // compute the picture order count
    int pic_order_cnt_msb = 0;
    if (sps->pic_order_cnt_type == 0) {
		unsigned int max_pic_order_cnt_lsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (m_SliceHeader->pic_order_cnt_lsb < m_PrevPicOrderCntLsb &&
            m_PrevPicOrderCntLsb - m_SliceHeader->pic_order_cnt_lsb >= max_pic_order_cnt_lsb/2) {
            pic_order_cnt_msb = m_PrevPicOrderCntMsb + max_pic_order_cnt_lsb;
        } else if (m_SliceHeader->pic_order_cnt_lsb > m_PrevPicOrderCntLsb &&
                   m_SliceHeader->pic_order_cnt_lsb - m_PrevPicOrderCntLsb > max_pic_order_cnt_lsb/2) {
            pic_order_cnt_msb = m_PrevPicOrderCntMsb - max_pic_order_cnt_lsb;
        } else {
            pic_order_cnt_msb = m_PrevPicOrderCntMsb;
        }
        
        if (pic_type != AP4_AVC_PIC_TYPE_BOTTOM_FIELD) {
            top_field_pic_order_cnt = pic_order_cnt_msb+m_SliceHeader->pic_order_cnt_lsb;
        }
        if (pic_type != AP4_AVC_PIC_TYPE_TOP_FIELD) {
            if (!m_SliceHeader->field_pic_flag) {
                bottom_field_pic_order_cnt = top_field_pic_order_cnt + m_SliceHeader->delta_pic_order_cnt[0];
            } else {
                bottom_field_pic_order_cnt = pic_order_cnt_msb + m_SliceHeader->pic_order_cnt_lsb;
            }
        }
    } else if (sps->pic_order_cnt_type == 1) {
        unsigned int abs_frame_num = 0;
        if (sps->num_ref_frames_in_pic_order_cnt_cycle) {
            abs_frame_num = frame_num_offset + frame_num;
        }
        if (m_NalRefIdc == 0 && abs_frame_num > 0) {
            --abs_frame_num;
        }
        
        int expected_pic_order_cnt = 0;
        if (abs_frame_num >0) {
            unsigned int pic_order_cnt_cycle_cnt = (abs_frame_num-1)/sps->num_ref_frames_in_pic_order_cnt_cycle;
            unsigned int frame_num_in_pic_order_cnt_cycle = (abs_frame_num-1)%sps->num_ref_frames_in_pic_order_cnt_cycle;
            
            int expected_delta_per_pic_order_cnt_cycle = 0;
            for (unsigned int i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
                expected_delta_per_pic_order_cnt_cycle += sps->offset_for_ref_frame[i];
            }
            expected_pic_order_cnt = pic_order_cnt_cycle_cnt * expected_delta_per_pic_order_cnt_cycle;
            for (unsigned int i=0; i<frame_num_in_pic_order_cnt_cycle; i++) {
                expected_pic_order_cnt += sps->offset_for_ref_frame[i];
            }
        }
        if (m_NalRefIdc == 0) {
            expected_pic_order_cnt += sps->offset_for_non_ref_pic;
        }
    
        if (!m_SliceHeader->field_pic_flag) {
            top_field_pic_order_cnt    = expected_pic_order_cnt + m_SliceHeader->delta_pic_order_cnt[0];
            bottom_field_pic_order_cnt = top_field_pic_order_cnt + sps->offset_for_top_to_bottom_field + m_SliceHeader->delta_pic_order_cnt[1];
        } else if (!m_SliceHeader->bottom_field_flag) {
            top_field_pic_order_cnt = expected_pic_order_cnt + m_SliceHeader->delta_pic_order_cnt[0];
        } else {
            bottom_field_pic_order_cnt = expected_pic_order_cnt + sps->offset_for_top_to_bottom_field + m_SliceHeader->delta_pic_order_cnt[0];
        }
    } else if (sps->pic_order_cnt_type == 2) {
		int pic_order_cnt;
        if (m_NalUnitType == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE) {
            pic_order_cnt = 0;
		} else if (m_NalRefIdc == 0) {
            pic_order_cnt = 2*(frame_num_offset+frame_num)-1;
        } else {
            pic_order_cnt = 2*(frame_num_offset+frame_num);
        }
        
        if (!m_SliceHeader->field_pic_flag) {
            top_field_pic_order_cnt    = pic_order_cnt;
            bottom_field_pic_order_cnt = pic_order_cnt;
        } else if (m_SliceHeader->bottom_field_flag) {
            bottom_field_pic_order_cnt = pic_order_cnt;
        } else {
            top_field_pic_order_cnt = pic_order_cnt;
        }
    }

    unsigned int pic_order_cnt;
	if (pic_type == AP4_AVC_PIC_TYPE_FRAME) {
		pic_order_cnt = top_field_pic_order_cnt < bottom_field_pic_order_cnt ? top_field_pic_order_cnt : bottom_field_pic_order_cnt;
	} else if (pic_type == AP4_AVC_PIC_TYPE_TOP_FIELD) {
		pic_order_cnt = top_field_pic_order_cnt;
	} else {
		pic_order_cnt = bottom_field_pic_order_cnt;
    }
    
    // emit the access unit (transfer ownership)
    access_unit_info.nal_units     = m_AccessUnitData;
    access_unit_info.is_idr        = (m_NalUnitType == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE);
    access_unit_info.decode_order  = m_TotalAccessUnitCount;
    access_unit_info.display_order = pic_order_cnt;
    m_AccessUnitData.Clear();
    ++m_TotalAccessUnitCount;
    
    // update state
    m_PrevFrameNum       = frame_num;
    m_PrevFrameNumOffset = frame_num_offset;
    if (m_NalRefIdc) {
        m_PrevPicOrderCntMsb = pic_order_cnt_msb;
        m_PrevPicOrderCntLsb = m_SliceHeader->pic_order_cnt_lsb;
    }
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::AppendNalUnitData
+---------------------------------------------------------------------*/
void
AP4_AvcFrameParser::AppendNalUnitData(const unsigned char* data, unsigned int data_size)
{
    m_AccessUnitData.Append(new AP4_DataBuffer(data, data_size));
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::Feed
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcFrameParser::Feed(const void*     data,
                         AP4_Size        data_size,
                         AP4_Size&       bytes_consumed,
                         AccessUnitInfo& access_unit_info,
                         bool            eos)
{
    const AP4_DataBuffer* nal_unit = NULL;

    // feed the NAL unit parser
    AP4_Result result = m_NalParser.Feed(data, data_size, bytes_consumed, nal_unit, eos);
    if (AP4_FAILED(result)) {
        return result;
    }
    
    if (bytes_consumed < data_size) {
        // there will be more to parse
        eos = false;
    }
    
    return Feed(nal_unit ? nal_unit->GetData() : NULL,
                nal_unit ? nal_unit->GetDataSize() : 0,
                access_unit_info,
                eos);
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::Feed
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcFrameParser::Feed(const AP4_UI08* nal_unit,
                         AP4_Size        nal_unit_size,
                         AccessUnitInfo& access_unit_info,
                         bool            last_unit)
{
    AP4_Result result;
    
    // default return values
    access_unit_info.Reset();
    
    if (nal_unit && nal_unit_size) {
        unsigned int nal_unit_type = nal_unit[0]&0x1F;
        const char*  nal_unit_type_name = AP4_AvcNalParser::NaluTypeName(nal_unit_type);
        unsigned int nal_ref_idc = (nal_unit[0]>>5)&3;
        if (nal_unit_type_name == NULL) nal_unit_type_name = "UNKNOWN";
        DBG_PRINTF_5("NALU %5d: ref=%d, size=%5d, type=%02d (%s) ",
               m_TotalNalUnitCount,
               nal_ref_idc,
               nal_unit_size,
               nal_unit_type,
               nal_unit_type_name);
        if (nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_ACCESS_UNIT_DELIMITER) {
            unsigned int primary_pic_type = (nal_unit[1]>>5);
            const char*  primary_pic_type_name = AP4_AvcNalParser::PrimaryPicTypeName(primary_pic_type);
            if (primary_pic_type_name == NULL) primary_pic_type_name = "UNKNOWN";
            DBG_PRINTF_2("[%d:%s]\n", primary_pic_type, primary_pic_type_name);

            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_NON_IDR_PICTURE ||
                   nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE     ||
                   nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A) {
            AP4_AvcSliceHeader* slice_header = new AP4_AvcSliceHeader;
            result = ParseSliceHeader(nal_unit+1,
                                      nal_unit_size-1,
                                      nal_unit_type,
                                      nal_ref_idc,
                                      *slice_header);
            if (AP4_FAILED(result)) {
                return AP4_ERROR_INVALID_FORMAT;
            }
            
            const char* slice_type_name = AP4_AvcNalParser::SliceTypeName(slice_header->slice_type);
            if (slice_type_name == NULL) slice_type_name = "?";
            DBG_PRINTF_5(" pps_id=%d, header_size=%d, frame_num=%d, slice_type=%d (%s), ",
                   slice_header->pic_parameter_set_id,
                   slice_header->size,
                   slice_header->frame_num,
                   slice_header->slice_type,
                   slice_type_name);
            DBG_PRINTF_0("\n");
            if (slice_header) {
                if (m_SliceHeader &&
                    !SameFrame(m_NalUnitType, m_NalRefIdc, *m_SliceHeader,
                               nal_unit_type, nal_ref_idc, *slice_header)) {
                    CheckIfAccessUnitIsCompleted(access_unit_info);
                    m_AccessUnitVclNalUnitCount = 1;
                } else {
                    // continuation of an access unit
                    ++m_AccessUnitVclNalUnitCount;
                }
            }

            // buffer this NAL unit
            AppendNalUnitData(nal_unit, nal_unit_size);
            delete m_SliceHeader;
            m_SliceHeader = slice_header;
            m_NalUnitType = nal_unit_type;
            m_NalRefIdc   = nal_ref_idc;
        } else if (nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_PPS) {
            AP4_AvcPictureParameterSet* pps = new AP4_AvcPictureParameterSet;
            result = ParsePPS(nal_unit, nal_unit_size, *pps);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_0("PPS ERROR!!!\n");
                delete pps;
            } else {
                delete m_PPS[pps->pic_parameter_set_id];
                m_PPS[pps->pic_parameter_set_id] = pps;
                DBG_PRINTF_2("PPS sps_id=%d, pps_id=%d\n", pps->seq_parameter_set_id, pps->pic_parameter_set_id);
                
                // keep the PPS with the NAL unit (this is optional)
                AppendNalUnitData(nal_unit, nal_unit_size);
                CheckIfAccessUnitIsCompleted(access_unit_info);
            }
        } else if (nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_SPS) {
            AP4_AvcSequenceParameterSet* sps = new AP4_AvcSequenceParameterSet;
            result = ParseSPS(nal_unit, nal_unit_size, *sps);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_0("SPS ERROR!!!\n");
                delete sps;
            } else {
                delete m_SPS[sps->seq_parameter_set_id];
                m_SPS[sps->seq_parameter_set_id] = sps;
                DBG_PRINTF_1("SPS sps_id=%d\n", sps->seq_parameter_set_id);
                CheckIfAccessUnitIsCompleted(access_unit_info);
            }
        } else if (nal_unit_type == AP4_AVC_NAL_UNIT_TYPE_SEI) {
            AppendNalUnitData(nal_unit, nal_unit_size);
            CheckIfAccessUnitIsCompleted(access_unit_info);
            DBG_PRINTF_0("\n");
        } else if (nal_unit_type >= 14 && nal_unit_type <= 18) {
            CheckIfAccessUnitIsCompleted(access_unit_info);
            DBG_PRINTF_0("\n");
        } else {
            DBG_PRINTF_0("\n");
        }
        m_TotalNalUnitCount++;
    }
    
    // flush if needed
    if (last_unit && access_unit_info.nal_units.ItemCount() == 0) {
        DBG_PRINTF_0("------ last unit\n");
        CheckIfAccessUnitIsCompleted(access_unit_info);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser::AccessUnitInfo::Reset
+---------------------------------------------------------------------*/
void
AP4_AvcFrameParser::AccessUnitInfo::Reset()
{
    for (unsigned int i=0; i<nal_units.ItemCount(); i++) {
        delete nal_units[i];
    }
    nal_units.Clear();
    is_idr = false;
    decode_order = 0;
    display_order = 0;
}
