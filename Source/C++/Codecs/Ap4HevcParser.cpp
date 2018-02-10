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
#define AP4_HEVC_PARSER_ENABLE_DEBUG 0

#if AP4_HEVC_PARSER_ENABLE_DEBUG
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
        case AP4_HEVC_SLICE_TYPE_B: return "B";
        case AP4_HEVC_SLICE_TYPE_P: return "P";
        case AP4_HEVC_SLICE_TYPE_I: return "I";
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
|   BitsNeeded
+---------------------------------------------------------------------*/
static unsigned int
BitsNeeded(unsigned int num_values)
{
    unsigned int bits_needed = 1;
    while (num_values > (unsigned int)(1 << bits_needed)) {
        ++bits_needed;
    }
    
    return bits_needed;
}

/*----------------------------------------------------------------------
|   AP4_HevcNalParser::AP4_HevcNalParser
+---------------------------------------------------------------------*/
AP4_HevcNalParser::AP4_HevcNalParser() :
    AP4_NalParser()
{
}

/*----------------------------------------------------------------------
|   scaling_list_data
+---------------------------------------------------------------------*/
static void
scaling_list_data(AP4_BitReader& bits)
{
    for (unsigned int sizeId = 0; sizeId < 4; sizeId++) {
        for (unsigned int matrixId = 0; matrixId < ((sizeId == 3)?2:6); matrixId++) {
            unsigned int flag = bits.ReadBit(); // scaling_list_pred_mode_flag[ sizeId ][ matrixId ]
            if (!flag) {
                ReadGolomb(bits); // scaling_list_pred_matrix_id_delta[ sizeId ][ matrixId ]
            } else {
                // nextCoef = 8;
                unsigned int coefNum = (1 << (4+(sizeId << 1)));
                if (coefNum > 64) coefNum = 64;
                if (sizeId > 1) {
                    ReadGolomb(bits); // scaling_list_dc_coef_minus8[ sizeId − 2 ][ matrixId ]
                    // nextCoef = scaling_list_dc_coef_minus8[ sizeId − 2 ][ matrixId ] + 8
                }
                for (unsigned i = 0; i < coefNum; i++) {
                    ReadGolomb(bits); // scaling_list_delta_coef
                    // nextCoef = ( nextCoef + scaling_list_delta_coef + 256 ) % 256
                    // ScalingList[ sizeId ][ matrixId ][ i ] = nextCoef
                }
            }
        }
    }
}

