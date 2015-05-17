/*****************************************************************
|
|    AP4 - Segment Builder
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
#include <stdio.h> // FIXME: testing
#include "Ap4SegmentBuilder.h"
#include "Ap4Results.h"
#include "Ap4ByteStream.h"
#include "Ap4Movie.h"
#include "Ap4MehdAtom.h"
#include "Ap4SyntheticSampleTable.h"
#include "Ap4TrexAtom.h"
#include "Ap4FtypAtom.h"
#include "Ap4File.h"
#include "Ap4TfhdAtom.h"
#include "Ap4MfhdAtom.h"
#include "Ap4TrunAtom.h"
#include "Ap4TfdtAtom.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_SEGMENT_BUILDER_DEFAULT_TIMESCALE = 1000;

/*----------------------------------------------------------------------
|   AP4_SegmentBuilder::AP4_SegmentBuilder
+---------------------------------------------------------------------*/
AP4_SegmentBuilder::AP4_SegmentBuilder(AP4_Track::Type track_type,
                                       AP4_UI32        track_id,
                                       AP4_UI64        media_time_origin) :
    m_TrackType(track_type),
    m_TrackId(track_id),
    m_TrackLanguage("und"),
    m_Timescale(AP4_SEGMENT_BUILDER_DEFAULT_TIMESCALE),
    m_SampleStartNumber(0),
    m_MediaTimeOrigin(media_time_origin),
    m_MediaStartTime(0),
    m_MediaDuration(0)
{
}

/*----------------------------------------------------------------------
|   AP4_SegmentBuilder::~AP4_SegmentBuilder
+---------------------------------------------------------------------*/
AP4_SegmentBuilder::~AP4_SegmentBuilder()
{
    m_Samples.Clear();
}

