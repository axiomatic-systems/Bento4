/*****************************************************************
|
|    AP4 - E-AC-3 Sync Frame Parser
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

#ifndef _AP4_EAC3_PARSER_H_
#define _AP4_EAC3_PARSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4BitStream.h"
#include "Ap4Dec3Atom.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define AP4_EAC3_HEADER_SIZE                64      /* The header size for E-AC-3 parser, syncinfo() + bsi() */
#define AP4_EAC3_SYNC_WORD_BIG_ENDIAN       0x0B77  /* Big endian, 16 sync bits */
#define AP4_EAC3_SYNC_WORD_LITTLE_ENDIAN    0x770B  /* Little endian, 16 sync bits */

const AP4_UI08 GLOBAL_CHANNEL_ARY[]   = {2, 1, 2, 3, 3, 4, 4, 5};   /* Channel count number from acmod value */
const AP4_UI32 EAC3_SAMPLE_RATE_ARY[] = {48000, 44100, 32000};      /* Sample rate from fscode vale */

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
class AP4_Eac3Header {
public:
    // constructor
    AP4_Eac3Header(const AP4_UI08* bytes);
    
    // methods
    AP4_Result Check();

    AP4_UI32 m_HeadSize;
    AP4_UI32 m_ChannelCount;
    AP4_UI32 m_FrameSize;

    // E-AC-3 bsi()
    AP4_UI32 m_Strmtyp;
    AP4_UI32 m_Substreamid;
    AP4_UI32 m_Frmsiz;
    AP4_UI32 m_Fscod;
    AP4_UI32 m_Acmod;
    AP4_UI32 m_Lfeon;
    AP4_UI32 m_Bsid;
    AP4_UI32 m_Chanmape;
    AP4_UI32 m_Chanmap;
    AP4_UI32 m_Infomdate;
    AP4_UI32 m_Bsmod;
    AP4_UI32 m_Convsync;
    AP4_UI32 m_Addbsie;
    AP4_UI32 m_Addbsil;         // 6 bits
    AP4_UI08 m_Addbsi[65];      // (addbsil + 1)* 8

    // class methods
    static bool MatchFixed(AP4_Eac3Header& frame, AP4_Eac3Header& next_frame);
};

typedef struct {
    AP4_UI32  m_ChannelCount;
    AP4_UI32  m_FrameSize;
    AP4_UI32  m_SampleRate;
    AP4_Dec3Atom::SubStream m_Eac3SubStream;
    AP4_UI32  complexity_index_type_a;
} AP4_Eac3FrameInfo;

typedef struct {
    AP4_BitStream*   m_Source;
    AP4_Eac3FrameInfo m_Info;
    AP4_Flags m_LittleEndian;
} AP4_Eac3Frame;

class AP4_Eac3Parser {
public:
    // constructor and destructor
    AP4_Eac3Parser();
    virtual ~AP4_Eac3Parser();

    // methods
    AP4_Result Reset();
    AP4_Result Feed(const AP4_UI08* buffer, 
                    AP4_Size*       buffer_size,
                    AP4_Flags       flags = 0);
    AP4_Result FindFrame(AP4_Eac3Frame& frame);
    AP4_Result Skip(AP4_Size size);
    AP4_Size   GetBytesFree();
    AP4_Size   GetBytesAvailable();

private:
    // methods
    AP4_Result FindHeader(AP4_UI08* header, AP4_Size& skip_size);

    // members
    AP4_BitStream m_Bits;
    AP4_Cardinal  m_FrameCount;
    AP4_Flags     m_LittleEndian;
};

#endif // _AP4_EAC3_PARSER_H_
