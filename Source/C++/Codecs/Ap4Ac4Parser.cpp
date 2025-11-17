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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4BitStream.h"
#include "Ap4Ac4Parser.h"

bool AP4_Ac4Header::m_DeprecatedV0 = true;
/*----------------------------------------------------------------------+
|    AP4_Ac4Header::AP4_Ac4Header
+----------------------------------------------------------------------*/
AP4_Ac4Header::AP4_Ac4Header(const AP4_UI08* bytes, unsigned int size, bool header_exists)
{
    /* Read the entire the frame */
    AP4_BitReader bits(bytes, size);

    /* Sync word and frame_size()*/
    if (header_exists) {
        m_SyncWord = bits.ReadBits(16);
        m_HeaderSize = 2;
        if (m_SyncWord == AP4_AC4_SYNC_WORD_CRC){
            m_CrcSize = 2;
        }else {
            m_CrcSize = 0;
        }
        m_FrameSize = bits.ReadBits(16);
        m_HeaderSize += 2;
        if (m_FrameSize == 0xFFFF){
            m_FrameSize = bits.ReadBits(24);
            m_HeaderSize += 3;
        }
    }

    /* Begin to parse TOC */
    AP4_Position tocStart = bits.GetBitsPosition() / 8;

    m_BitstreamVersion = bits.ReadBits(2);
    if (m_BitstreamVersion == 3) {
        m_BitstreamVersion = AP4_Ac4VariableBits(bits, 2);
    }
    m_SequenceCounter  = bits.ReadBits(10);
    m_BWaitFrames      = bits.ReadBit();
    if (m_BWaitFrames) {
        m_WaitFrames = bits.ReadBits(3);
        if (m_WaitFrames > 0){ m_BrCode = bits.ReadBits(2);}
    }else {
        m_WaitFrames = -1;
    }

    m_FsIndex               = bits.ReadBit();
    m_FrameRateIndex        = bits.ReadBits(4);
    m_BIframeGlobal         = bits.ReadBit();
    m_BSinglePresentation   = bits.ReadBit();
    if (m_BSinglePresentation == 1){
        m_NPresentations = 1;
        m_BMorePresentations = 0;
    }else{
        m_BMorePresentations = bits.ReadBit();
        if (m_BMorePresentations == 1){
            m_NPresentations = AP4_Ac4VariableBits(bits, 2) + 2;
        }else{
            m_NPresentations = 0;
        }
    }
    m_PayloadBase = 0;
    m_BPayloadBase = bits.ReadBit();
    if (m_BPayloadBase == 1){
        m_PayloadBase = bits.ReadBits(5)  + 1;
        if (m_PayloadBase == 0x20){
            m_PayloadBase += AP4_Ac4VariableBits(bits, 3);
        }
    }
    if (m_BitstreamVersion <= 1){
        if (m_DeprecatedV0){
            m_DeprecatedV0 = false;
            printf("Warning: Bitstream version 0 is deprecated\n");
        }
    }else{
        m_BProgramId = bits.ReadBit();
        if (m_BProgramId == 1){
            m_ShortProgramId = bits.ReadBits(16);
            m_BProgramUuidPresent = bits.ReadBit();
            if (m_BProgramUuidPresent == 1){
                for (int cnt = 0; cnt < 16; cnt++){
                    m_ProgramUuid[cnt] = bits.ReadBits(8);
                }
            }else {
                memcpy(m_ProgramUuid,"ONDEL TEAM UUID\0", 16);
            }
        }else {
            m_ShortProgramId = 0;
            m_BProgramUuidPresent = 0;
            memcpy(m_ProgramUuid, "ONDEL TEAM UUID\0", 16);
        }
        unsigned int maxGroupIndex = 0;
        unsigned int *firstPresentationSubstreamGroupIndexes = NULL;
        unsigned int firstPresentationNSubstreamGroups = 0;

        if (m_NPresentations > 0){
            m_PresentationV1 = new AP4_Dac4Atom::Ac4Dsi::PresentationV1[m_NPresentations];
            AP4_SetMemory(m_PresentationV1, 0, m_NPresentations * sizeof(m_PresentationV1[0]));
        } else {
            m_PresentationV1 = NULL;
        }
        // ac4_presentation_v1_info()
        for (unsigned int pres_idx = 0; pres_idx < m_NPresentations; pres_idx++){
            AP4_Dac4Atom::Ac4Dsi::PresentationV1& presentation = m_PresentationV1[pres_idx];
            presentation.ParsePresentationV1Info(bits,
                                                 m_BitstreamVersion,
                                                 m_FrameRateIndex,
                                                 pres_idx,
                                                 maxGroupIndex,
                                                 &firstPresentationSubstreamGroupIndexes,
                                                 firstPresentationNSubstreamGroups);            
        }
        unsigned int bObjorAjoc = 0;
        unsigned int channelCount = 0;
        unsigned int speakerGroupIndexMask = 0;
        AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 *substream_groups = new AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1[maxGroupIndex + 1];
        AP4_SetMemory(substream_groups, 0, (maxGroupIndex + 1) * sizeof(substream_groups[0]));
        for (unsigned int sg = 0 ; (sg < (maxGroupIndex + 1)) && (m_NPresentations > 0); sg ++) {
            AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 &substream_group = substream_groups[sg];
            AP4_Result pres_index = GetPresentationIndexBySGIndex(sg);
            if (pres_index == AP4_FAILURE) {
                break;
            }
            unsigned int localChannelCount = 0;
            unsigned int frame_rate_factor = (m_PresentationV1[pres_index].d.v1.dsi_frame_rate_multiply_info == 0)? 1: (m_PresentationV1[pres_index].d.v1.dsi_frame_rate_multiply_info * 2);
            substream_group.ParseSubstreamGroupInfo(bits,
                                                    m_BitstreamVersion,
                                                    GetPresentationVersionBySGIndex(sg),
                                                    Ap4_Ac4SubstreamGroupPartOfDefaultPresentation(sg, firstPresentationSubstreamGroupIndexes, firstPresentationNSubstreamGroups),
                                                    frame_rate_factor,
                                                    m_FsIndex,
                                                    localChannelCount,
                                                    speakerGroupIndexMask,
                                                    bObjorAjoc);
            if (channelCount < localChannelCount) { channelCount = localChannelCount;}
        }

        for (unsigned int pres_idx = 0; pres_idx < m_NPresentations; pres_idx++){
            m_PresentationV1[pres_idx].d.v1.substream_groups = new AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1[m_PresentationV1[pres_idx].d.v1.n_substream_groups];
            for (unsigned int sg = 0; sg < m_PresentationV1[pres_idx].d.v1.n_substream_groups; sg++){
                // TODO: Default Copy Construct Function
                m_PresentationV1[pres_idx].d.v1.substream_groups[sg]   = substream_groups[m_PresentationV1[pres_idx].d.v1.substream_group_indexs[sg]];
                m_PresentationV1[pres_idx].d.v1.dolby_atmos_indicator |= m_PresentationV1[pres_idx].d.v1.substream_groups[sg].d.v1.dolby_atmos_indicator;
            }
        }
        delete[] substream_groups;

        if (bObjorAjoc == 0){
            m_ChannelCount = AP4_Ac4ChannelCountFromSpeakerGroupIndexMask(speakerGroupIndexMask);
        }else {
            m_ChannelCount = 2;
        }
    }

    // substream_index_table
    AP4_UI32 n_substreams = bits.ReadBits(2);
    if (n_substreams == 0) {
        n_substreams = AP4_Ac4VariableBits(bits, 2) + 4;
    }
    AP4_UI08 b_size_present;
    if (n_substreams == 1) {
        b_size_present = bits.ReadBit();
    } else {
        b_size_present = 1;
    }
    if (b_size_present) {
        for (AP4_UI32 s = 0; s < n_substreams; s++) {
            AP4_UI08 b_more_bits = bits.ReadBit();
            AP4_UI32 substream_size = bits.ReadBits(10);
            if (b_more_bits) {
                substream_size += (AP4_Ac4VariableBits(bits, 2) << 10);
            }
        }
    }
    if (bits.GetBitsPosition() % 8) {
        bits.SkipBits(8 - (bits.GetBitsPosition() % 8));
    }
    m_TocSize = bits.GetBitsPosition() / 8 - tocStart;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Header::MatchFixed
|
|    Check that two fixed headers are the same
|
+----------------------------------------------------------------------*/
bool
AP4_Ac4Header::MatchFixed(AP4_Ac4Header& frame, AP4_Ac4Header& next_frame)
{
    // Some parameter shall be const which defined in AC-4 in ISO-BMFF specs
    // TODO: More constraints will be added
    if ((frame.m_FsIndex == next_frame.m_FsIndex) &&
        (frame.m_FrameRateIndex == next_frame.m_FrameRateIndex)){
        return true;
    }else {
        return false;
    }
    
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Header::Check
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Header::Check()
{
    // Bitstream version
    if (m_BitstreamVersion != 2) {
        return AP4_FAILURE;
    }
    if ((m_FsIndex == 0  && m_FrameRateIndex != 13) || (m_FsIndex == 1 &&  m_FrameRateIndex >13)){
        return AP4_FAILURE;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Header::GetPresentationVersionBySGIndex
+----------------------------------------------------------------------*/
AP4_Result 
AP4_Ac4Header::GetPresentationVersionBySGIndex(unsigned int substream_group_index) 
{
    for (unsigned int idx = 0; idx < m_NPresentations; idx++){
        for (unsigned int sg = 0; sg < m_PresentationV1[idx].d.v1.n_substream_groups; sg++){
            if (substream_group_index ==  m_PresentationV1[idx].d.v1.substream_group_indexs[sg]) {
                return m_PresentationV1[idx].presentation_version;
            }
        }
    }
    return AP4_FAILURE;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Header::GetPresentationIndexBySGIndex
+----------------------------------------------------------------------*/
AP4_Result 
AP4_Ac4Header::GetPresentationIndexBySGIndex(unsigned int substream_group_index) 
{
    for (unsigned int idx = 0; idx < m_NPresentations; idx++){
        for (unsigned int sg = 0; sg < m_PresentationV1[idx].d.v1.n_substream_groups; sg++){
            if (substream_group_index ==  m_PresentationV1[idx].d.v1.substream_group_indexs[sg]) {
                return idx;
            }
        }
    }
    return AP4_FAILURE;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::AP4_Ac4Parser
+----------------------------------------------------------------------*/
AP4_Ac4Parser::AP4_Ac4Parser() :
    m_FrameCount(0)
{
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::~AP4_Ac4Parser
+----------------------------------------------------------------------*/
AP4_Ac4Parser::~AP4_Ac4Parser()
{
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::Reset
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Parser::Reset()
{
    m_FrameCount = 0;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::Feed
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Parser::Feed(const AP4_UI08* buffer, 
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
|    AP4_Ac4Parser::FindHeader
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Parser::FindHeader(AP4_UI08* header)
{
    AP4_Size available = m_Bits.GetBytesAvailable();

    /* look for the sync pattern */
    while (available-- >= AP4_AC4_HEADER_SIZE) {
        m_Bits.PeekBytes(header, 2);

        if ((((header[0] << 8) | header[1]) == AP4_AC4_SYNC_WORD_CRC) || (((header[0] << 8) | header[1]) == AP4_AC4_SYNC_WORD)) {
            /* found a sync pattern, read the entire the header */
            m_Bits.PeekBytes(header, AP4_AC4_HEADER_SIZE);
            
           return AP4_SUCCESS;
        } else {
            m_Bits.SkipBytes(1); 
        }
    }

    return AP4_ERROR_NOT_ENOUGH_DATA;
}
/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::GetSyncFrameSize
+----------------------------------------------------------------------*/
AP4_UI32
AP4_Ac4Parser::GetSyncFrameSize(AP4_BitReader &bits)
{
    unsigned int sync_word  = bits.ReadBits(16);
    unsigned int frame_size = bits.ReadBits(16);
    unsigned int head_size  = 4;
    if (frame_size == 0xFFFF){
        frame_size = bits.ReadBits(24);
        head_size += 3; 
    }
    if (sync_word == AP4_AC4_SYNC_WORD_CRC) {
        head_size += 2;
    }
    return (head_size + frame_size); 
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::FindFrame
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Parser::FindFrame(AP4_Ac4Frame& frame)
{
    unsigned int   available;
    unsigned char  *raw_header = new unsigned char[AP4_AC4_HEADER_SIZE];
    AP4_Result     result;

    /* align to the start of the next byte */
    m_Bits.ByteAlign();
    
    /* find a frame header */
    result = FindHeader(raw_header);
    if (AP4_FAILED(result)) return result;

    // duplicated work, just to get the frame size
    AP4_BitReader tmp_bits(raw_header, AP4_AC4_HEADER_SIZE);
    unsigned int sync_frame_size = GetSyncFrameSize(tmp_bits);
    if (sync_frame_size > (AP4_BITSTREAM_BUFFER_SIZE - 1)) {
        return AP4_ERROR_NOT_ENOUGH_DATA;
    }

    delete[] raw_header;
    raw_header = new unsigned char[sync_frame_size];
    /*
     * Error handling to skip the 'fake' sync word. 
     * - the maximum sync frame size is about (AP4_BITSTREAM_BUFFER_SIZE - 1) bytes.
     */
    if (m_Bits.GetBytesAvailable() < sync_frame_size ) {
        if (m_Bits.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE - 1)) {
            // skip the sync word, assume it's 'fake' sync word
            m_Bits.SkipBytes(2);
        }
        return AP4_ERROR_NOT_ENOUGH_DATA;
    }
    // copy the whole frame becasue toc size is unknown
    m_Bits.PeekBytes(raw_header, sync_frame_size);
    /* parse the header */
    AP4_Ac4Header ac4_header(raw_header, sync_frame_size);

    // Place before goto statement to resolve Xcode compiler issue
    unsigned int bit_rate_mode = 0;

    /* check the header */
    result = ac4_header.Check();
    if (AP4_FAILED(result)) {
        m_Bits.SkipBytes(sync_frame_size);
        goto fail;
    }
    
    /* check if we have enough data to peek at the next header */
    available = m_Bits.GetBytesAvailable();
    // TODO: find the proper AP4_AC4_MAX_TOC_SIZE or just parse what this step need ?
    if (available >= ac4_header.m_FrameSize + ac4_header.m_HeaderSize + ac4_header.m_CrcSize + AP4_AC4_HEADER_SIZE + AP4_AC4_MAX_TOC_SIZE) {
        // enough to peek at the header of the next frame
        unsigned char *peek_raw_header = new unsigned char[AP4_AC4_HEADER_SIZE];

        m_Bits.SkipBytes(ac4_header.m_FrameSize + ac4_header.m_HeaderSize + ac4_header.m_CrcSize);
        m_Bits.PeekBytes(peek_raw_header, AP4_AC4_HEADER_SIZE);

        // duplicated work, just to get the frame size
        AP4_BitReader peak_tmp_bits(peek_raw_header, AP4_AC4_HEADER_SIZE);
        unsigned int peak_sync_frame_size = GetSyncFrameSize(peak_tmp_bits);

        delete[] peek_raw_header;
        peek_raw_header = new unsigned char[peak_sync_frame_size];
        // copy the whole frame becasue toc size is unknown
        if (m_Bits.GetBytesAvailable() < (peak_sync_frame_size)) {
            peak_sync_frame_size = m_Bits.GetBytesAvailable();
        }
        m_Bits.PeekBytes(peek_raw_header, peak_sync_frame_size);

        m_Bits.SkipBytes(-((int)(ac4_header.m_FrameSize + ac4_header.m_HeaderSize + ac4_header.m_CrcSize)));

        /* check the header */
        AP4_Ac4Header peek_ac4_header(peek_raw_header, peak_sync_frame_size, true);
        result = peek_ac4_header.Check();
        if (AP4_FAILED(result)) {
            // TODO: need to reserve current sync frame ?
            m_Bits.SkipBytes(sync_frame_size + peak_sync_frame_size);
            goto fail;
        }

        /* check that the fixed part of this header is the same as the */
        /* fixed part of the previous header                           */
        if (!AP4_Ac4Header::MatchFixed(ac4_header, peek_ac4_header)) {
            // TODO: need to reserve current sync frame ?
            m_Bits.SkipBytes(sync_frame_size + peak_sync_frame_size);
            goto fail;
        }
    } else if (available < (ac4_header.m_FrameSize + ac4_header.m_HeaderSize + ac4_header.m_CrcSize) || (m_Bits.m_Flags & AP4_BITSTREAM_FLAG_EOS) == 0) {
        // not enough for a frame, or not at the end (in which case we'll want to peek at the next header)
        return AP4_ERROR_NOT_ENOUGH_DATA;
    }

    m_Bits.SkipBytes(ac4_header.m_HeaderSize);

    /* fill in the frame info */
    frame.m_Info.m_HeaderSize     = ac4_header.m_HeaderSize;
    frame.m_Info.m_FrameSize      = ac4_header.m_FrameSize;
    frame.m_Info.m_CRCSize        = ac4_header.m_CrcSize;
    frame.m_Info.m_ChannelCount   = ac4_header.m_ChannelCount; 
    frame.m_Info.m_SampleDuration = (ac4_header.m_FsIndex == 0)? 2048 : AP4_Ac4SampleDeltaTable   [ac4_header.m_FrameRateIndex];
    frame.m_Info.m_MediaTimeScale = (ac4_header.m_FsIndex == 0)? 44100: AP4_Ac4MediaTimeScaleTable[ac4_header.m_FrameRateIndex];
    frame.m_Info.m_Iframe         = ac4_header.m_BIframeGlobal;
  
    /* fill the AC4 DSI info */
    frame.m_Info.m_Ac4Dsi.ac4_dsi_version        = 1;
    frame.m_Info.m_Ac4Dsi.d.v1.bitstream_version = ac4_header.m_BitstreamVersion;
    frame.m_Info.m_Ac4Dsi.d.v1.fs_index          = ac4_header.m_FsIndex;
    frame.m_Info.m_Ac4Dsi.d.v1.fs                = AP4_Ac4SamplingFrequencyTable[frame.m_Info.m_Ac4Dsi.d.v1.fs_index];
    frame.m_Info.m_Ac4Dsi.d.v1.frame_rate_index  = ac4_header.m_FrameRateIndex;
    frame.m_Info.m_Ac4Dsi.d.v1.b_program_id      = ac4_header.m_BProgramId;
    frame.m_Info.m_Ac4Dsi.d.v1.short_program_id  = ac4_header.m_ShortProgramId;
    frame.m_Info.m_Ac4Dsi.d.v1.b_uuid            = ac4_header.m_BProgramUuidPresent;
    AP4_CopyMemory(frame.m_Info.m_Ac4Dsi.d.v1.program_uuid, ac4_header.m_ProgramUuid, 16);

    // Calcuate the bit rate mode according to ETSI TS 103 190-2 V1.2.1 Annex B 
    if (ac4_header.m_WaitFrames      == 0)                                 { bit_rate_mode = 1;}
    else if (ac4_header.m_WaitFrames >= 1 && ac4_header.m_WaitFrames <= 6) { bit_rate_mode = 2;}
    else if (ac4_header.m_WaitFrames >  6)                                 { bit_rate_mode = 3;}

    frame.m_Info.m_Ac4Dsi.d.v1.ac4_bitrate_dsi.bit_rate_mode = bit_rate_mode;
    frame.m_Info.m_Ac4Dsi.d.v1.ac4_bitrate_dsi.bit_rate = 0;                    // unknown, fixed value now
    frame.m_Info.m_Ac4Dsi.d.v1.ac4_bitrate_dsi.bit_rate_precision = 0xffffffff; // unknown, fixed value now
    frame.m_Info.m_Ac4Dsi.d.v1.n_presentations = ac4_header.m_NPresentations;

    if (ac4_header.m_PresentationV1) {
        assert(ac4_header.m_PresentationV1 != NULL);
        frame.m_Info.m_Ac4Dsi.d.v1.presentations = new AP4_Dac4Atom::Ac4Dsi::PresentationV1[ac4_header.m_NPresentations];
        AP4_CopyMemory(frame.m_Info.m_Ac4Dsi.d.v1.presentations,
                       ac4_header.m_PresentationV1,
                       sizeof(AP4_Dac4Atom::Ac4Dsi::PresentationV1) * ac4_header.m_NPresentations);
        for (unsigned int pres_idx = 0; pres_idx < ac4_header.m_NPresentations; pres_idx++) {
            assert(ac4_header.m_PresentationV1[pres_idx].d.v1.substream_groups != NULL);
            int n_substream_group = ac4_header.m_PresentationV1[pres_idx].d.v1.n_substream_groups;
            frame.m_Info.m_Ac4Dsi.d.v1.presentations[pres_idx].d.v1.substream_groups = new AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1[n_substream_group];
            frame.m_Info.m_Ac4Dsi.d.v1.presentations[pres_idx].d.v1.substream_group_indexs = new AP4_UI32[n_substream_group];
            AP4_CopyMemory(frame.m_Info.m_Ac4Dsi.d.v1.presentations[pres_idx].d.v1.substream_group_indexs,
                           ac4_header.m_PresentationV1[pres_idx].d.v1.substream_group_indexs,
                           sizeof(AP4_UI32)*n_substream_group);
            AP4_CopyMemory(frame.m_Info.m_Ac4Dsi.d.v1.presentations[pres_idx].d.v1.substream_groups,
                           ac4_header.m_PresentationV1[pres_idx].d.v1.substream_groups,
                           sizeof(AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1)*n_substream_group);
            for (unsigned int sg_idx = 0; sg_idx < n_substream_group; sg_idx++) {
                assert(ac4_header.m_PresentationV1[pres_idx].d.v1.substream_groups[sg_idx].d.v1.substreams != NULL);
                int n_substreams = ac4_header.m_PresentationV1[pres_idx].d.v1.substream_groups[sg_idx].d.v1.n_substreams;
                frame.m_Info.m_Ac4Dsi.d.v1.presentations[pres_idx].d.v1.substream_groups[sg_idx].d.v1.substreams
                    = new AP4_Dac4Atom::Ac4Dsi::SubStream[n_substreams];
                AP4_CopyMemory(frame.m_Info.m_Ac4Dsi.d.v1.presentations[pres_idx].d.v1.substream_groups[sg_idx].d.v1.substreams,
                               ac4_header.m_PresentationV1[pres_idx].d.v1.substream_groups[sg_idx].d.v1.substreams,
                               sizeof(AP4_Dac4Atom::Ac4Dsi::SubStream) * n_substreams);
            }
        }
    }

    /* set the frame source */
    frame.m_Source = &m_Bits;

    return AP4_SUCCESS;

fail:
    /* skip the header and return (only skip the first byte in  */
    /* case this was a false header that hides one just after)  */
    return AP4_ERROR_CORRUPTED_BITSTREAM;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::GetBytesFree
+----------------------------------------------------------------------*/
AP4_Size
AP4_Ac4Parser::GetBytesFree()
{
  return (m_Bits.GetBytesFree());
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::GetBytesAvailable
+----------------------------------------------------------------------*/
AP4_Size  
AP4_Ac4Parser::GetBytesAvailable()
{
  return (m_Bits.GetBytesAvailable());
}
