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

#ifndef _AP4_HEVC_PARSER_H_
#define _AP4_HEVC_PARSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4Results.h"
#include "Ap4DataBuffer.h"
#include "Ap4NalParser.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_HEVC_NALU_TYPE_TRAIL_N        = 0;
const unsigned int AP4_HEVC_NALU_TYPE_TRAIL_R        = 1;
const unsigned int AP4_HEVC_NALU_TYPE_TSA_N          = 2;
const unsigned int AP4_HEVC_NALU_TYPE_TSA_R          = 3;
const unsigned int AP4_HEVC_NALU_TYPE_STSA_N         = 4;
const unsigned int AP4_HEVC_NALU_TYPE_STSA_R         = 5;
const unsigned int AP4_HEVC_NALU_TYPE_RADL_N         = 6;
const unsigned int AP4_HEVC_NALU_TYPE_RADL_R         = 7;
const unsigned int AP4_HEVC_NALU_TYPE_RASL_N         = 8;
const unsigned int AP4_HEVC_NALU_TYPE_RASL_R         = 9;
const unsigned int AP4_HEVC_NALU_TYPE_BLA_W_LP       = 16;
const unsigned int AP4_HEVC_NALU_TYPE_BLA_W_RADL     = 17;
const unsigned int AP4_HEVC_NALU_TYPE_BLA_N_LP       = 18;
const unsigned int AP4_HEVC_NALU_TYPE_IDR_W_RADL     = 19;
const unsigned int AP4_HEVC_NALU_TYPE_IDR_N_LP       = 20;
const unsigned int AP4_HEVC_NALU_TYPE_CRA_NUT        = 21;
const unsigned int AP4_HEVC_NALU_TYPE_VPS_NUT        = 32;
const unsigned int AP4_HEVC_NALU_TYPE_SPS_NUT        = 33;
const unsigned int AP4_HEVC_NALU_TYPE_PPS_NUT        = 34;
const unsigned int AP4_HEVC_NALU_TYPE_AUD_NUT        = 35;
const unsigned int AP4_HEVC_NALU_TYPE_EOS_NUT        = 36;
const unsigned int AP4_HEVC_NALU_TYPE_EOB_NUT        = 37;
const unsigned int AP4_HEVC_NALU_TYPE_FD_NUT         = 38;
const unsigned int AP4_HEVC_NALU_TYPE_PREFIX_SEI_NUT = 39;
const unsigned int AP4_HEVC_NALU_TYPE_SUFFIX_SEI_NUT = 40;

/*----------------------------------------------------------------------
|   AP4_HevcParser
+---------------------------------------------------------------------*/
class AP4_HevcParser : public AP4_NalParser {
public:
    static const char* NaluTypeName(unsigned int nalu_type);
    static const char* PicTypeName(unsigned int primary_pic_type);
    static const char* SliceTypeName(unsigned int slice_type);
    
    AP4_HevcParser();
};

#endif // _AP4_HEVC_PARSER_H_