/*----------------------------------------------------------------------
|   st_ref_pic_set
+---------------------------------------------------------------------*/
static AP4_Result
parse_st_ref_pic_set(AP4_HevcShortTermRefPicSet*         rps,
                     const AP4_HevcSequenceParameterSet* sps,
                     unsigned int                        stRpsIdx,
                     unsigned int                        num_short_term_ref_pic_sets,
                     AP4_BitReader&                      bits) {
    AP4_SetMemory(rps, 0, sizeof(*rps));
    
    unsigned int inter_ref_pic_set_prediction_flag = 0;
    if (stRpsIdx != 0) {
        inter_ref_pic_set_prediction_flag = bits.ReadBit();
    }
    if (inter_ref_pic_set_prediction_flag) {
        unsigned int delta_idx_minus1 = 0;
        if (stRpsIdx == num_short_term_ref_pic_sets) {
            delta_idx_minus1 = ReadGolomb(bits);
        }
        /* delta_rps_sign = */ bits.ReadBit();
        /* abs_delta_rps_minus1 = */ ReadGolomb(bits);
        if (delta_idx_minus1+1 > stRpsIdx) return AP4_ERROR_INVALID_FORMAT; // should not happen
        unsigned int RefRpsIdx = stRpsIdx - (delta_idx_minus1 + 1);
        unsigned int NumDeltaPocs = sps->short_term_ref_pic_sets[RefRpsIdx].num_negative_pics + sps->short_term_ref_pic_sets[RefRpsIdx].num_positive_pics;
        for (unsigned j=0; j<=NumDeltaPocs; j++) {
            unsigned int used_by_curr_pic_flag /*[j]*/ = bits.ReadBit();
            if (!used_by_curr_pic_flag /*[j]*/) {
                /* use_delta_flag[j] = */ bits.ReadBit();
            }
        }
    } else {
        rps->num_negative_pics = ReadGolomb(bits);
        rps->num_positive_pics = ReadGolomb(bits);
        if (rps->num_negative_pics > 16 || rps->num_positive_pics > 16) {
            return AP4_ERROR_INVALID_FORMAT;
        }
        for (unsigned int i=0; i<rps->num_negative_pics; i++) {
            rps->delta_poc_s0_minus1[i] = ReadGolomb(bits);
            rps->used_by_curr_pic_s0_flag[i] = bits.ReadBit();
        }
        for (unsigned i=0; i<rps->num_positive_pics; i++) {
            rps->delta_poc_s1_minus1[i] = ReadGolomb(bits);
            rps->used_by_curr_pic_s1_flag[i] = bits.ReadBit();
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   NumPicTotalCurr
+---------------------------------------------------------------------*/
static unsigned int
NumPicTotalCurr(const AP4_HevcShortTermRefPicSet* rps,
                const AP4_HevcSliceSegmentHeader* slice_segment_header)
{
    // compute NumPicTotalCurr
    unsigned int nptc = 0;
    if (rps) {
        for (unsigned int i=0; i<rps->num_negative_pics; i++) {
            if (rps->used_by_curr_pic_s0_flag[i]) {
                ++nptc;
            }
        }
        for (unsigned int i=0; i<rps->num_positive_pics; i++) {
            if (rps->used_by_curr_pic_s1_flag[i]) {
                ++nptc;
            }
        }
    }
    for (unsigned int i=0; i<slice_segment_header->num_long_term_sps + slice_segment_header->num_long_term_pics; i++) {
        if (slice_segment_header->used_by_curr_pic_lt_flag[i]) {
            ++nptc;
        }
    }
    // TODO: for now we assume pps_curr_pic_ref_enabled is 0
    //if (pps_curr_pic_ref_enabled) {
    //    ++nptc;
    //}
    
    return nptc;
}

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
    
    // some fields default to 1
    pic_output_flag = 1;

    // start the parser
    AP4_DataBuffer unescaped(data, data_size);
    AP4_NalParser::Unescape(unescaped);
    AP4_BitReader bits(unescaped.GetData(), unescaped.GetDataSize());

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

    if (!first_slice_segment_in_pic_flag) {
        if (pps->dependent_slice_segments_enabled_flag) {
            dependent_slice_segment_flag = bits.ReadBit();
        }
        
        unsigned int bits_needed = BitsNeeded(PicSizeInCtbsY);
        if (bits_needed) {
            slice_segment_address = bits.ReadBits(bits_needed);
        }
    }
    
    if (!dependent_slice_segment_flag) {
        if (pps->num_extra_slice_header_bits) {
            bits.ReadBits(pps->num_extra_slice_header_bits); // slice_reserved_flag[...]
        }
    
        slice_type = ReadGolomb(bits);
        if (slice_type != AP4_HEVC_SLICE_TYPE_B && slice_type != AP4_HEVC_SLICE_TYPE_P && slice_type != AP4_HEVC_SLICE_TYPE_I) {
            return AP4_ERROR_INVALID_FORMAT;
        }
        if (pps->output_flag_present_flag) {
            pic_output_flag = bits.ReadBit();
        }
        if (sps->separate_colour_plane_flag) {
            colour_plane_id = bits.ReadBits(2);
        }
        unsigned int slice_sao_luma_flag = 0;
        unsigned int slice_sao_chroma_flag = 0;
        unsigned int slice_deblocking_filter_disabled_flag = 0;
        unsigned int slice_temporal_mvp_enabled_flag = 0;
        const AP4_HevcShortTermRefPicSet* rps = NULL;
        if (nal_unit_type != AP4_HEVC_NALU_TYPE_IDR_W_RADL && nal_unit_type != AP4_HEVC_NALU_TYPE_IDR_N_LP) {
            slice_pic_order_cnt_lsb = bits.ReadBits(sps->log2_max_pic_order_cnt_lsb_minus4+4);
            short_term_ref_pic_set_sps_flag = bits.ReadBit();
            if (!short_term_ref_pic_set_sps_flag) {
                AP4_Result result = parse_st_ref_pic_set(&short_term_ref_pic_set,
                                                         sps,
                                                         sps->num_short_term_ref_pic_sets,
                                                         sps->num_short_term_ref_pic_sets,
                                                         bits);
                if (AP4_FAILED(result)) return result;
                rps = &short_term_ref_pic_set;
            } else if (sps->num_short_term_ref_pic_sets > 1) {
                short_term_ref_pic_set_idx = bits.ReadBits(BitsNeeded(sps->num_short_term_ref_pic_sets));
                rps = &sps->short_term_ref_pic_sets[short_term_ref_pic_set_idx];
            }
            
            if (sps->long_term_ref_pics_present_flag) {
                if (sps->num_long_term_ref_pics_sps > 0) {
                    num_long_term_sps = ReadGolomb(bits);
                }
                num_long_term_pics = ReadGolomb(bits);
                
                if (num_long_term_sps > sps->num_long_term_ref_pics_sps) {
                    return AP4_ERROR_INVALID_FORMAT;
                }
                if (num_long_term_sps + num_long_term_pics > AP4_HEVC_MAX_LT_REFS) {
                    return AP4_ERROR_INVALID_FORMAT;
                }
                for (unsigned int i=0; i<num_long_term_sps + num_long_term_pics; i++) {
                    if (i < num_long_term_sps) {
                        if (sps->num_long_term_ref_pics_sps > 1) {
                            /* lt_idx_sps[i] = */ bits.ReadBits(BitsNeeded(sps->num_long_term_ref_pics_sps));
                        }
                    } else {
                        /* poc_lsb_lt[i] = */ bits.ReadBits(sps->log2_max_pic_order_cnt_lsb_minus4+4);
                        used_by_curr_pic_lt_flag[i] = bits.ReadBit();
                    }
                    unsigned int delta_poc_msb_present_flag /*[i]*/ = bits.ReadBit();
                    if (delta_poc_msb_present_flag /*[i]*/) {
                        /* delta_poc_msb_cycle_lt[i] = */ ReadGolomb(bits);
                    }
                }
            }
            if (sps->sps_temporal_mvp_enabled_flag) {
                slice_temporal_mvp_enabled_flag = bits.ReadBit();
            }
        }
        if (sps->sample_adaptive_offset_enabled_flag) {
            slice_sao_luma_flag   = bits.ReadBit();
            unsigned int ChromaArrayType = sps->separate_colour_plane_flag ? 0 : sps->chroma_format_idc;
            if (ChromaArrayType) {
                slice_sao_chroma_flag = bits.ReadBit();
            }
        }
        if (slice_type == AP4_HEVC_SLICE_TYPE_P || slice_type == AP4_HEVC_SLICE_TYPE_B) {
            unsigned int num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
            unsigned int num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
            unsigned int num_ref_idx_active_override_flag = bits.ReadBit();
            if (num_ref_idx_active_override_flag) {
                num_ref_idx_l0_active_minus1 = ReadGolomb(bits);
                if (slice_type == AP4_HEVC_SLICE_TYPE_B) {
                    num_ref_idx_l1_active_minus1 = ReadGolomb(bits);
                }
            }
            if (num_ref_idx_l0_active_minus1 > 14 || num_ref_idx_l1_active_minus1 > 14) {
                return AP4_ERROR_INVALID_FORMAT;
            }
            unsigned int nptc = NumPicTotalCurr(rps, this);
            if (pps->lists_modification_present_flag && nptc > 1) {
                // ref_pic_lists_modification
                unsigned int ref_pic_list_modification_flag_l0 = bits.ReadBit();
                if (ref_pic_list_modification_flag_l0) {
                    for (unsigned int i=0; i<=num_ref_idx_l0_active_minus1; i++) {
                        /* list_entry_l0[i]; */ bits.ReadBits(BitsNeeded(nptc));
                    }
                }
                if (slice_type == AP4_HEVC_SLICE_TYPE_B) {
                    unsigned int ref_pic_list_modification_flag_l1 = bits.ReadBit();
                    if (ref_pic_list_modification_flag_l1) {
                        for (unsigned int i=0; i<=num_ref_idx_l1_active_minus1; i++) {
                            /* list_entry_l1[i]; */ bits.ReadBits(BitsNeeded(nptc));
                        }
                    }
                }
            }
            if (slice_type == AP4_HEVC_SLICE_TYPE_B) {
                /* mvd_l1_zero_flag = */ bits.ReadBit();
            }
            if (pps->cabac_init_present_flag) {
                /* cabac_init_flag = */ bits.ReadBit();
            }
            if (slice_temporal_mvp_enabled_flag) {
                unsigned int collocated_from_l0_flag = 1;
                if (slice_type == AP4_HEVC_SLICE_TYPE_B) {
                    collocated_from_l0_flag = bits.ReadBit();
                }
                if (( collocated_from_l0_flag && num_ref_idx_l0_active_minus1 > 0) ||
                    (!collocated_from_l0_flag && num_ref_idx_l1_active_minus1 > 0)) {
                    /* collocated_ref_idx = */ ReadGolomb(bits);
                }
            }
            if ((pps->weighted_pred_flag   && slice_type == AP4_HEVC_SLICE_TYPE_P) ||
                (pps->weighted_bipred_flag && slice_type == AP4_HEVC_SLICE_TYPE_B)) {
                // +++ pred_weight_table()
                /* luma_log2_weight_denom = */ ReadGolomb(bits);
                if (sps->chroma_format_idc != 0) {
                    /* delta_chroma_log2_weight_denom = */ /* SignedGolomb( */ ReadGolomb(bits) /*)*/;
                }
                unsigned int luma_weight_l0_flag[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                for (unsigned int i=0; i<=num_ref_idx_l0_active_minus1; i++) {
                    luma_weight_l0_flag[i] = bits.ReadBit();
                }
                unsigned int chroma_weight_l0_flag[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                if (sps->chroma_format_idc != 0) {
                    for (unsigned int i=0; i<=num_ref_idx_l0_active_minus1; i++) {
                        chroma_weight_l0_flag[i] = bits.ReadBit();
                    }
                }
                for (unsigned int i=0; i<=num_ref_idx_l0_active_minus1; i++) {
                    if (luma_weight_l0_flag[i]) {
                        /* delta_luma_weight_l0[i] = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                        /* luma_offset_l0[i] = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                    }
                    if (chroma_weight_l0_flag[i]) {
                        for (unsigned int j=0; j<2; j++) {
                            /* delta_chroma_weight_l0[i][j] = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                            /* delta_chroma_offset_l0[i][j] = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                        }
                    }
                }
                if (slice_type == AP4_HEVC_SLICE_TYPE_B) {
                    unsigned int luma_weight_l1_flag[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                    for (unsigned int i=0; i<=num_ref_idx_l1_active_minus1; i++) {
                        luma_weight_l1_flag[i] = bits.ReadBit();
                    }
                    unsigned int chroma_weight_l1_flag[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                    if (sps->chroma_format_idc != 0) {
                        for (unsigned int i=0; i<=num_ref_idx_l1_active_minus1; i++) {
                            chroma_weight_l1_flag[i] = bits.ReadBit();
                        }
                    }
                    for (unsigned int i=0; i<=num_ref_idx_l1_active_minus1; i++) {
                        if (luma_weight_l1_flag[i]) {
                            /* delta_luma_weight_l1[i] = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                            /* luma_offset_l1[i] = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                        }
                        if (chroma_weight_l1_flag[i]) {
                            for (unsigned int j=0; j<2; j++) {
                                /* delta_chroma_weight_l1[i][j] = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                                /* delta_chroma_offset_l1[i][j] = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                            }
                        }
                    }
                }
                // --- pred_weight_table()
            }
            /* five_minus_max_num_merge_cand = */ ReadGolomb(bits);
        }
        /* slice_qp_delta = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
        if (pps->pps_slice_chroma_qp_offsets_present_flag) {
            /* slice_cb_qp_offset = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
            /* slice_cr_qp_offset = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
        }
        unsigned int deblocking_filter_override_flag = 0;
        if (pps->deblocking_filter_override_enabled_flag) {
            deblocking_filter_override_flag = bits.ReadBit();
        }
        if (deblocking_filter_override_flag) {
            slice_deblocking_filter_disabled_flag = bits.ReadBit();
            if (!slice_deblocking_filter_disabled_flag) {
                /* slice_beta_offset_div2 = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
                /* slice_tc_offset_div2   = */ /*SignedGolomb(*/ ReadGolomb(bits) /*)*/;
            }
        }
        if (pps->pps_loop_filter_across_slices_enabled_flag &&
            (slice_sao_luma_flag || slice_sao_chroma_flag || !slice_deblocking_filter_disabled_flag)) {
            /* slice_loop_filter_across_slices_enabled_flag = */ bits.ReadBit();
        }
    }

    if (pps->tiles_enabled_flag || pps->entropy_coding_sync_enabled_flag) {
        num_entry_point_offsets = ReadGolomb(bits);
        if (num_entry_point_offsets > 0 ) {
            offset_len_minus1 = ReadGolomb(bits);
            if (offset_len_minus1 > 31) {
                return AP4_ERROR_INVALID_FORMAT;
            }
            for (unsigned int i=0; i<num_entry_point_offsets; i++) {
                bits.ReadBits(offset_len_minus1+1);
            }
        }
    }

    if (pps->slice_segment_header_extension_present_flag) {
        unsigned int slice_segment_header_extension_length = ReadGolomb(bits);
        for (unsigned int i=0; i<slice_segment_header_extension_length; i++) {
            bits.ReadBits(8); // slice_segment_header_extension_data_byte[i]
        }
    }

    // byte_alignment()
    bits.ReadBit(); // alignment_bit_equal_to_one
    unsigned int bits_read = bits.GetBitsRead();
    if (bits_read % 8) {
        bits.ReadBits(8-(bits_read%8));
    }
    
    /* compute the size */
    size = bits.GetBitsRead();
    DBG_PRINTF_2("*** slice segment header size=%d bits (%d bytes)\n", size, size/8);

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
    num_extra_slice_header_bits(0),
    sign_data_hiding_enabled_flag(0),
    cabac_init_present_flag(0),
    num_ref_idx_l0_default_active_minus1(0),
    num_ref_idx_l1_default_active_minus1(0),
    init_qp_minus26(0),
    constrained_intra_pred_flag(0),
    transform_skip_enabled_flag(0),
    cu_qp_delta_enabled_flag(0),
    diff_cu_qp_delta_depth(0),
    pps_cb_qp_offset(0),
    pps_cr_qp_offset(0),
    pps_slice_chroma_qp_offsets_present_flag(0),
    weighted_pred_flag(0),
    weighted_bipred_flag(0),
    transquant_bypass_enabled_flag(0),
    tiles_enabled_flag(0),
    entropy_coding_sync_enabled_flag(0),
    num_tile_columns_minus1(0),
    num_tile_rows_minus1(0),
    uniform_spacing_flag(1),
    loop_filter_across_tiles_enabled_flag(1),
    pps_loop_filter_across_slices_enabled_flag(0),
    deblocking_filter_control_present_flag(0),
    deblocking_filter_override_enabled_flag(0),
    pps_deblocking_filter_disabled_flag(0),
    pps_beta_offset_div2(0),
    pps_tc_offset_div2(0),
    pps_scaling_list_data_present_flag(0),
    lists_modification_present_flag(0),
    log2_parallel_merge_level_minus2(0),
    slice_segment_header_extension_present_flag(0)
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
    dependent_slice_segments_enabled_flag    = bits.ReadBit();
    output_flag_present_flag                 = bits.ReadBit();
    num_extra_slice_header_bits              = bits.ReadBits(3);
    sign_data_hiding_enabled_flag            = bits.ReadBit();
    cabac_init_present_flag                  = bits.ReadBit();
    num_ref_idx_l0_default_active_minus1     = ReadGolomb(bits);
    num_ref_idx_l1_default_active_minus1     = ReadGolomb(bits);
    init_qp_minus26                          = SignedGolomb(ReadGolomb(bits));
    constrained_intra_pred_flag              = bits.ReadBit();
    transform_skip_enabled_flag              = bits.ReadBit();
    cu_qp_delta_enabled_flag                 = bits.ReadBit();
    if (cu_qp_delta_enabled_flag) {
        diff_cu_qp_delta_depth = ReadGolomb(bits);
    }
    pps_cb_qp_offset                         = SignedGolomb(ReadGolomb(bits));
    pps_cr_qp_offset                         = SignedGolomb(ReadGolomb(bits));
    pps_slice_chroma_qp_offsets_present_flag = bits.ReadBit();
    weighted_pred_flag                       = bits.ReadBit();
    weighted_bipred_flag                     = bits.ReadBit();
    transquant_bypass_enabled_flag           = bits.ReadBit();
    tiles_enabled_flag                       = bits.ReadBit();
    entropy_coding_sync_enabled_flag         = bits.ReadBit();
    if (tiles_enabled_flag) {
        num_tile_columns_minus1 = ReadGolomb(bits);
        num_tile_rows_minus1    = ReadGolomb(bits);
        uniform_spacing_flag    = bits.ReadBit();
        if (!uniform_spacing_flag) {
            for (unsigned int i=0; i<num_tile_columns_minus1; i++) {
                ReadGolomb(bits); // column_width_minus1[i]
            }
            for (unsigned int i = 0; i < num_tile_rows_minus1; i++) {
                ReadGolomb(bits); // row_height_minus1[i]
            }
        }
        loop_filter_across_tiles_enabled_flag = bits.ReadBit();
    }
    pps_loop_filter_across_slices_enabled_flag = bits.ReadBit();
    deblocking_filter_control_present_flag     = bits.ReadBit();
    if (deblocking_filter_control_present_flag) {
        deblocking_filter_override_enabled_flag = bits.ReadBit();
        pps_deblocking_filter_disabled_flag     = bits.ReadBit();
        if (!pps_deblocking_filter_disabled_flag) {
            pps_beta_offset_div2 = SignedGolomb(ReadGolomb(bits));
            pps_tc_offset_div2   = SignedGolomb(ReadGolomb(bits));
        }
    }
    pps_scaling_list_data_present_flag = bits.ReadBit();
    if (pps_scaling_list_data_present_flag) {
        scaling_list_data(bits);
    }
    lists_modification_present_flag = bits.ReadBit();
    log2_parallel_merge_level_minus2 = ReadGolomb(bits);
    slice_segment_header_extension_present_flag = bits.ReadBit();
    
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
    num_short_term_ref_pic_sets(0),
    long_term_ref_pics_present_flag(0),
    num_long_term_ref_pics_sps(0),
    sps_temporal_mvp_enabled_flag(0),
    strong_intra_smoothing_enabled_flag(0)
{
    AP4_SetMemory(&profile_tier_level, 0, sizeof(profile_tier_level));
    for (unsigned int i=0; i<8; i++) {
        sps_max_dec_pic_buffering_minus1[i] = 0;
        sps_max_num_reorder_pics[i]         = 0;
        sps_max_latency_increase_plus1[i]   = 0;
    }
    AP4_SetMemory(short_term_ref_pic_sets, 0, sizeof(short_term_ref_pic_sets));
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
    scaling_list_enabled_flag                = bits.ReadBit();
    if (scaling_list_enabled_flag) {
        sps_scaling_list_data_present_flag = bits.ReadBit();
        if (sps_scaling_list_data_present_flag) {
            scaling_list_data(bits);
        }
    }
    amp_enabled_flag = bits.ReadBit();
    sample_adaptive_offset_enabled_flag = bits.ReadBit();
    pcm_enabled_flag = bits.ReadBit();
    if (pcm_enabled_flag) {
        pcm_sample_bit_depth_luma_minus1 = bits.ReadBits(4);
        pcm_sample_bit_depth_chroma_minus1 = bits.ReadBits(4);
        log2_min_pcm_luma_coding_block_size_minus3 = ReadGolomb(bits);
        log2_diff_max_min_pcm_luma_coding_block_size = ReadGolomb(bits);
        pcm_loop_filter_disabled_flag = bits.ReadBit();
    }
    num_short_term_ref_pic_sets = ReadGolomb(bits);
    if (num_short_term_ref_pic_sets > AP4_HEVC_SPS_MAX_RPS) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    for (unsigned int i=0; i<num_short_term_ref_pic_sets; i++) {
        AP4_Result result = parse_st_ref_pic_set(&short_term_ref_pic_sets[i], this, i, num_short_term_ref_pic_sets, bits);
        if (AP4_FAILED(result)) return result;
    }
    long_term_ref_pics_present_flag = bits.ReadBit();
    if (long_term_ref_pics_present_flag) {
        num_long_term_ref_pics_sps = ReadGolomb(bits);
        for (unsigned int i=0; i<num_long_term_ref_pics_sps; i++) {
            /* lt_ref_pic_poc_lsb_sps[i] = */ bits.ReadBits(log2_max_pic_order_cnt_lsb_minus4 + 4);
            /* used_by_curr_pic_lt_sps_flag[i] = */ bits.ReadBit();
        }
    }
    sps_temporal_mvp_enabled_flag  = bits.ReadBit();
    strong_intra_smoothing_enabled_flag = bits.ReadBit();

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
|   AP4_HevcFrameParser::Feed
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcFrameParser::Feed(const AP4_UI08* nal_unit,
                          AP4_Size        nal_unit_size,
                          AccessUnitInfo& access_unit_info,
                          bool            last_unit)
{
    AP4_Result result;

    // default return values
    access_unit_info.Reset();
    
    if (nal_unit && nal_unit_size >= 2) {
        unsigned int nal_unit_type    = (nal_unit[0] >> 1) & 0x3F;
        unsigned int nuh_layer_id     = (((nal_unit[0] & 1) << 5) | (nal_unit[1] >> 3));
        unsigned int nuh_temporal_id  = nal_unit[1] & 0x7;
        (void)nuh_layer_id;
        
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
            result = slice_header->Parse(nal_unit+2, nal_unit_size-2, nal_unit_type, &m_PPS[0], &m_SPS[0]);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_1("VCL parsing failed (%d)", result);
                return AP4_ERROR_INVALID_FORMAT;
            }
            
#if defined(AP4_HEVC_PARSER_ENABLE_DEBUG)
            const char* slice_type_name = AP4_HevcNalParser::SliceTypeName(slice_header->slice_type);
            if (slice_type_name == NULL) slice_type_name = "?";
            DBG_PRINTF_5(" pps_id=%d, first=%s, slice_type=%d (%s), size=%d, ",
                slice_header->slice_pic_parameter_set_id,
                slice_header->first_slice_segment_in_pic_flag?"YES":"NO",
                slice_header->slice_type,
                slice_type_name,
                slice_header->size);
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
            AppendNalUnitData(nal_unit, nal_unit_size);
            ++m_VclNalUnitsInAccessUnit;
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_AUD_NUT) {
            unsigned int pic_type = (nal_unit[1]>>5);
            const char*  pic_type_name = AP4_HevcNalParser::PicTypeName(pic_type);
            if (pic_type_name == NULL) pic_type_name = "UNKNOWN";
            DBG_PRINTF_2("[%d:%s]\n", pic_type, pic_type_name);

            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_PPS_NUT) {
            AP4_HevcPictureParameterSet* pps = new AP4_HevcPictureParameterSet;
            result = pps->Parse(nal_unit, nal_unit_size);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_0("PPS ERROR!!!");
                delete pps;
                return AP4_ERROR_INVALID_FORMAT;
            }
            delete m_PPS[pps->pps_pic_parameter_set_id];
            m_PPS[pps->pps_pic_parameter_set_id] = pps;
            DBG_PRINTF_2("PPS pps_id=%d, sps_id=%d", pps->pps_pic_parameter_set_id, pps->pps_seq_parameter_set_id);
            
            // keep the PPS with the NAL unit (this is optional)
            AppendNalUnitData(nal_unit, nal_unit_size);
            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_SPS_NUT) {
            AP4_HevcSequenceParameterSet* sps = new AP4_HevcSequenceParameterSet;
            result = sps->Parse(nal_unit, nal_unit_size);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_0("SPS ERROR!!!\n");
                delete sps;
                return AP4_ERROR_INVALID_FORMAT;
            }
            delete m_SPS[sps->sps_seq_parameter_set_id];
            m_SPS[sps->sps_seq_parameter_set_id] = sps;
            DBG_PRINTF_2("SPS sps_id=%d, vps_id=%d", sps->sps_seq_parameter_set_id, sps->sps_video_parameter_set_id);
            
            // keep the SPS with the NAL unit (this is optional)
            AppendNalUnitData(nal_unit, nal_unit_size);
            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_VPS_NUT) {
            AP4_HevcVideoParameterSet* vps = new AP4_HevcVideoParameterSet;
            result = vps->Parse(nal_unit, nal_unit_size);
            if (AP4_FAILED(result)) {
                DBG_PRINTF_0("VPS ERROR!!!\n");
                delete vps;
                return AP4_ERROR_INVALID_FORMAT;
            }
            delete m_VPS[vps->vps_video_parameter_set_id];
            m_VPS[vps->vps_video_parameter_set_id] = vps;
            DBG_PRINTF_1("VPS vps_id=%d", vps->vps_video_parameter_set_id);
            
            // keep the VPS with the NAL unit (this is optional)
            AppendNalUnitData(nal_unit, nal_unit_size);
            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_EOS_NUT ||
                   nal_unit_type == AP4_HEVC_NALU_TYPE_EOB_NUT) {
            CheckIfAccessUnitIsCompleted(access_unit_info);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_PREFIX_SEI_NUT) {
            CheckIfAccessUnitIsCompleted(access_unit_info);
            AppendNalUnitData(nal_unit, nal_unit_size);
        } else if (nal_unit_type == AP4_HEVC_NALU_TYPE_SUFFIX_SEI_NUT){
            AppendNalUnitData(nal_unit, nal_unit_size);
        }
        DBG_PRINTF_0("\n");
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
|   AP4_HevFrameParser::AccessUnitInfo::Reset
+---------------------------------------------------------------------*/
AP4_Result
AP4_HevcFrameParser::ParseSliceSegmentHeader(const AP4_UI08*             data,
                                             unsigned int                data_size,
                                             unsigned int                nal_unit_type,
                                             AP4_HevcSliceSegmentHeader& slice_header)
{
    return slice_header.Parse(data, data_size, nal_unit_type, &m_PPS[0], &m_SPS[0]);
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
