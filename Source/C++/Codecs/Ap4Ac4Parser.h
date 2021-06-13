/*****************************************************************
|
|    AP4 - AC4 Sync Frame Parser
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

#ifndef _AP4_AC4_PARSER_H_
#define _AP4_AC4_PARSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4BitStream.h"
#include "Ap4Dac4Atom.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define AP4_AC4_HEADER_SIZE     7       /* The header size for AC-4 parser, only include (sync word + frame size) */
#define AP4_AC4_MAX_TOC_SIZE    512     /* Ths assumption of max toc size */
#define AP4_AC4_SYNC_WORD       0xAC40  /* 16 sync bits without CRC */
#define AP4_AC4_SYNC_WORD_CRC   0xAC41  /* 16 sync bits with    CRC */

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
class AP4_Ac4Header {
public:
    // constructor
    AP4_Ac4Header(const AP4_UI08* bytes, unsigned int size);
    
    // methods
    AP4_Result Check();

    // AC-4 Sync Frame Inforamtion
    AP4_UI32 m_SyncWord;
    AP4_UI32 m_HeaderSize;
    AP4_UI32 m_FrameSize;
    AP4_UI32 m_CrcSize;

    // AC-4 Channel Count
    AP4_UI32 m_ChannelCount;

    // Ac-4 General/High Level TOC Information
    AP4_UI32 m_BitstreamVersion;
    AP4_UI32 m_SequenceCounter;
    AP4_UI32 m_BWaitFrames;
    AP4_UI32 m_WaitFrames;
    AP4_UI32 m_BrCode;
    AP4_UI32 m_FsIndex;
    AP4_UI32 m_FrameRateIndex;
    AP4_UI32 m_BIframeGlobal;
    AP4_UI32 m_BSinglePresentation;
    AP4_UI32 m_BMorePresentations;
    AP4_UI32 m_NPresentations;
    AP4_UI32 m_BPayloadBase;
    AP4_UI32 m_PayloadBase;
    AP4_UI32 m_BProgramId;
    AP4_UI32 m_ShortProgramId;
    AP4_UI32 m_BProgramUuidPresent;
    AP4_Byte m_ProgramUuid[16];

    static bool m_DeprecatedV0;

    // AC-4 Presentation Information
    AP4_Dac4Atom::Ac4Dsi::PresentationV1 *m_PresentationV1;
    // class methods
    static bool MatchFixed(AP4_Ac4Header& frame, AP4_Ac4Header& next_frame);
    
private:
    AP4_Result GetPresentationVersionBySGIndex(unsigned int substream_group_index);
    AP4_Result GetPresentationIndexBySGIndex  (unsigned int substream_group_index);
};

typedef struct {
    AP4_UI32  m_HeaderSize;
    AP4_UI32  m_FrameSize;
    AP4_UI32  m_CRCSize;
    AP4_UI32  m_ChannelCount;
    AP4_UI32  m_SampleDuration;
    AP4_UI32  m_MediaTimeScale;
    AP4_UI32  m_Iframe;
    AP4_Dac4Atom::Ac4Dsi m_Ac4Dsi;
} AP4_Ac4FrameInfo;

typedef struct {
    AP4_BitStream*   m_Source;
    AP4_Ac4FrameInfo m_Info;
} AP4_Ac4Frame;

class AP4_Ac4Parser {
public:
    // constructor and destructor
    AP4_Ac4Parser();
    virtual ~AP4_Ac4Parser();

    // methods
    AP4_Result Reset();
    AP4_Result Feed(const AP4_UI08* buffer, 
                    AP4_Size*       buffer_size,
                    AP4_Flags       flags = 0);
    AP4_Result FindFrame(AP4_Ac4Frame& frame);
    AP4_Result Skip(AP4_Size size);
    AP4_Size   GetBytesFree();
    AP4_Size   GetBytesAvailable();

private:
    // methods
    AP4_Result FindHeader(AP4_UI08* header);
    AP4_UI32   GetSyncFrameSize(AP4_BitReader &bits);

    // members
    AP4_BitStream m_Bits;
    AP4_Cardinal  m_FrameCount;
};

#endif // _AP4_AC4_PARSER_H_
