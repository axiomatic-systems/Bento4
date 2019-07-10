/*****************************************************************
|
|    AP4 - AVC Parser
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

#ifndef _AP4_AVC_PARSER_H_
#define _AP4_AVC_PARSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4Results.h"
#include "Ap4DataBuffer.h"
#include "Ap4NalParser.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_AVC_NAL_UNIT_TYPE_UNSPECIFIED                       = 0;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_NON_IDR_PICTURE    = 1;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A      = 2;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B      = 3;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C      = 4;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE        = 5;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_SEI                               = 6;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_SPS                               = 7;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_PPS                               = 8;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_ACCESS_UNIT_DELIMITER             = 9;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_END_OF_SEQUENCE                   = 10;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_END_OF_STREAM                     = 11;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_FILLER_DATA                       = 12;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_SPS_EXTENSION                     = 13;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_PREFIX                            = 14;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_SUBSET_SPS                        = 15;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_AUXILIARY_PICTURE  = 19;
const unsigned int AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_IN_SCALABLE_EXTENSION = 20;

const unsigned int AP4_AVC_SLICE_TYPE_P                                    = 0;
const unsigned int AP4_AVC_SLICE_TYPE_B                                    = 1;
const unsigned int AP4_AVC_SLICE_TYPE_I                                    = 2;
const unsigned int AP4_AVC_SLICE_TYPE_SP                                   = 3;
const unsigned int AP4_AVC_SLICE_TYPE_SI                                   = 4;

const unsigned int AP4_AVC_SPS_MAX_ID                                      = 255;
const unsigned int AP4_AVC_SPS_MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE   = 256;
const unsigned int AP4_AVC_SPS_MAX_SCALING_LIST_COUNT                      = 12;

const unsigned int AP4_AVC_PPS_MAX_ID                                      = 255;
const unsigned int AP4_AVC_PPS_MAX_SLICE_GROUPS                            = 256;
const unsigned int AP4_AVC_PPS_MAX_PIC_SIZE_IN_MAP_UNITS                   = 65536;

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    int scale[16];
} AP4_AvcScalingList4x4;

typedef struct {
    int scale[64];
} AP4_AvcScalingList8x8;

struct AP4_AvcSequenceParameterSet {
    AP4_AvcSequenceParameterSet();
    
    void GetInfo(unsigned int& width, unsigned int& height);
    
    AP4_DataBuffer raw_bytes;

    unsigned int profile_idc;
    unsigned int constraint_set0_flag;
    unsigned int constraint_set1_flag;
    unsigned int constraint_set2_flag;
    unsigned int constraint_set3_flag;
    unsigned int level_idc;
    unsigned int seq_parameter_set_id;
    unsigned int chroma_format_idc;
    unsigned int separate_colour_plane_flag;
    unsigned int bit_depth_luma_minus8;
    unsigned int bit_depth_chroma_minus8;
    unsigned int qpprime_y_zero_transform_bypass_flag;
    unsigned int seq_scaling_matrix_present_flag;
    AP4_AvcScalingList4x4 scaling_list_4x4[6];
    bool                  use_default_scaling_matrix_4x4[AP4_AVC_SPS_MAX_SCALING_LIST_COUNT];
    AP4_AvcScalingList8x8 scaling_list_8x8[6];
    unsigned char         use_default_scaling_matrix_8x8[AP4_AVC_SPS_MAX_SCALING_LIST_COUNT];
    unsigned int log2_max_frame_num_minus4;
    unsigned int pic_order_cnt_type;
    unsigned int log2_max_pic_order_cnt_lsb_minus4;
    unsigned int delta_pic_order_always_zero_flags;
    int          offset_for_non_ref_pic;
    int          offset_for_top_to_bottom_field;
    unsigned int num_ref_frames_in_pic_order_cnt_cycle;
    unsigned int offset_for_ref_frame[AP4_AVC_SPS_MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE];
    unsigned int num_ref_frames;
    unsigned int gaps_in_frame_num_value_allowed_flag;
    unsigned int pic_width_in_mbs_minus1;
    unsigned int pic_height_in_map_units_minus1;
    unsigned int frame_mbs_only_flag;
    unsigned int mb_adaptive_frame_field_flag;
    unsigned int direct_8x8_inference_flag;
    unsigned int frame_cropping_flag;
    unsigned int frame_crop_left_offset;
    unsigned int frame_crop_right_offset;
    unsigned int frame_crop_top_offset;
    unsigned int frame_crop_bottom_offset;
};

struct AP4_AvcPictureParameterSet {
    AP4_AvcPictureParameterSet();
    
    AP4_DataBuffer raw_bytes;
    
    unsigned int pic_parameter_set_id;
    unsigned int seq_parameter_set_id;
    unsigned int entropy_coding_mode_flag;
    unsigned int pic_order_present_flag;
    unsigned int num_slice_groups_minus1;
    unsigned int slice_group_map_type;
    unsigned int run_length_minus1[AP4_AVC_PPS_MAX_SLICE_GROUPS];
    unsigned int top_left[AP4_AVC_PPS_MAX_SLICE_GROUPS];
    unsigned int bottom_right[AP4_AVC_PPS_MAX_SLICE_GROUPS];
    unsigned int slice_group_change_direction_flag;
    unsigned int slice_group_change_rate_minus1;
    unsigned int pic_size_in_map_units_minus1;
    unsigned int num_ref_idx_10_active_minus1;
    unsigned int num_ref_idx_11_active_minus1;
    unsigned int weighted_pred_flag;
    unsigned int weighted_bipred_idc;
    int          pic_init_qp_minus26;
    int          pic_init_qs_minus26;
    int          chroma_qp_index_offset;
    unsigned int deblocking_filter_control_present_flag;
    unsigned int constrained_intra_pred_flag;
    unsigned int redundant_pic_cnt_present_flag;
};

struct AP4_AvcSliceHeader {
    AP4_AvcSliceHeader();
    
    unsigned int size; // not from the bitstream, this is computed after parsing
    
    unsigned int first_mb_in_slice;
    unsigned int slice_type;
    unsigned int pic_parameter_set_id;
    unsigned int colour_plane_id;
    unsigned int frame_num;
    unsigned int field_pic_flag;
    unsigned int bottom_field_flag;
    unsigned int idr_pic_id;
    unsigned int pic_order_cnt_lsb;
    int          delta_pic_order_cnt[2];
    unsigned int redundant_pic_cnt;
    unsigned int direct_spatial_mv_pred_flag;
    unsigned int num_ref_idx_active_override_flag;
    unsigned int num_ref_idx_l0_active_minus1;
    unsigned int num_ref_idx_l1_active_minus1;
    unsigned int ref_pic_list_reordering_flag_l0;
    unsigned int reordering_of_pic_nums_idc;
    unsigned int abs_diff_pic_num_minus1;
    unsigned int long_term_pic_num;
    unsigned int ref_pic_list_reordering_flag_l1;
    unsigned int luma_log2_weight_denom;
    unsigned int chroma_log2_weight_denom;
    unsigned int cabac_init_idc;
    unsigned int slice_qp_delta;
    unsigned int sp_for_switch_flag;
    int          slice_qs_delta;
    unsigned int disable_deblocking_filter_idc;
    int          slice_alpha_c0_offset_div2;
    int          slice_beta_offset_div2;
    unsigned int slice_group_change_cycle;
    unsigned int no_output_of_prior_pics_flag;
    unsigned int long_term_reference_flag;
    unsigned int difference_of_pic_nums_minus1;
    unsigned int long_term_frame_idx;
    unsigned int max_long_term_frame_idx_plus1;
};

/*----------------------------------------------------------------------
|   AP4_AvcNalParser
+---------------------------------------------------------------------*/
class AP4_AvcNalParser : public AP4_NalParser {
public:
    static const char* NaluTypeName(unsigned int nalu_type);
    static const char* PrimaryPicTypeName(unsigned int primary_pic_type);
    static const char* SliceTypeName(unsigned int slice_type);
    