/*----------------------------------------------------------------------
|   AP4_SegmentBuilder::AddSample
+---------------------------------------------------------------------*/
AP4_Result
AP4_SegmentBuilder::AddSample(AP4_Sample& sample)
{
    AP4_Result result = m_Samples.Append(sample);
    if (AP4_FAILED(result)) return result;
    m_MediaDuration += sample.GetDuration();
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_FeedSegmentBuilder::AP4_FeedSegmentBuilder
+---------------------------------------------------------------------*/
AP4_FeedSegmentBuilder::AP4_FeedSegmentBuilder(AP4_Track::Type track_type,
                                               AP4_UI32        track_id,
                                               AP4_UI64        media_time_origin) :
    AP4_SegmentBuilder(track_type, track_id, media_time_origin)
{
}

/*----------------------------------------------------------------------
|   AP4_AvcSegmentBuilder::AP4_AvcSegmentBuilder
+---------------------------------------------------------------------*/
AP4_AvcSegmentBuilder::AP4_AvcSegmentBuilder(AP4_UI32 track_id,
                                             double   frames_per_second,
                                             AP4_UI64 media_time_origin) :
    AP4_FeedSegmentBuilder(AP4_Track::TYPE_VIDEO, track_id, media_time_origin),
    m_FramesPerSecond(frames_per_second)
{
    m_Timescale = (unsigned int)(frames_per_second*1000.0);
}

/*----------------------------------------------------------------------
|   AP4_SegmentBuilder::WriteMediaSegment
+---------------------------------------------------------------------*/
AP4_Result
AP4_SegmentBuilder::WriteMediaSegment(AP4_ByteStream& stream, unsigned int sequence_number)
{
    unsigned int tfhd_flags = AP4_TFHD_FLAG_DEFAULT_BASE_IS_MOOF;
    if (m_TrackType == AP4_Track::TYPE_VIDEO) {
        tfhd_flags |= AP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT;
    }
        
    // setup the moof structure
    AP4_ContainerAtom* moof = new AP4_ContainerAtom(AP4_ATOM_TYPE_MOOF);
    AP4_MfhdAtom* mfhd = new AP4_MfhdAtom(sequence_number);
    moof->AddChild(mfhd);
    AP4_ContainerAtom* traf = new AP4_ContainerAtom(AP4_ATOM_TYPE_TRAF);
    AP4_TfhdAtom* tfhd = new AP4_TfhdAtom(tfhd_flags,
                                          m_TrackId,
                                          0,
                                          1,
                                          0,
                                          0,
                                          0);
    if (tfhd_flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT) {
        tfhd->SetDefaultSampleFlags(0x1010000); // sample_is_non_sync_sample=1, sample_depends_on=1 (not I frame)
    }
    
    traf->AddChild(tfhd);
    AP4_TfdtAtom* tfdt = new AP4_TfdtAtom(1, m_MediaTimeOrigin+m_MediaStartTime);
    traf->AddChild(tfdt);
    AP4_UI32 trun_flags = AP4_TRUN_FLAG_DATA_OFFSET_PRESENT     |
                          AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT |
                          AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT;
    AP4_UI32 first_sample_flags = 0;
    if (m_TrackType == AP4_Track::TYPE_VIDEO) {
        trun_flags |= AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT;
        first_sample_flags = 0x2000000; // sample_depends_on=2 (I frame)
    }
    AP4_TrunAtom* trun = new AP4_TrunAtom(trun_flags, 0, first_sample_flags);
    
    traf->AddChild(trun);
    moof->AddChild(traf);
    
    // add samples to the fragment
    AP4_Array<AP4_UI32>            sample_indexes;
    AP4_Array<AP4_TrunAtom::Entry> trun_entries;
    AP4_UI32                       mdat_size = AP4_ATOM_HEADER_SIZE;
    trun_entries.SetItemCount(m_Samples.ItemCount());
    for (unsigned int i=0; i<m_Samples.ItemCount(); i++) {
        // if we have one non-zero CTS delta, we'll need to express it
        if (m_Samples[i].GetCtsDelta()) {
            trun->SetFlags(trun->GetFlags() | AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT);
        }
        
        // add one sample
        AP4_TrunAtom::Entry& trun_entry = trun_entries[i];
        trun_entry.sample_duration                = m_Samples[i].GetDuration();
        trun_entry.sample_size                    = m_Samples[i].GetSize();
        trun_entry.sample_composition_time_offset = m_Samples[i].GetCtsDelta();
                    
        mdat_size += trun_entry.sample_size;
    }
    
    // update moof and children
    trun->SetEntries(trun_entries);
    trun->SetDataOffset((AP4_UI32)moof->GetSize()+AP4_ATOM_HEADER_SIZE);
    
    // write moof
    moof->Write(stream);
    
    // write mdat
    stream.WriteUI32(mdat_size);
    stream.WriteUI32(AP4_ATOM_TYPE_MDAT);
    for (unsigned int i=0; i<m_Samples.ItemCount(); i++) {
        AP4_Result result;
        AP4_ByteStream* data_stream = m_Samples[i].GetDataStream();
        result = data_stream->Seek(m_Samples[i].GetOffset());
        if (AP4_FAILED(result)) {
            data_stream->Release();
            return result;
        }
        result = data_stream->CopyTo(stream, m_Samples[i].GetSize());
        if (AP4_FAILED(result)) {
            data_stream->Release();
            return result;
        }
        
        data_stream->Release();
    }
    
    // update counters
    m_SampleStartNumber += m_Samples.ItemCount();
    m_MediaStartTime    += m_MediaDuration;
    m_MediaDuration      = 0;
    
    // cleanup
    delete moof;
    m_Samples.Clear();

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AvcSegmentBuilder::Feed
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcSegmentBuilder::Feed(const void* data,
                            AP4_Size    data_size,
                            AP4_Size&   bytes_consumed)
{
    AP4_Result result;
    
    AP4_AvcFrameParser::AccessUnitInfo access_unit_info;
    result = m_FrameParser.Feed(data, data_size, bytes_consumed, access_unit_info, data == NULL);
    if (AP4_FAILED(result)) return result;
    
    // check if we have an access unit
    if (access_unit_info.nal_units.ItemCount()) {
        // compute the total size of the sample data
        unsigned int sample_data_size = 0;
        for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
            sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
        }
        
        // format the sample data
        AP4_MemoryByteStream* sample_data = new AP4_MemoryByteStream(sample_data_size);
        for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
            sample_data->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
            sample_data->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
        }
        
        // compute the timestamp in a drift-less manner
        AP4_UI32 duration = 0;
        AP4_UI64 dts      = 0;
        if (m_Timescale !=0 && m_FramesPerSecond != 0.0) {
            AP4_UI64 this_sample_time = m_MediaStartTime+m_MediaDuration;
            AP4_UI64 next_sample_time = (AP4_UI64)((double)m_Timescale*(double)(m_SampleStartNumber+m_Samples.ItemCount()+1)/m_FramesPerSecond);
            duration = (AP4_UI32)(next_sample_time-this_sample_time);
            dts      = (AP4_UI64)((double)m_Timescale/m_FramesPerSecond*(double)m_Samples.ItemCount());
        }

        // create a new sample and add it to the list
        AP4_Sample sample(*sample_data, 0, sample_data_size, duration, 0, dts, 0, access_unit_info.is_idr);
        AddSample(sample);
        sample_data->Release();
        
        // remember the sample order
        m_SampleOrders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));
        
        // free the memory buffers
        for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
            delete access_unit_info.nal_units[i];
        }
        access_unit_info.nal_units.Clear();
        
        return 1; // one access unit returned
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AvcSegmentBuilder::SortSamples
+---------------------------------------------------------------------*/
void
AP4_AvcSegmentBuilder::SortSamples(SampleOrder* array, unsigned int n)
{
    if (n < 2) {
        return;
    }
    SampleOrder pivot = array[n / 2];
    SampleOrder* left  = array;
    SampleOrder* right = array + n - 1;
    while (left <= right) {
        if (left->m_DisplayOrder < pivot.m_DisplayOrder) {
            ++left;
            continue;
        }
        if (right->m_DisplayOrder > pivot.m_DisplayOrder) {
            --right;
            continue;
        }
        SampleOrder temp = *left;
        *left++ = *right;
        *right-- = temp;
    }
    SortSamples(array, (unsigned int)(right - array + 1));
    SortSamples(left, (unsigned int)(array + n - left));
}

