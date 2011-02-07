/*****************************************************************
|
|    AP4 - File Writer
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
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

#ifndef _AP4_LINEAR_READER_H_
#define _AP4_LINEAR_READER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4Array.h"
#include "Ap4Movie.h"
#include "Ap4Sample.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class AP4_Track;
class AP4_MovieFragment;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_LINEAR_READER_INITIALIZED = 1;
const unsigned int AP4_LINEAR_READER_FLAG_EOS    = 2;

const unsigned int AP4_LINEAR_READER_DEFAULT_BUFFER_SIZE = 16*1024*1024;

/*----------------------------------------------------------------------
|   AP4_LinearReader
+---------------------------------------------------------------------*/
class AP4_LinearReader {
public:
    AP4_LinearReader(AP4_Movie& movie, AP4_ByteStream* fragment_stream = NULL, AP4_Size max_buffer=AP4_LINEAR_READER_DEFAULT_BUFFER_SIZE);
   ~AP4_LinearReader();
    
    AP4_Result EnableTrack(AP4_UI32 track_id);
    /**
     * Read the next sample in storage order, from any track.
     * track_id is updated to reflect the track from which the sample was read.
     */
    AP4_Result ReadNextSample(AP4_Sample&     sample, 
                              AP4_DataBuffer& sample_data,
                              AP4_UI32&       track_id);
    /**
     * Read the next sample in storage order from a specific track.
     */
    AP4_Result ReadNextSample(AP4_UI32        track_id,
                              AP4_Sample&     sample, 
                              AP4_DataBuffer& sample_data);
                            
    AP4_Result SetSampleIndex(AP4_UI32 track_id, AP4_UI32 sample_index);
    
    // accessors
    AP4_Size GetBufferFullness() { return m_BufferFullness; }
    
private:
    class SampleBuffer {
    public:
        SampleBuffer(AP4_Sample* sample) : m_Sample(sample) {}
       ~SampleBuffer() { delete m_Sample; } 
        AP4_Sample*    m_Sample;
        AP4_DataBuffer m_Data;
    };
    class Tracker {
    public:
        Tracker(AP4_Track* track) :
            m_Eos(false),
            m_TrackId(track->GetId()),
            m_SampleTable(NULL), 
            m_SampleTableIsOwned(false),
            m_NextSample(NULL),
            m_NextSampleIndex(0),
            m_NextDts(0) {}
        Tracker(const Tracker& other) : 
            m_Eos(other.m_Eos),
            m_TrackId(other.m_TrackId),
            m_SampleTable(other.m_SampleTable),
            m_SampleTableIsOwned(false),
            m_NextSample(NULL),
            m_NextSampleIndex(other.m_NextSampleIndex),
            m_NextDts(other.m_NextDts) {} // don't copy samples
       ~Tracker();
        bool                   m_Eos;
        AP4_UI32               m_TrackId;
        AP4_SampleTable*       m_SampleTable;
        bool                   m_SampleTableIsOwned;
        bool                   m_HasFragments;
        AP4_Sample*            m_NextSample;
        AP4_Ordinal            m_NextSampleIndex;
        AP4_UI64               m_NextDts;
        AP4_List<SampleBuffer> m_Samples;
    };
    
    // methods
    Tracker*   FindTracker(AP4_UI32 track_id);
    AP4_Result Advance();
    AP4_Result AdvanceFragment();
    AP4_Result ProcessMoof(AP4_ContainerAtom* moof, 
                           AP4_Position       moof_offset, 
                           AP4_Position       mdat_payload_offset);
    bool       PopSample(Tracker* tracker, AP4_Sample& sample, AP4_DataBuffer& sample_data);
    
    // members
    AP4_Movie&          m_Movie;
    bool                m_HasFragments;
    AP4_MovieFragment*  m_Fragment;
    AP4_ByteStream*     m_FragmentStream;
    AP4_Position        m_NextFragmentPosition;
    AP4_Array<Tracker*> m_Trackers;
    AP4_Size            m_BufferFullness;
    AP4_Size            m_BufferFullnessPeak;
    AP4_Size            m_MaxBufferFullness;
};

#endif // _AP4_LINEAR_READER_H_