    AP4_AvcNalParser();
};

/*----------------------------------------------------------------------
|   AP4_AvcFrameParser
+---------------------------------------------------------------------*/
class AP4_AvcFrameParser {
public:
    // types
    struct AccessUnitInfo {
        AP4_Array<AP4_DataBuffer*> nal_units;
        bool                       is_idr;
        AP4_UI32                   decode_order;
        AP4_UI32                   display_order;
        
        void Reset();
    };
    
    // methods
    AP4_AvcFrameParser();
   ~AP4_AvcFrameParser();
    
    /**
     * Feed some data to the parser and look for the next NAL Unit.
     *
     * @param data Pointer to the memory buffer with the data to feed.
     * @param data_size Size in bytes of the buffer pointed to by the
     * data pointer.
     * @param bytes_consumed Number of bytes from the data buffer that were
     * consumed and stored by the parser.
     * @param access_unit_info Reference to a AccessUnitInfo structure that will
     * contain information about any access unit found in the data. If no
     * access unit was found, the nal_units field of this structure will be an
     * empty array.
     * @param eos Boolean flag that indicates if this buffer is the last
     * buffer in the stream/file (End Of Stream).
     *
     * @result: AP4_SUCCESS is the call succeeds, or an error code if it
     * fails.
     * 
     * The caller must not feed the same data twice. When this method
     * returns, the caller should inspect the value of bytes_consumed and
     * advance the input stream source accordingly, such that the next
     * buffer passed to this method will be exactly bytes_consumed bytes
     * after what was passed in this call. After all the input data has
     * been supplied to the parser (eos=true), the caller also should
     * this method with an empty buffer (data=NULL and/or data_size=0) until
     * no more data is returned, because there may be buffered data still
     * available.
     *
     * When data is returned in the access_unit_info structure, the caller is
     * responsible for freeing this data by calling AccessUnitInfo::Reset()
     */
    AP4_Result Feed(const void*     data,
                    AP4_Size        data_size,
                    AP4_Size&       bytes_consumed,
                    AccessUnitInfo& access_unit_info,
                    bool            eos=false);
    
