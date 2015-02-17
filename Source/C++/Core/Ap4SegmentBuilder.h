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

#ifndef _AP4_SEGMENT_BUILDER_H_
#define _AP4_SEGMENT_BUILDER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4AvcParser.h"
#include "Ap4AdtsParser.h"
#include "Ap4List.h"
#include "Ap4Sample.h"
#include "Ap4String.h"
#include "Ap4Track.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class AP4_ByteStream;
class AP4_MpegAudioSampleDescription;

/*----------------------------------------------------------------------
|   AP4_SegmentBuilder
+---------------------------------------------------------------------*/
class AP4_SegmentBuilder
{
public:
    // constructor and destructor
    AP4_SegmentBuilder(AP4_Track::Type track_type,
                       AP4_UI32        track_id,
                       AP4_UI64        media_time_origin = 0);
    virtual ~AP4_SegmentBuilder();
    
    // accessors
    AP4_UI32               GetTrackId()        { return m_TrackId;        }
    AP4_UI32               GetTimescale()      { return m_Timescale;      }
    AP4_UI64               GetMediaStartTime() { return m_MediaStartTime; }
    AP4_UI64               GetMediaDuration()  { return m_MediaDuration;  }
    AP4_Array<AP4_Sample>& GetSamples()        { return m_Samples;        }
    
    // methods
    virtual AP4_Result AddSample(AP4_Sample& sample);
    virtual AP4_Result WriteMediaSegment(AP4_ByteStream& stream, unsigned int sequence_number);
    virtual AP4_Result WriteInitSegment(AP4_ByteStream& stream) = 0;
    
protected:
    // methods
    AP4_Track::Type       m_TrackType;
    AP4_UI32              m_TrackId;
    AP4_String            m_TrackLanguage;
    AP4_UI32              m_Timescale;
    AP4_UI64              m_SampleStartNumber;
    AP4_UI64              m_MediaTimeOrigin;
    AP4_UI64              m_MediaStartTime;
    AP4_UI64              m_MediaDuration;
    AP4_Array<AP4_Sample> m_Samples;
};

/*----------------------------------------------------------------------
|   AP4_FeedSegmentBuilder
+---------------------------------------------------------------------*/
class AP4_FeedSegmentBuilder : public AP4_SegmentBuilder
{
public:
    // constructor
    AP4_FeedSegmentBuilder(AP4_Track::Type track_type,
                           AP4_UI32        track_id,
                           AP4_UI64        media_time_origin = 0);
    
    // methods
    virtual AP4_Result Feed(const void* data,
                            AP4_Size    data_size,
                            AP4_Size&   bytes_consumed) = 0;    
};

/*----------------------------------------------------------------------
|   AP4_AvcSegmentBuilder
+---------------------------------------------------------------------*/
class AP4_AvcSegmentBuilder : public AP4_FeedSegmentBuilder
{
public:
    // constructor
    AP4_AvcSegmentBuilder(AP4_UI32 track_id,
                          double   frames_per_second,
                          AP4_UI64 media_time_origin = 0);
    
    // AP4_SegmentBuilder methods
    virtual AP4_Result WriteMediaSegment(AP4_ByteStream& stream, unsigned int sequence_number);
    virtual AP4_Result WriteInitSegment(AP4_ByteStream& stream);

    // methods
    AP4_Result Feed(const void* data,
                    AP4_Size    data_size,
                    AP4_Size&   bytes_consumed);
    
protected:
    // types
    struct SampleOrder {
        SampleOrder(AP4_UI32 decode_order, AP4_UI32 display_order) :
            m_DecodeOrder(decode_order),
            m_DisplayOrder(display_order) {}
        AP4_UI32        m_DecodeOrder;
        AP4_UI32        m_DisplayOrder;
    };
    
    // methods
    void SortSamples(SampleOrder* array, unsigned int n);

    // members
    AP4_AvcFrameParser     m_FrameParser;
    double                 m_FramesPerSecond;
    AP4_Array<SampleOrder> m_SampleOrders;
};

/*----------------------------------------------------------------------
|   AP4_AacSegmentBuilder
+---------------------------------------------------------------------*/
class AP4_AacSegmentBuilder : public AP4_FeedSegmentBuilder
{
public:
    // constructor
    AP4_AacSegmentBuilder(AP4_UI32 track_id, AP4_UI64 media_time_origin = 0);
    ~AP4_AacSegmentBuilder();
    
    // AP4_SegmentBuilder methods
    virtual AP4_Result WriteInitSegment(AP4_ByteStream& stream);

    // methods
    AP4_Result Feed(const void* data,
                    AP4_Size    data_size,
                    AP4_Size&   bytes_consumed);
    
protected:
    // members
    AP4_AdtsParser                  m_FrameParser;
    AP4_MpegAudioSampleDescription* m_SampleDescription;
};

#endif // _AP4_SEGMENT_BUILDER_H_