/*----------------------------------------------------------------------
|   AP4_AvcSegmentBuilder::WriteMediaSegment
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcSegmentBuilder::WriteMediaSegment(AP4_ByteStream& stream, unsigned int sequence_number)
{
    if (m_SampleOrders.ItemCount() > 1) {
        // rebase the decode order
        AP4_UI32 decode_order_base = m_SampleOrders[0].m_DecodeOrder;
        for (unsigned int i=0; i<m_SampleOrders.ItemCount(); i++) {
            if (m_SampleOrders[i].m_DecodeOrder >= decode_order_base) {
                m_SampleOrders[i].m_DecodeOrder -= decode_order_base;
            }
        }
    
        // adjust the sample CTS/DTS offsets based on the sample orders
        unsigned int start = 0;
        for (unsigned int i=1; i<=m_SampleOrders.ItemCount(); i++) {
            if (i == m_SampleOrders.ItemCount() || m_SampleOrders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&m_SampleOrders[start], i-start);
                start = i;
            }
        }

        // compute the max CTS delta
        unsigned int max_delta = 0;
        for (unsigned int i=0; i<m_SampleOrders.ItemCount(); i++) {
            if (m_SampleOrders[i].m_DecodeOrder > i) {
                unsigned int delta =m_SampleOrders[i].m_DecodeOrder-i;
                if (delta > max_delta) {
                    max_delta = delta;
                }
            }
        }

        // set the CTS for all samples
        for (unsigned int i=0; i<m_SampleOrders.ItemCount(); i++) {
            AP4_UI64 dts = m_Samples[i].GetDts();
            if (m_Timescale) {
                dts = (AP4_UI64)((double)m_Timescale/m_FramesPerSecond*(double)(i+max_delta));
            }
            if (m_SampleOrders[i].m_DecodeOrder < m_Samples.ItemCount()) {
                m_Samples[m_SampleOrders[i].m_DecodeOrder].SetCts(dts);
            }
        }
    
        m_SampleOrders.Clear();
    }
    
    return AP4_SegmentBuilder::WriteMediaSegment(stream, sequence_number);
}

/*----------------------------------------------------------------------
|   AP4_AvcSegmentBuilder::WriteInitSegment
+---------------------------------------------------------------------*/
AP4_Result
AP4_AvcSegmentBuilder::WriteInitSegment(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // compute the track parameters
    AP4_AvcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (m_FrameParser.GetSequenceParameterSets()[i]) {
            sps = m_FrameParser.GetSequenceParameterSets()[i];
            break;
        }
    }
    if (sps == NULL) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    unsigned int video_width = 0;
    unsigned int video_height = 0;
    sps->GetInfo(video_width, video_height);
    
    // collect the SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (m_FrameParser.GetSequenceParameterSets()[i]) {
            sps_array.Append(m_FrameParser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_AVC_PPS_MAX_ID; i++) {
        if (m_FrameParser.GetPictureParameterSets()[i]) {
            pps_array.Append(m_FrameParser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }
    
    // setup the video the sample descripton
    AP4_AvcSampleDescription* sample_description =
        new AP4_AvcSampleDescription(AP4_SAMPLE_FORMAT_AVC1,
                                     (AP4_UI16)video_width,
                                     (AP4_UI16)video_height,
                                     24,
                                     "h264",
                                     (AP4_UI08)sps->profile_idc,
                                     (AP4_UI08)sps->level_idc,
                                     (AP4_UI08)(sps->constraint_set0_flag<<7 |
                                                sps->constraint_set1_flag<<6 |
                                                sps->constraint_set2_flag<<5 |
                                                sps->constraint_set3_flag<<4),
                                     4,
                                     sps_array,
                                     pps_array);
    
    // create the output file object
    AP4_Movie* output_movie = new AP4_Movie(AP4_SEGMENT_BUILDER_DEFAULT_TIMESCALE);
    
    // create an mvex container
    AP4_ContainerAtom* mvex = new AP4_ContainerAtom(AP4_ATOM_TYPE_MVEX);
    AP4_MehdAtom* mehd = new AP4_MehdAtom(0);
    mvex->AddChild(mehd);
    
    // create a sample table (with no samples) to hold the sample description
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();
    sample_table->AddSampleDescription(sample_description, true);
    
    // create the track
    AP4_Track* output_track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                            sample_table,
                                            m_TrackId,
                                            AP4_SEGMENT_BUILDER_DEFAULT_TIMESCALE,
                                            0,
                                            m_Timescale,
                                            0,
                                            m_TrackLanguage.GetChars(),
                                            video_width << 16,
                                            video_height << 16);
    output_movie->AddTrack(output_track);
    
    // add a trex entry to the mvex container
    AP4_TrexAtom* trex = new AP4_TrexAtom(m_TrackId,
                                          1,
                                          0,
                                          0,
                                          0);
    mvex->AddChild(trex);
    
    // update the mehd duration
    // TBD mehd->SetDuration(0);
    
    // the mvex container to the moov container
    output_movie->GetMoovAtom()->AddChild(mvex);
    
    // write the ftyp atom
    AP4_Array<AP4_UI32> brands;
    brands.Append(AP4_FILE_BRAND_ISOM);
    brands.Append(AP4_FILE_BRAND_MP42);
    brands.Append(AP4_FILE_BRAND_MP41);
    
    AP4_FtypAtom* ftyp = new AP4_FtypAtom(AP4_FILE_BRAND_MP42, 1, &brands[0], brands.ItemCount());
    ftyp->Write(stream);
    delete ftyp;
    
    // write the moov atom
    result = output_movie->GetMoovAtom()->Write(stream);
    if (AP4_FAILED(result)) {
        return result;
    }
    
    // cleanup
    delete output_movie;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AacSegmentBuilder::AP4_AacSegmentBuilder
+---------------------------------------------------------------------*/
AP4_AacSegmentBuilder::AP4_AacSegmentBuilder(AP4_UI32 track_id, AP4_UI64 media_time_origin) :
    AP4_FeedSegmentBuilder(AP4_Track::TYPE_AUDIO, track_id, media_time_origin),
    m_SampleDescription(NULL)
{
    m_Timescale = 0; // we will compute the real value when we get the first audio frame
}

/*----------------------------------------------------------------------
|   AP4_AacSegmentBuilder::~AP4_AacSegmentBuilder
+---------------------------------------------------------------------*/
AP4_AacSegmentBuilder::~AP4_AacSegmentBuilder()
{
    delete m_SampleDescription;
}

/*----------------------------------------------------------------------
|   AP4_AacSegmentBuilder::Feed
+---------------------------------------------------------------------*/
AP4_Result
AP4_AacSegmentBuilder::Feed(const void* data,
                            AP4_Size    data_size,
                            AP4_Size&   bytes_consumed)
{
    bytes_consumed = 0;
    
    // try to get a frame
    AP4_AacFrame frame;
    AP4_Result result = m_FrameParser.FindFrame(frame);
    if (AP4_SUCCEEDED(result)) {
        if (!m_SampleDescription) {
            // create a sample description for our samples
            AP4_DataBuffer dsi;
            unsigned char aac_dsi[2];

            unsigned int object_type = 2; // AAC LC by default
            aac_dsi[0] = (AP4_UI08)((object_type<<3) | (frame.m_Info.m_SamplingFrequencyIndex>>1));
            aac_dsi[1] = (AP4_UI08)(((frame.m_Info.m_SamplingFrequencyIndex&1)<<7) | (frame.m_Info.m_ChannelConfiguration<<3));

            dsi.SetData(aac_dsi, 2);
            m_SampleDescription =
                new AP4_MpegAudioSampleDescription(
                AP4_OTI_MPEG4_AUDIO,   // object type
                (AP4_UI32)frame.m_Info.m_SamplingFrequency,
                16,                    // sample size
                (AP4_UI16)frame.m_Info.m_ChannelConfiguration,
                &dsi,                  // decoder info
                6144,                  // buffer size
                128000,                // max bitrate
                128000);               // average bitrate
            
            m_Timescale = (AP4_UI32)frame.m_Info.m_SamplingFrequency;
        }

        // read and store the sample data
        AP4_DataBuffer sample_data(frame.m_Info.m_FrameLength);
        sample_data.SetDataSize(frame.m_Info.m_FrameLength);
        frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameLength);
        AP4_MemoryByteStream* sample_data_stream = new AP4_MemoryByteStream(frame.m_Info.m_FrameLength);
        sample_data_stream->Write(sample_data.GetData(), sample_data.GetDataSize());

        // add the sample to the table
        AP4_Sample sample(*sample_data_stream, 0, frame.m_Info.m_FrameLength, 1024, 0, 0, 0, true);
        AddSample(sample);
        sample_data_stream->Release();
        
        return 1;
    }
    
    // not enough data in the parser for a frame, feed some more
    if (data == NULL) {
        m_FrameParser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
    } else {
        AP4_Size can_feed = m_FrameParser.GetBytesFree();
        if (can_feed > data_size) {
            can_feed = data_size;
        }
        result = m_FrameParser.Feed((const AP4_UI08*)data, &can_feed);
        if (AP4_SUCCEEDED(result)) {
            bytes_consumed += can_feed;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AacSegmentBuilder::WriteInitSegment
+---------------------------------------------------------------------*/
AP4_Result
AP4_AacSegmentBuilder::WriteInitSegment(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // check that we have a sample description
    if (!m_SampleDescription) {
        return AP4_ERROR_INVALID_STATE;
    }
    
    // create the output file object
    AP4_Movie* output_movie = new AP4_Movie(AP4_SEGMENT_BUILDER_DEFAULT_TIMESCALE);
    
    // create an mvex container
    AP4_ContainerAtom* mvex = new AP4_ContainerAtom(AP4_ATOM_TYPE_MVEX);
    AP4_MehdAtom* mehd = new AP4_MehdAtom(0);
    mvex->AddChild(mehd);
    
    // create a sample table (with no samples) to hold the sample description
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();
    sample_table->AddSampleDescription(m_SampleDescription, false);
    
    // create the track
    AP4_Track* output_track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                            sample_table,
                                            m_TrackId,
                                            AP4_SEGMENT_BUILDER_DEFAULT_TIMESCALE,
                                            0,
                                            m_Timescale,
                                            0,
                                            m_TrackLanguage.GetChars(),
                                            0,
                                            0);
    output_movie->AddTrack(output_track);
    
    // add a trex entry to the mvex container
    AP4_TrexAtom* trex = new AP4_TrexAtom(m_TrackId,
                                          1,
                                          0,
                                          0,
                                          0);
    mvex->AddChild(trex);
    
    // the mvex container to the moov container
    output_movie->GetMoovAtom()->AddChild(mvex);
    
    // write the ftyp atom
    AP4_Array<AP4_UI32> brands;
    brands.Append(AP4_FILE_BRAND_ISOM);
    brands.Append(AP4_FILE_BRAND_MP42);
    brands.Append(AP4_FILE_BRAND_MP41);
    
    AP4_FtypAtom* ftyp = new AP4_FtypAtom(AP4_FILE_BRAND_MP42, 1, &brands[0], brands.ItemCount());
    ftyp->Write(stream);
    delete ftyp;
    
    // write the moov atom
    result = output_movie->GetMoovAtom()->Write(stream);
    
    // cleanup
    delete output_movie;
    
    return result;
}