    AP4_Result Feed(const AP4_UI08* nal_unit,
                    AP4_Size        nal_unit_size,
                    AccessUnitInfo& access_unit_info,
                    bool            last_unit=false);

    AP4_AvcSequenceParameterSet** GetSequenceParameterSets() { return &m_SPS[0];     }
    AP4_AvcPictureParameterSet**  GetPictureParameterSets()  { return &m_PPS[0];     }
    const AP4_AvcSliceHeader*     GetSliceHeader()           { return m_SliceHeader; }
    
    AP4_Result ParseSPS(const unsigned char*         data,
                        unsigned int                 data_size,
                        AP4_AvcSequenceParameterSet& sps);
    AP4_Result ParsePPS(const unsigned char*        data,
                        unsigned int                data_size,
                        AP4_AvcPictureParameterSet& pps);
    AP4_Result ParseSliceHeader(const AP4_UI08*               data,
                                unsigned int                  data_size,
                                unsigned int                  nal_unit_type,
                                unsigned int                  nal_ref_idc,
                                AP4_AvcSliceHeader&           slice_header);

private:
    // methods
    bool SameFrame(unsigned int nal_unit_type_1, unsigned int nal_ref_idc_1, AP4_AvcSliceHeader& sh1,
                   unsigned int nal_unit_type_2, unsigned int nal_ref_idc_2, AP4_AvcSliceHeader& sh2);
    AP4_AvcSequenceParameterSet* GetSliceSPS(AP4_AvcSliceHeader& sh);
    void                         CheckIfAccessUnitIsCompleted(AccessUnitInfo& access_unit_info);
    void                         AppendNalUnitData(const unsigned char* data, unsigned int data_size);
    
    // members
    AP4_AvcNalParser             m_NalParser;
    AP4_AvcSequenceParameterSet* m_SPS[AP4_AVC_SPS_MAX_ID+1];
    AP4_AvcPictureParameterSet*  m_PPS[AP4_AVC_PPS_MAX_ID+1];

    // only updated on new VLC NAL Units
    unsigned int                 m_NalUnitType;
    unsigned int                 m_NalRefIdc;
    AP4_AvcSliceHeader*          m_SliceHeader;
    unsigned int                 m_AccessUnitVclNalUnitCount;
    
    // accumulator for NAL unit data
    unsigned int                 m_TotalNalUnitCount;
    unsigned int                 m_TotalAccessUnitCount;
    AP4_Array<AP4_DataBuffer*>   m_AccessUnitData;
    
    // used to keep track of picture order count
    unsigned int                 m_PrevFrameNum;
    unsigned int                 m_PrevFrameNumOffset;
    int                          m_PrevPicOrderCntMsb;
    unsigned int                 m_PrevPicOrderCntLsb;
};

#endif // _AP4_AVC_PARSER_H_
