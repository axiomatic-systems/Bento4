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
#include "Ap4Protection.h"

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
    virtual ~AP4_LinearReader();
    
    AP4_Result EnableTrack(AP4_UI32 track_id);
    
    /**
     * Read the next sample in storage order, from any track.
     * track_id is updated to reflect the track from which the sample was read.
     */
    AP4_Result ReadNextSample(AP4_Sample&     sample, 
                              AP4_DataBuffer& sample_data,
                              AP4_UI32&       track_id);
                              
    /**
     * Get the next sample in storage order, without reading the sample data, 
     * from any track. track_id is updated to reflect the track from which the 
     * sample was read.
     */
    AP4_Result GetNextSample(AP4_Sample& sample, AP4_UI32& track_id);

    /**
     * Read the next sample in storage order from a specific track.
     */
    AP4_Result ReadNextSample(AP4_UI32        track_id,
                              AP4_Sample&     sample, 
                              AP4_DataBuffer& sample_data);
                            
    AP4_Result SetSampleIndex(AP4_UI32 track_id, AP4_UI32 sample_index);
    
    AP4_Result SeekTo(AP4_UI32 time_ms, AP4_UI32* actual_time_ms = 0);
    
    // accessors
    AP4_Size GetBufferFullness() { return m_BufferFullness; }
    AP4_Position GetCurrentFragmentPosition() { return m_CurrentFragmentPosition; }
    
    // classes
    class SampleReader {
    public:
        virtual ~SampleReader() {}
        virtual AP4_Result ReadSampleData(AP4_Sample& sample, AP4_DataBuffer& sample_data) = 0;
    };

protected:
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
            m_Track(track),
            m_SampleTable(NULL), 
            m_SampleTableIsOwned(false),
            m_NextSample(NULL),
            m_NextSampleIndex(0),
            m_NextDts(0),
            m_Reader(NULL) {
                m_SeekPoint.m_Pending      = false;
                m_SeekPoint.m_Time         = 0;
                m_SeekPoint.m_MoofOffset   = 0;
                m_SeekPoint.m_TrafNumber   = 0;
                m_SeekPoint.m_TrunNumber   = 0;
                m_SeekPoint.m_SampleNumber = 0;
            }
        Tracker(const Tracker& other) : 
            m_Eos(other.m_Eos),
            m_Track(other.m_Track),
            m_SampleTable(other.m_SampleTable),
            m_SampleTableIsOwned(false),
            m_NextSample(NULL),
            m_NextSampleIndex(other.m_NextSampleIndex),
            m_NextDts(other.m_NextDts),
            m_Reader(other.m_Reader) {
                m_SeekPoint = other.m_SeekPoint;
            } // don't copy samples
       ~Tracker();
        bool                   m_Eos;
        AP4_Track*             m_Track;
        AP4_SampleTable*       m_SampleTable;
        bool                   m_SampleTableIsOwned;
        AP4_Sample*            m_NextSample;
        AP4_Ordinal            m_NextSampleIndex;
        AP4_UI64               m_NextDts;
        AP4_List<SampleBuffer> m_Samples;
        SampleReader*          m_Reader;
        struct {
            bool         m_Pending;
            AP4_UI64     m_Time;
            AP4_Position m_MoofOffset;
            unsigned int m_TrafNumber;
            unsigned int m_TrunNumber;
            unsigned int m_SampleNumber;
        } m_SeekPoint;
    };
    
    // methods that can be overridden
    virtual AP4_Result ProcessTrack(AP4_Track* track);
    virtual AP4_Result ProcessMoof(AP4_ContainerAtom* moof, 
                                   AP4_Position       moof_offset, 
                                   AP4_Position       mdat_payload_offset);
    
    // methods
    Tracker*   FindTracker(AP4_UI32 track_id);
    AP4_Result Advance(bool read_data = true);
    AP4_Result AdvanceFragment();
    bool       PopSample(Tracker* tracker, AP4_Sample& sample, AP4_DataBuffer* sample_data);
    AP4_Result ReadNextSample(AP4_Sample&     sample, 
                              AP4_DataBuffer* sample_data,
                              AP4_UI32&       track_id);
    void       FlushQueue(Tracker* tracker);
    void       FlushQueues();
    
    // members
    AP4_Movie&          m_Movie;
    bool                m_HasFragments;
    AP4_MovieFragment*  m_Fragment;
    AP4_ByteStream*     m_FragmentStream;
    AP4_Position        m_CurrentFragmentPosition;
    AP4_Position        m_NextFragmentPosition;
    AP4_Array<Tracker*> m_Trackers;
    AP4_Size            m_BufferFullness;
    AP4_Size            m_BufferFullnessPeak;
    AP4_Size            m_MaxBufferFullness;
    AP4_ContainerAtom*  m_Mfra;
};

/*----------------------------------------------------------------------
|   AP4_DecryptingSampleReader
+---------------------------------------------------------------------*/
class AP4_DecryptingSampleReader : public AP4_LinearReader::SampleReader
{
public:
    AP4_DecryptingSampleReader(AP4_SampleDecrypter* decrypter, bool transfer_ownership) :
        m_DecrypterIsOwned(transfer_ownership),
        m_Decrypter(decrypter) {}
    virtual ~AP4_DecryptingSampleReader() { 
        if (m_DecrypterIsOwned) delete m_Decrypter; 
    }
    virtual AP4_Result ReadSampleData(AP4_Sample& sample, AP4_DataBuffer& sample_data);
    
    bool                 m_DecrypterIsOwned;
    AP4_DataBuffer       m_DataBuffer;
    AP4_SampleDecrypter* m_Decrypter;
};


#endif // _AP4_LINEAR_READER_H_
