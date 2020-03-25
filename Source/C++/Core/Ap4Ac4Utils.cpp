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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Ac4Utils.h"

AP4_UI32
AP4_Ac4VariableBits(AP4_BitReader &data, int nBits)
{
    AP4_UI32 value = 0;
    AP4_UI32 b_moreBits;
    do{
        value += data.ReadBits(nBits);
        b_moreBits = data.ReadBit();
        if (b_moreBits == 1) {
            value <<= nBits;
            value += (1<<nBits);
      }
    } while (b_moreBits == 1);
    return value;
}

AP4_Result
AP4_Ac4ChannelCountFromSpeakerGroupIndexMask(unsigned int speakerGroupIndexMask)
{

    unsigned int channelCount= 0;
    if ((speakerGroupIndexMask & 1) != 0) { // 0: L,R 0b1
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 2) != 0) { // 1: C 0b10
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 4) != 0) { // 2: Ls,Rs 0b100
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 8) != 0) { // 3: Lb,Rb 0b1000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 16) != 0) { // 4: Tfl,Tfr 0b10000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 32) != 0) { // 5: Tbl,Tbr 0b100000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 64) != 0) { // 6: LFE 0b1000000
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 128) != 0) { // 7: TL,TR 0b10000000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 256) != 0) { // 8: Tsl,Tsr 0b100000000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 512) != 0) { // 9: Tfc 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 1024) != 0) { // 10: Tbc 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 2048) != 0) { // 11: Tc 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 4096) != 0) { // 12: LFE2 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 8192) != 0) { // 13: Bfl,Bfr 
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 16384) != 0) { // 14: Bfc 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 32768) != 0) { // 15: Cb 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 65536) != 0) { // 16: Lscr,Rscr 
        channelCount += 2; 
    }
    if ((speakerGroupIndexMask & 131072) != 0) { // 17: Lw,Rw 
        channelCount += 2;
    } 
    if ((speakerGroupIndexMask & 262144) != 0) { // 18: Vhl,Vhr 
        channelCount += 2;
    }
    return channelCount;
}
