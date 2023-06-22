/*****************************************************************
 |
 |    AP4 - AC-3 Sync Frame Parser
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

#ifndef _AP4_AC3_PARSER_H_
#define _AP4_AC3_PARSER_H_

/*----------------------------------------------------------------------
 |   includes
 +---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4BitStream.h"
#include "Ap4Dac3Atom.h"
#include "Ap4Eac3Parser.h"

/*----------------------------------------------------------------------
 |   constants
 +---------------------------------------------------------------------*/
#define AP4_AC3_HEADER_SIZE                32      /* The header size for AC-3 parser, syncinfo() + bsi() */
#define AP4_AC3_SYNC_WORD_BIG_ENDIAN       0x0B77  /* Big endian, 16 sync bits */
#define AP4_AC3_SYNC_WORD_LITTLE_ENDIAN    0x770B  /* Little endian, 16 sync bits */

const AP4_UI32 FRAME_SIZE_CODE_ARY_AC3[3][38] = {
    {64,64,80,80,96,96,112,112,128,128,160,160,192,192,224,224,256,256,320,320,384,384,448,448,512,512,640,640,768,768,896,896,1024,1024,1152,1152,1280,1280},
    {69,70,87,88,104,105,121,122,139,140,174,175,208,209,243,244,278,279,348,349,417,418,487,488,557,558696,697,835,836,976,977,1114,1115,1253,1254,1393,1394},
    {96,96,120,120,144,144,168,168,192,192,240,240,288,288,336,336,384,384,480,480,576,576,672,672,768,768,960,960,1152,1152,1344,1344,1536,1536,1728,1728,1920,1920}};
const AP4_UI32 FSCOD_AC3[4] = {48000, 44100, 32000, 0};

/*----------------------------------------------------------------------
 |   types
 +---------------------------------------------------------------------*/
class AP4_Ac3Header {
public:
    // constructor
    AP4_Ac3Header(const AP4_UI08* bytes);
    
    // methods
    AP4_Result Check();
    
    AP4_UI32 m_HeadSize;
    AP4_UI32 m_FrameSize;
    AP4_UI32 m_ChannelCount;
    
    // AC-3 sysninfo()
    AP4_UI32 m_Fscod;
    AP4_UI32 m_Frmsizecod;
    
    // AC-3 bsi()
    AP4_UI32 m_Bsid;
    AP4_UI32 m_Bsmod;
    AP4_UI32 m_Acmod;
    AP4_UI32 m_Lfeon;
    AP4_UI32 m_Addbsie;
    AP4_UI32 m_Addbsil;         // 6 bits
    AP4_UI08 m_addbsi[65];      // (addbsil+1)×8

    // class methods
    static bool MatchFixed(AP4_Ac3Header& frame, AP4_Ac3Header& next_frame);
};

typedef struct {
    AP4_UI32  m_ChannelCount;
    AP4_UI32  m_FrameSize;
    AP4_UI32  m_SampleRate;
    AP4_Dac3Atom::StreamInfo m_Ac3StreamInfo;
} AP4_Ac3FrameInfo;

typedef struct {
    AP4_BitStream*   m_Source;
    AP4_Ac3FrameInfo m_Info;
    AP4_Flags m_LittleEndian;
} AP4_Ac3Frame;

class AP4_Ac3Parser {
public:
    // constructor and destructor
    AP4_Ac3Parser();
    virtual ~AP4_Ac3Parser();
    
    // methods
    AP4_Result Reset();
    AP4_Result Feed(const AP4_UI08* buffer,
                    AP4_Size*       buffer_size,
                    AP4_Flags       flags = 0);
    AP4_Result FindFrame(AP4_Ac3Frame& frame);
    AP4_Result Skip(AP4_Size size);
    AP4_Size   GetBytesFree();
    AP4_Size   GetBytesAvailable();
    
private:
    // methods
    AP4_Result FindHeader(AP4_UI08* header);
    
    // members
    AP4_BitStream m_Bits;
    AP4_Cardinal  m_FrameCount;
    AP4_Flags     m_LittleEndian;
};

#endif // _AP4_AC3_PARSER_H_
