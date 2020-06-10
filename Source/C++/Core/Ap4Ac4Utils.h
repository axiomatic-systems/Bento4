/*****************************************************************
|
|    AP4 - AC-4 Utilities
|
|    Copyright 2002-2020 Axiomatic Systems, LLC
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

#ifndef _AP4_AC4_UTILS_H_
#define _AP4_AC4_UTILS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_CH_MODE_LENGTH           = 16;   /* AC-4 ch_mode length  */
const unsigned int AP4_AC4_FRAME_RATE_NUM       = 14;   /* AC-4 frame rate number  */
const unsigned int AP4_AC4_SUBSTREAM_GROUP_NUM  = 3;    /* AC-4 Maximum substream group if presentation_config < 5 */

const unsigned char 
SUPER_SET_CH_MODE[AP4_CH_MODE_LENGTH][AP4_CH_MODE_LENGTH] =
{
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
    {1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
    {2, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
    {3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
    {4, 4, 4, 4, 4, 6, 6, 8, 8, 10,10,12,12,14,14,15},
    {5, 5, 5, 5, 6, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
    {6, 6, 6, 6, 6, 6, 6, 6, 8, 6, 10,12,12,14,14,15},
    {7, 7, 7, 7, 8, 7, 6, 7, 8, 9, 10,12,12,13,14,15},
    {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 10,11,12,14,14,15},
    {9, 9, 9, 9, 10,9, 10,9, 9, 9, 10,11,12,13,14,15},
    {10,10,10,10,10,10,10,10,10,10,10,10,12,13,14,15},
    {11,11,11,11,12,11,12,11,12,11,12,11,13,13,14,15},
    {12,12,12,12,12,12,12,12,12,12,12,12,12,13,14,15},
    {13,13,13,13,14,13,14,13,14,13,14,13,14,13,14,15},
    {14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15},
    {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
};

// ch_mode - TS 103 190-2 table 78 
const int CH_MODE_MONO      = 0;
const int CH_MODE_STEREO    = 1;
const int CH_MODE_3_0       = 2;
const int CH_MODE_5_0       = 3;
const int CH_MODE_5_1       = 4;
const int CH_MODE_70_34     = 5;
const int CH_MODE_71_34     = 6;
const int CH_MODE_70_52     = 7;
const int CH_MODE_71_52     = 8;
const int CH_MODE_70_322    = 9;
const int CH_MODE_71_322    = 10;
const int CH_MODE_7_0_4     = 11;
const int CH_MODE_7_1_4     = 12;
const int CH_MODE_9_0_4     = 13;
const int CH_MODE_9_1_4     = 14;
const int CH_MODE_22_2      = 15;
const int CH_MODE_RESERVED  = 16;

// speaker group index mask, indexed by ch_mode - TS 103 190-2 A.27 
const int 
AC4_SPEAKER_GROUP_INDEX_MASK_BY_CH_MODE[] =
{
    2,        // 0b10 - 1.0
    1,        // 0b01 - 2.0
    3,        // 0b11 - 3.0
    7,        // 0b0000111 - 5.0
    71,       // 0b1000111 - 5.1
    15,       // 0b0001111 - 7.0: 3/4/0
    79,       // 0b1001111 - 7.1: 3/4/0.1
    131079,   // 0b100000000000000111 - 7.0: 5/2/0
    131143,   // 0b100000000001000111 - 7.1: 5/2/0.1
    262151,   // 0b1000000000000000111 - 7.0: 3/2/2
    262215,   // 0b1000000000001000111 - 7.1: 3/2/2.1
    63,       // 0b0111111 - 7.0.4
    127,      // 0b1111111 - 7.1.4
    65599,    // 0b10000000000111111 - 9.0.4
    65663,    // 0b10000000001111111 - 9.1.4
    196479,   // 0b101111111101111111 - 22.2
    0         // reserved
};

const AP4_UI32
AP4_Ac4SamplingFrequencyTable[] =
{
    44100,  // 44.1 kHz
    48000   // 48 kHz
};

const AP4_UI32
AP4_Ac4SampleDeltaTable[AP4_AC4_FRAME_RATE_NUM] =
{
    2002,
    2000,
    1920,
    8008,  // 29.97 fps, using 240 000 media time scale
    1600,
    1001,
    1000,
    960,
    4004,  // 59.97 fps
    800,
    480,
    2002,  // 119.88 fps
    400,
    2048  // 23.44 fps, AC-4 native frame rate
};

const AP4_UI32
AP4_Ac4MediaTimeScaleTable[AP4_AC4_FRAME_RATE_NUM] =
{
    48000,
    48000,
    48000,
    240000,  //29.97 fps
    48000,
    48000,
    48000,
    48000,
    240000,  // 59.97 fps
    48000,
    48000,
    240000,  // 119.88 fps
    48000,
    48000,
};

/*----------------------------------------------------------------------
|   struct
+---------------------------------------------------------------------*/
struct AP4_Ac4EmdfInfo{
    AP4_UI08 emdf_version;
    AP4_UI16 key_id;
    AP4_UI08 b_emdf_payloads_substream_info;
    AP4_UI16 substream_index;
    AP4_UI08 protectionLengthPrimary;
    AP4_UI08 protectionLengthSecondary;
    AP4_UI08 protection_bits_primary[16];
    AP4_UI08 protection_bits_Secondary[16];
};

/*----------------------------------------------------------------------
|   non-inline functions
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Ac4VariableBits(AP4_BitReader &data, int nBits);

AP4_Result
AP4_Ac4ChannelCountFromSpeakerGroupIndexMask(unsigned int speakerGroupIndexMask);

/*----------------------------------------------------------------------
|   inline functions
+---------------------------------------------------------------------*/
inline AP4_Result
AP4_Ac4SuperSet(int lvalue, int rvalue)
{
    if ((lvalue == -1) || (lvalue > 15)) return rvalue;
    if ((rvalue == -1) || (rvalue > 15)) return lvalue;
    return SUPER_SET_CH_MODE[lvalue][rvalue];
}

inline AP4_Result 
Ap4_Ac4SubstreamGroupPartOfDefaultPresentation(unsigned int substreamGroupIndex, unsigned int presSubstreamGroupIndexes[], unsigned int presNSubstreamGroups) 
{
    int partOf = false;
    for (unsigned int idx = 0; idx < presNSubstreamGroups; idx ++) {
        if (presSubstreamGroupIndexes[idx] == substreamGroupIndex) { partOf = true; }
    }
    return partOf;
}

inline AP4_UI32
AP4_SetMaxGroupIndex(unsigned int groupIndex, unsigned int maxGroupIndex) 
{
    if (groupIndex > maxGroupIndex) { maxGroupIndex = groupIndex; }
    return maxGroupIndex;
}

inline void
AP4_ByteAlign(AP4_BitWriter &bits)
{
    unsigned int byte_align_nums = 8 - (bits.GetBitCount() % 8 == 0 ? 8: bits.GetBitCount() % 8);
    if (byte_align_nums){ bits.Write(0, byte_align_nums); }
}

inline void 
Ap4_Ac4UpdatePresBytes(const unsigned char* buffer, unsigned int idx, unsigned char value)
{    unsigned char *data = const_cast<unsigned char*>(buffer);
    *(data + idx) = value; 
}

#endif // _AP4_AC4_UTILS_H_
