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
|   AP4_HevcParser::NaluTypeName
+---------------------------------------------------------------------*/
const char*
AP4_HevcParser::NaluTypeName(unsigned int nalu_type)
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
|   AP4_HevcParser::PicTypeName
+---------------------------------------------------------------------*/
const char*
AP4_HevcParser::PicTypeName(unsigned int primary_pic_type)
{
	switch (primary_pic_type) {
        case 0: return "I";
        case 1: return "I, P";
        case 2: return "I, P, B";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_HevcParser::SliceTypeName
+---------------------------------------------------------------------*/
const char*
AP4_HevcParser::SliceTypeName(unsigned int slice_type)
{
	switch (slice_type) {
        case 0: return "B";
        case 1: return "P";
        case 2: return "I";
        default: return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_AvcParser::AP4_AvcParser
+---------------------------------------------------------------------*/
AP4_HevcParser::AP4_HevcParser() :
    AP4_NalParser()
{
}
