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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4BitStream.h"
#include "Ap4Eac3Parser.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------+
|    AP4_Eac3Header::AP4_Eac3Header
+----------------------------------------------------------------------*/
AP4_Eac3Header::AP4_Eac3Header(const AP4_UI08* bytes)
{
    AP4_BitReader bits(bytes, AP4_EAC3_HEADER_SIZE);
    bits.SkipBits(16);                          // sync word

    m_Strmtyp     = bits.ReadBits(2);
    m_Substreamid = bits.ReadBits(3);
    m_Frmsiz      = bits.ReadBits(11);
    m_FrameSize   = (m_Frmsiz + 1) * 2;

    m_Fscod       = bits.ReadBits(2);
    unsigned char numblkscod   = 0;
    unsigned int  blks_per_frm = 0;
    if (m_Fscod == 0x3){
       fprintf(stderr, "ERROR: Half sample rate unsupported\n");
       return;
    }else{
        numblkscod = bits.ReadBits(2);
        if (numblkscod == 0x3){
            blks_per_frm = 6;
        }else {
            blks_per_frm = numblkscod + 1;
        }
    }

    m_Acmod = bits.ReadBits(3);
    m_Lfeon = bits.ReadBits(1);
    m_ChannelCount = GLOBAL_CHANNEL_ARY[m_Acmod]  + m_Lfeon;

    m_Bsid  = bits.ReadBits(5);
    if (!((m_Bsid > 10) && (m_Bsid <=16))){
        fprintf(stderr, "ERROR: Unsupported bitstream id\n");
        return;
    }

    /* unsigned char dialnorm = */ bits.ReadBits(5);  // dialnorm
    // unsigned char compr;
    if (bits.ReadBit()) {                       // compre
        /* compr = */ bits.ReadBits(8);
    }
    
    if (m_Acmod == 0x0){                        // if 1+1 mode (dual mono, so some items need a second value)
        bits.SkipBits(5);                       // dialnorm2
        if (bits.ReadBit()) {                   // compr2e
            bits.SkipBits(8);                   // compr2
        } 
    }

    if (m_Strmtyp == 0x1) {                     // if dependent stream
        m_Chanmape = bits.ReadBit();
        if (m_Chanmape) {
            m_Chanmap  = bits.ReadBits(16);
        }else {
            //TODO: Derive chanmap from the acmod and lfeon parameters
            m_Chanmap = 0;
        }
    }else {
        m_Chanmape = 0;
        m_Chanmap  = 0;
    }
    
    // Extract mixing metadata 
    // unsigned char dmixmod, ltrtcmixlev, lorocmixlev, ltrtsurmixlev, lorosurmixlev, lfemixlevcod;
    if (bits.ReadBit()){                        // mixmdate
        if (m_Acmod  > 0x2) {
            /* dmixmod = */ bits.ReadBits(2);
        }
        if ((m_Acmod & 0x1) && (m_Acmod > 0x2)) {
            /* ltrtcmixlev = */ bits.ReadBits(3);
            /* lorocmixlev = */ bits.ReadBits(3);
        }
        if (m_Acmod & 0x4) {
            /* ltrtsurmixlev = */ bits.ReadBits(3);
            /* lorosurmixlev = */ bits.ReadBits(3);
        }
        if (m_Lfeon) { 
            if (bits.ReadBit()) {               // lfemixlevcode
                /* lfemixlevcod = */ bits.ReadBits(5);
            }
        } 
        if (m_Strmtyp == 0x0) {                 // if independent stream
            // unsigned char pgmscl, extpgmscl;
            if (bits.ReadBit()) {               // pgmscle
                /* pgmscl = */ bits.ReadBits(6);
            }

            if (m_Acmod == 0x0){                // if 1+1 mode (dual mono, so some items need a second value)
                if(bits.ReadBit()) {            // pgmscl2e
                    bits.SkipBits(6);           // pgmscl2
                } 
            }

            if(bits.ReadBit()) {                // extpgmscle
                /* extpgmscl = */ bits.ReadBits(6);
            }

            char mixdef = bits.ReadBits(2);
            if(mixdef == 0x1)       { bits.SkipBits(5) ;}   // premixcmpsel, drcsrc, premixcmpscl
            else if (mixdef == 0x2) { bits.SkipBits(12);}   // mixdata
            else if (mixdef == 0x3) {
                char mixdeflen = bits.ReadBits(5);
                unsigned int mixdefbits = 1;                // the initial value represents mixdata2e
                if(bits.ReadBit()){                         // mixdata2e
                    bits.SkipBits(5);                       // premixcmpsel, drcsrc, premixcmpscl
                    mixdefbits += 5; 

                    mixdefbits += 1;                        // extpgmlscle
                    if(bits.ReadBit()) { 
                        bits.SkipBits(4);                   // extpgmlscl
                        mixdefbits += 4;
                    }

                    mixdefbits += 1;                        // extpgmcscle
                    if(bits.ReadBit()) { 
                        bits.SkipBits(4);                   // extpgmcscl
                        mixdefbits += 4;
                    } 

                    mixdefbits += 1;                        // extpgmrscle
                    if(bits.ReadBit()) { 
                        bits.SkipBits(4);                   // extpgmrscl
                        mixdefbits += 4;
                    }

                    mixdefbits += 1;                        // extpgmlsscle
                    if(bits.ReadBit()) { 
                        bits.SkipBits(4);                   // extpgmlsscl
                        mixdefbits += 4;
                    } 

                    mixdefbits += 1;                        // extpgmrsscle
                    if(bits.ReadBit()) { 
                        bits.SkipBits(4);                   // extpgmrsscl
                        mixdefbits += 4;
                    } 

                    mixdefbits += 1;                        // extpgmlfescle
                    if(bits.ReadBit()) { 
                        bits.SkipBits(4);                   // extpgmlfescl
                        mixdefbits += 4;
                    } 

                    mixdefbits += 1;                        // dmixscle
                    if(bits.ReadBit()) { 
                        bits.SkipBits(4);                   // dmixscl
                        mixdefbits += 4;
                    } 

                    mixdefbits += 1;
                    if(bits.ReadBit()){                     // addche
                        mixdefbits += 1;
                        if(bits.ReadBit()) {                // extpgmaux1scle
                            bits.SkipBits(4);               // extpgmaux1scl
                            mixdefbits += 4;
                        } 

                        mixdefbits += 1;
                        if(bits.ReadBit()) {                // extpgmaux2scle
                            bits.SkipBits(4);               // extpgmaux2scl
                            mixdefbits += 4;
                        } 
                    }
                } //  if(bits.ReadBit()){   // mixdata2e

                mixdefbits += 1;
                if(bits.ReadBit()){                         // mixdata3e
                    bits.SkipBits(5);                       // spchdat
                    mixdefbits += 5;

                    mixdefbits += 1;
                    if(bits.ReadBit()){                     // addspchdate
                        bits.SkipBits(7);                   // spchdat1, spchan1att
                        mixdefbits += 7;

                        mixdefbits += 1;
                        if(bits.ReadBit()){                 // addspdat1e
                            bits.SkipBits(8);               // spchdat2, spchan2att
                            mixdefbits += 8;
                        }
                    }
                } //  if(bits.ReadBit()){   // mixdata3e

                bits.SkipBits(8 * (mixdeflen + 2) - mixdefbits); // mixdatafill
            }

            if (m_Acmod < 0x2) {                                // if mono or dual mono source 
                if(bits.ReadBit()){                             // paninfoe
                    bits.SkipBits(8 + 6);                       // panmean, paninfo
                }
                if (m_Acmod == 0x0) {                           // if 1+1 mode (dual mono - some items need a second value) 
                    if(bits.ReadBit()){                         // paninfo2e
                        bits.SkipBits(8 + 6);                   // panmean2, paninfo2
                    }
                }
            }

            if (bits.ReadBit()){                            // frmmixcfginfoe
                if (blks_per_frm == 1) {
                    bits.SkipBits(5);                       // blkmixcfginfo[0]
                }else{
                    for (unsigned int idx = 0; idx < blks_per_frm; idx++){
                        if(bits.ReadBit()){                 // blkmixcfginfoe
                            bits.SkipBits(5);               // blkmixcfginfo[blk]
                        }
                    }
                }
            }
        } // if (m_Strmtyp == 0x0)
    }

    m_Infomdate = bits.ReadBit();
    if (m_Infomdate){
        m_Bsmod = bits.ReadBits(3);
        // unsigned char copyrightb, origbs, dsurexmod;
        /* copyrightb = */ bits.ReadBits(1);
        /* origbs     = */ bits.ReadBits(1);

        if (m_Acmod == 0x2) {                               // if in 2/0 mode
            bits.SkipBits(4);                               // dsurmod, dheadphonmod
        }
        if (m_Acmod >= 0x6) {                               // if both surround channels exist
            /* dsurexmod = */ bits.ReadBits(2);
        }
        if(bits.ReadBit()){                                 // audprodie
            bits.SkipBits(8);                               // mixlevel, roomtyp, adconvtyp
        }
        if (m_Acmod == 0x0) {                               //if 1+1 mode (dual mono, so some items need a second value)
            if(bits.ReadBit()){                             // audprodi2e
                bits.SkipBits(8);                           // mixlevel2, roomtyp2, adconvtyp2
            }
        }
        if (m_Fscod < 0x3) {                                // if not half sample rate
            bits.SkipBits(1);                               // sourcefscod
        }
    } else { // if (m_Infomdate)
        m_Bsmod = 0;
    }
    
    m_Convsync = 1;
    if ((m_Strmtyp == 0x0) && (numblkscod != 0x3)){
        m_Convsync = bits.ReadBits(1); 
    }

    if (m_Strmtyp == 0x2) {                                 // if bit stream converted from AC-3
        unsigned char blkid = 0;
        if (numblkscod == 0x3) {
            blkid = 1;
        }else {
            blkid = bits.ReadBits(1);
        }
        if (blkid) {bits.SkipBits(6); }                     // frmsizecod
    } 

    m_Addbsie = bits.ReadBit();
    if (m_Addbsie){ 
        m_Addbsil = bits.ReadBits(6);
        for (unsigned int idx = 0 ; idx < (m_Addbsil + 1); idx ++){
            m_Addbsi[idx] = bits.ReadBits(8);
        }
    } else {
        m_Addbsil = 0;
        AP4_SetMemory(m_Addbsi, 0, sizeof (m_Addbsi));
    }
    m_HeadSize = (bits.GetBitsRead() / 8) + ((bits.GetBitsRead() % 8 == 0) ? 0: 1); 
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Header::MatchFixed
|
|    Check that two fixed headers are the same
|
+----------------------------------------------------------------------*/
bool
AP4_Eac3Header::MatchFixed(AP4_Eac3Header& frame, AP4_Eac3Header& next_frame)
{
    if (frame.m_Acmod ==  next_frame.m_Acmod &&
        frame.m_Bsid  ==  next_frame.m_Bsid  &&
        frame.m_Bsmod ==  next_frame.m_Bsmod &&
        frame.m_Lfeon ==  next_frame.m_Lfeon &&
        frame.m_Fscod ==  next_frame.m_Fscod ) {
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Header::Check
+----------------------------------------------------------------------*/
AP4_Result
AP4_Eac3Header::Check()
{
    if (m_Fscod == 1 || m_Fscod == 2) {
        fprintf(stderr, "WARN: The sample rate is NOT 48 kHz\n");
    } else if (m_Fscod == 3) {
        return AP4_FAILURE;
    }
    if (m_Bsid < 10 || m_Bsid > 16) {
        return AP4_FAILURE;
    }
    if (m_Substreamid != 0) {
        fprintf(stderr, "ERROR: Only single independent substream (I0) or single depenpent substream (D0) is allowed in a DD+ stream\n");
        return AP4_FAILURE;
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Parser::AP4_Eac3Parser
+----------------------------------------------------------------------*/
AP4_Eac3Parser::AP4_Eac3Parser() :
    m_FrameCount(0)
{
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Parser::~AP4_Eac3Parser
+----------------------------------------------------------------------*/
AP4_Eac3Parser::~AP4_Eac3Parser()
{
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Parser::Reset
+----------------------------------------------------------------------*/
AP4_Result
AP4_Eac3Parser::Reset()
{
    m_FrameCount = 0;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Parser::Feed
+----------------------------------------------------------------------*/
AP4_Result
AP4_Eac3Parser::Feed(const AP4_UI08* buffer, 
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
|    AP4_Eac3Parser::FindHeader
+----------------------------------------------------------------------*/
AP4_Result
AP4_Eac3Parser::FindHeader(AP4_UI08* header, AP4_Size& skip_size)
{
    AP4_Size available = m_Bits.GetBytesAvailable();

    /* look for the sync pattern */
    while (available-- >= AP4_EAC3_HEADER_SIZE) {
        m_Bits.PeekBytes(header, 2);

        if( (((header[0] << 8) | header[1]) == AP4_EAC3_SYNC_WORD_BIG_ENDIAN) || 
            (((header[0] << 8) | header[1]) == AP4_EAC3_SYNC_WORD_LITTLE_ENDIAN) ){
            if (((header[0] << 8) | header[1]) == AP4_EAC3_SYNC_WORD_LITTLE_ENDIAN) {
                m_LittleEndian = true;
            } else {
                m_LittleEndian = false;
            }
            /* found a sync pattern, read the entire the header */
            m_Bits.PeekBytes(header, AP4_EAC3_HEADER_SIZE);
            
           return AP4_SUCCESS;
        } else {
            m_Bits.SkipBytes(1); // skip
            skip_size++;
        }
    }

    return AP4_ERROR_NOT_ENOUGH_DATA;
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Parser::FindFrame
+----------------------------------------------------------------------*/
AP4_Result
AP4_Eac3Parser::FindFrame(AP4_Eac3Frame& frame)
{
    bool           dependent_stream_exist    = false;
    unsigned int   dependent_stream_chan_loc = 0;
    unsigned int   dependent_stream_length   = 0;
    unsigned int   skip_size = 0;
    unsigned int   available;
    unsigned char  raw_header[AP4_EAC3_HEADER_SIZE];
    AP4_Result     result;

    /* align to the start of the next byte */
    m_Bits.ByteAlign();
    
    /* find a frame header */
    result = FindHeader(raw_header, skip_size);
    if (AP4_FAILED(result)) return result;

    if (m_LittleEndian) {
        AP4_ByteSwap16(raw_header, AP4_EAC3_HEADER_SIZE);
    }

    /* parse the header */
    AP4_Eac3Header eac3_header(raw_header);

    /* check the header */
    result = eac3_header.Check();
    if (AP4_FAILED(result)) {
        goto fail;
    }
    
    /* check if we have enough data to peek at the next header */
    available = m_Bits.GetBytesAvailable();
    if (available >= eac3_header.m_FrameSize + AP4_EAC3_HEADER_SIZE) {
        // enough to peek at the header of the next frame
        unsigned char peek_raw_header[AP4_EAC3_HEADER_SIZE];

        m_Bits.SkipBytes(eac3_header.m_FrameSize);
        skip_size = 0;
        result = FindHeader(peek_raw_header, skip_size);
        if (AP4_FAILED(result)) return result;
        m_Bits.SkipBytes(-((int)(eac3_header.m_FrameSize + skip_size)));

        if (m_LittleEndian) {
            AP4_ByteSwap16(peek_raw_header, AP4_EAC3_HEADER_SIZE);
        }
        /* check the header */
        AP4_Eac3Header peek_eac3_header(peek_raw_header);
        result = peek_eac3_header.Check();
        if (AP4_FAILED(result)) {
            goto fail;
        }

        // TODO: Only support 7.1-channel now
        if (peek_eac3_header.m_Strmtyp  == 1) {
            dependent_stream_exist = true;
            if (peek_eac3_header.m_Chanmape == 0){
                goto fail;
            }else {
                if (peek_eac3_header.m_Chanmap & 0x200) {
                    dependent_stream_chan_loc  |= 0x2;
                    dependent_stream_length     = peek_eac3_header.m_FrameSize;
                    eac3_header.m_ChannelCount += 2;
                } else {
                    fprintf(stderr, "ERROR: Only support 7.1-channel (I0 + D0). For other D0, the tool doesn't support yet.\n");
                    goto fail;
                }
            }
        }

        /* check that the fixed part of this header is the same as the */
        /* fixed part of the previous header                           */
        else if (!AP4_Eac3Header::MatchFixed(eac3_header, peek_eac3_header)) {
            goto fail;
        }
    } else if (available < eac3_header.m_FrameSize || (m_Bits.m_Flags & AP4_BITSTREAM_FLAG_EOS) == 0) {
        // not enough for a frame, or not at the end (in which case we'll want to peek at the next header)
        return AP4_ERROR_NOT_ENOUGH_DATA;
    }

    /* fill in the frame info */
    frame.m_Info.m_ChannelCount   = eac3_header.m_ChannelCount;
    if (dependent_stream_exist) {
        frame.m_Info.m_FrameSize  = eac3_header.m_FrameSize + dependent_stream_length;
    }else {
        frame.m_Info.m_FrameSize  = eac3_header.m_FrameSize;
    }
    frame.m_Info.m_SampleRate                = EAC3_SAMPLE_RATE_ARY[eac3_header.m_Fscod];
    frame.m_Info.m_Eac3SubStream.fscod       = eac3_header.m_Fscod;
    frame.m_Info.m_Eac3SubStream.bsid        = eac3_header.m_Bsid;
    frame.m_Info.m_Eac3SubStream.bsmod       = eac3_header.m_Bsmod;
    frame.m_Info.m_Eac3SubStream.acmod       = eac3_header.m_Acmod;
    frame.m_Info.m_Eac3SubStream.lfeon       = eac3_header.m_Lfeon;
    if (dependent_stream_exist) {
        frame.m_Info.m_Eac3SubStream.num_dep_sub = 1;    
        frame.m_Info.m_Eac3SubStream.chan_loc    = dependent_stream_chan_loc;
    }else {
        frame.m_Info.m_Eac3SubStream.num_dep_sub = 0;    
        frame.m_Info.m_Eac3SubStream.chan_loc    = 0;
    }

    frame.m_Info.complexity_index_type_a = 0;
    if (eac3_header.m_Addbsie && (eac3_header.m_Addbsil == 1) && (eac3_header.m_Addbsi[0] == 0x1)){
        frame.m_Info.complexity_index_type_a = eac3_header.m_Addbsi[1];
    }   

    /* set the little endian flag */
    frame.m_LittleEndian = m_LittleEndian;

    /* set the frame source */
    frame.m_Source = &m_Bits;

    return AP4_SUCCESS;

fail:
    return AP4_ERROR_CORRUPTED_BITSTREAM;
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Parser::GetBytesFree
+----------------------------------------------------------------------*/
AP4_Size
AP4_Eac3Parser::GetBytesFree()
{
    return (m_Bits.GetBytesFree());
}

/*----------------------------------------------------------------------+
|    AP4_Eac3Parser::GetBytesAvailable
+----------------------------------------------------------------------*/
AP4_Size    
AP4_Eac3Parser::GetBytesAvailable()
{
    return (m_Bits.GetBytesAvailable());
}
