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

/*----------------------------------------------------------------------
 |   includes
 +---------------------------------------------------------------------*/
#include "Ap4BitStream.h"
#include "Ap4Ac3Parser.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------+
 |    AP4_Ac3Header::AP4_Ac3Header
 +----------------------------------------------------------------------*/
AP4_Ac3Header::AP4_Ac3Header(const AP4_UI08* bytes)
{
    AP4_BitReader bits(bytes, AP4_AC3_HEADER_SIZE);
    bits.SkipBits(16);                  // sync word
    bits.SkipBits(16);                  // crc1
    m_Fscod       = bits.ReadBits(2);
    m_Frmsizecod  = bits.ReadBits(6);   // frmsizecod
    m_FrameSize   = FRAME_SIZE_CODE_ARY_AC3[m_Fscod][m_Frmsizecod] * 2;
    
    m_Bsid        = bits.ReadBits(5);
    m_Bsmod       = bits.ReadBits(3);
    m_Acmod       = bits.ReadBits(3);
    if((m_Acmod & 0x1) && (m_Acmod != 0x1)){
        bits.SkipBits(2);               //cmixlev
    }
    if(m_Acmod & 0x4){
        bits.SkipBits(2);               //surmixlev
    }
    if(m_Acmod == 0x2){
        bits.SkipBits(2);               //dsurmod
    }
    m_Lfeon         = bits.ReadBit();
    m_ChannelCount  = GLOBAL_CHANNEL_ARY[m_Acmod] + m_Lfeon;
    bits.SkipBits(5);                   // dialnorm
    if(bits.ReadBit()){                 // compre
        bits.SkipBits(8);               // compr
    }
    if(bits.ReadBit()){                 // langcode
        bits.SkipBits(8);               // langcod
    }
    if(bits.ReadBit()){                 // audprodie
        bits.SkipBits(5);               // mixlevel
        bits.SkipBits(2);               // roomtyp
    }
    if (m_Acmod == 0){
        bits.SkipBits(5);               // dialnorm2
        if(bits.ReadBit()){             // compr2e
            bits.SkipBits(8);           // compr2
        }
        if(bits.ReadBit()){             // langcod2e
            bits.SkipBits(8);           // langcod2
        }
        if(bits.ReadBit()){             // audprodi2e
            bits.SkipBits(5);           // mixlevel2
            bits.SkipBits(2);           // roomtyp2
        }
    }
    bits.SkipBits(1);                   // copyrightb
    bits.SkipBits(1);                   // origbs
    if(bits.ReadBit()){                 // timecod1e
        bits.SkipBits(14);              // timecod1
    }
    if(bits.ReadBit()){                 //timecod2e
        bits.SkipBits(14);              // timecod2
    }
    m_Addbsie = bits.ReadBit();
    if (m_Addbsie){
        m_Addbsil = bits.ReadBits(6);
        for (unsigned int idx = 0 ; idx < (m_Addbsil + 1); idx ++){
            m_addbsi[idx] = bits.ReadBits(8);
        }
    } else {
        m_Addbsil = 0;
        AP4_SetMemory(m_addbsi, 0, sizeof (m_addbsi));
    }
    m_HeadSize = (bits.GetBitsRead() / 8) + ((bits.GetBitsRead() % 8 == 0) ? 0: 1);
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Header::MatchFixed
 |
 |    Check that two fixed headers are the same
 |
 +----------------------------------------------------------------------*/
bool
AP4_Ac3Header::MatchFixed(AP4_Ac3Header& frame, AP4_Ac3Header& next_frame)
{
    return true;
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Header::Check
 +----------------------------------------------------------------------*/
AP4_Result
AP4_Ac3Header::Check()
{
    if (m_Bsid > 8) {
        return AP4_FAILURE;
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Parser::AP4_Ac3Parser
 +----------------------------------------------------------------------*/
AP4_Ac3Parser::AP4_Ac3Parser() :
m_FrameCount(0)
{
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Parser::~AP4_Ac3Parser
 +----------------------------------------------------------------------*/
AP4_Ac3Parser::~AP4_Ac3Parser()
{
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Parser::Reset
 +----------------------------------------------------------------------*/
AP4_Result
AP4_Ac3Parser::Reset()
{
    m_FrameCount = 0;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Parser::Feed
 +----------------------------------------------------------------------*/
AP4_Result
AP4_Ac3Parser::Feed(const AP4_UI08* buffer,
                     AP4_Size*       buffer_size,
                     AP4_Flags       flags)
{
    AP4_Size free_space;
    
    /* update flags */
    m_Bits.m_Flags = flags;
    
    /* possible shortcut */
    if (buffer == NULL ||
        buffer_size == NULL ||
        *buffer_size == 0) {
        return AP4_SUCCESS;
    }
    
    /* see how much data we can write */
    free_space = m_Bits.GetBytesFree();
    if (*buffer_size > free_space) *buffer_size = free_space;
    if (*buffer_size == 0) return AP4_SUCCESS;
    
    /* write the data */
    return m_Bits.WriteBytes(buffer, *buffer_size);
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Parser::FindHeader
 +----------------------------------------------------------------------*/
AP4_Result
AP4_Ac3Parser::FindHeader(AP4_UI08* header)
{
    AP4_Size available = m_Bits.GetBytesAvailable();
    
    /* look for the sync pattern */
    while (available-- >= AP4_AC3_HEADER_SIZE) {
        m_Bits.PeekBytes(header, 2);  // ac3 header begins with 0x0B77
        
        if( (((header[0] << 8) | header[1]) == AP4_AC3_SYNC_WORD_BIG_ENDIAN) ||
           (((header[0] << 8) | header[1]) == AP4_AC3_SYNC_WORD_LITTLE_ENDIAN) ){
            // Little Endian
            if (((header[0] << 8) | header[1]) == AP4_AC3_SYNC_WORD_LITTLE_ENDIAN) {
                m_LittleEndian = true;
            } else {
                m_LittleEndian = false;
            }
            /* found a sync pattern, read the entire the header */
            m_Bits.PeekBytes(header, AP4_AC3_HEADER_SIZE);
            
            return AP4_SUCCESS;
        } else {
            m_Bits.SkipBytes(1); // skip
        }
    }
    
    return AP4_ERROR_NOT_ENOUGH_DATA;
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Parser::FindFrame
 +----------------------------------------------------------------------*/
AP4_Result
AP4_Ac3Parser::FindFrame(AP4_Ac3Frame& frame)
{
    unsigned int   available;
    unsigned char  raw_header[AP4_AC3_HEADER_SIZE];
    AP4_Result     result;
    
    /* align to the start of the next byte */
    m_Bits.ByteAlign();
    
    /* find a frame header */
    result = FindHeader(raw_header);
    if (AP4_FAILED(result)) return result;
    
    if (m_LittleEndian) {
        AP4_ByteSwap16(raw_header, AP4_AC3_HEADER_SIZE);
    }
    
    /* parse the header */
    AP4_Ac3Header ac3_header(raw_header);
    
    /* check the header */
    result = ac3_header.Check();
    if (AP4_FAILED(result)) {
        m_Bits.SkipBytes(2);
        goto fail;
    }
    
    /* check if we have enough data to peek at the next header */
    available = m_Bits.GetBytesAvailable();
    if (available >= ac3_header.m_FrameSize + AP4_AC3_HEADER_SIZE) {
        // enough to peek at the header of the next frame
        unsigned char peek_raw_header[AP4_AC3_HEADER_SIZE];
        
        m_Bits.SkipBytes(ac3_header.m_FrameSize);
        m_Bits.PeekBytes(peek_raw_header, AP4_AC3_HEADER_SIZE);
        m_Bits.SkipBytes(-((int)ac3_header.m_FrameSize));
        
        if (m_LittleEndian) {
            AP4_ByteSwap16(peek_raw_header, AP4_AC3_HEADER_SIZE);
        }
        /* check the header */
        AP4_Ac3Header peek_ac3_header(peek_raw_header);
        result = peek_ac3_header.Check();
        if (AP4_FAILED(result)) {
            m_Bits.SkipBytes(ac3_header.m_FrameSize + 2);
            goto fail;
        }
        
        /* check that the fixed part of this header is the same as the */
        /* fixed part of the previous header                           */
        else if (!AP4_Ac3Header::MatchFixed(ac3_header, peek_ac3_header)) {
            m_Bits.SkipBytes(ac3_header.m_FrameSize + 2);
            goto fail;
        }
    } else if (available < ac3_header.m_FrameSize || (m_Bits.m_Flags & AP4_BITSTREAM_FLAG_EOS) == 0) {
        // not enough for a frame, or not at the end (in which case we'll want to peek at the next header)
        return AP4_ERROR_NOT_ENOUGH_DATA;
    }
    frame.m_Info.m_ChannelCount                 = ac3_header.m_ChannelCount;
    frame.m_Info.m_SampleRate                   = FSCOD_AC3[ac3_header.m_Fscod];
    frame.m_Info.m_FrameSize                    = ac3_header.m_FrameSize;
    frame.m_Info.m_Ac3StreamInfo.fscod          = ac3_header.m_Fscod;
    frame.m_Info.m_Ac3StreamInfo.bsid           = ac3_header.m_Bsid;
    frame.m_Info.m_Ac3StreamInfo.bsmod          = ac3_header.m_Bsmod;
    frame.m_Info.m_Ac3StreamInfo.acmod          = ac3_header.m_Acmod;
    frame.m_Info.m_Ac3StreamInfo.lfeon          = ac3_header.m_Lfeon;
    frame.m_Info.m_Ac3StreamInfo.bit_rate_code  = ac3_header.m_Frmsizecod / 2;
    
    frame.m_LittleEndian = m_LittleEndian;
    
    /* set the frame source */
    frame.m_Source = &m_Bits;
    
    return AP4_SUCCESS;
    
fail:
    /* skip the header and return (only skip the first byte in  */
    /* case this was a false header that hides one just after)  */
    //m_Bits.SkipBytes(-(AP4_ADTS_HEADER_SIZE-1));
    return AP4_ERROR_CORRUPTED_BITSTREAM;
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Parser::GetBytesFree
 +----------------------------------------------------------------------*/
AP4_Size
AP4_Ac3Parser::GetBytesFree()
{
    return (m_Bits.GetBytesFree());
}

/*----------------------------------------------------------------------+
 |    AP4_Ac3Parser::GetBytesAvailable
 +----------------------------------------------------------------------*/
AP4_Size
AP4_Ac3Parser::GetBytesAvailable()
{
    return (m_Bits.GetBytesAvailable());
}
