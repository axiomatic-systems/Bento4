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
|   AP4_AvcParser::NaluTypeName
+---------------------------------------------------------------------*/
const char*
AP4_AvcParser::NaluTypeName(unsigned int nalu_type)
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
|   AP4_AvcParser::PrimaryPicTypeName
+---------------------------------------------------------------------*/
const char*
AP4_AvcParser::PrimaryPicTypeName(unsigned int primary_pic_type)
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
|   AP4_AvcParser::SliceTypeName
+---------------------------------------------------------------------*/
const char*
AP4_AvcParser::SliceTypeName(unsigned int slice_type)
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
|   AP4_AvcParser::AP4_AvcParser
+---------------------------------------------------------------------*/
AP4_AvcParser::AP4_AvcParser() :
    AP4_NalParser()
{
}
