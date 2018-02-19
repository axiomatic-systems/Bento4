/*****************************************************************
|
|    AP4 - MP4 Fragmenter
|
|    Copyright 2002-2015 Axiomatic Systems, LLC
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
#include <stdio.h>
#include <stdlib.h>

#include "Ap4.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 Fragmenter - Version 1.6.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2015 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_FRAGMENTER_DEFAULT_FRAGMENT_DURATION   = 2000; // ms
const unsigned int AP4_FRAGMENTER_MAX_AUTO_FRAGMENT_DURATION  = 40000;
const unsigned int AP4_FRAGMENTER_OUTPUT_MOVIE_TIMESCALE      = 1000;

typedef enum {
    AP4_FRAGMENTER_FORCE_SYNC_MODE_NONE,
    AP4_FRAGMENTER_FORCE_SYNC_MODE_AUTO,
    AP4_FRAGMENTER_FORCE_SYNC_MODE_ALL
} ForceSyncMode;

/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
struct _Options {
    unsigned int  verbosity;
    bool          trim;
    bool          debug;
    bool          no_tfdt;
    double        tfdt_start;
    unsigned int  sequence_number_start;
    ForceSyncMode force_i_frame_sync;
} Options;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4fragment [options] <input> <output>\n"
            "options are:\n"
            "  --verbosity <n> sets the verbosity (details) level to <n> (between 0 and 3)\n"
            "  --debug enable debugging information output\n"
            "  --quiet don't print out notice messages\n"
            "  --fragment-duration <milliseconds> (default = automatic)\n"
            "  --timescale <n> (use 10000000 for Smooth Streaming compatibility)\n"
            "  --track <track-id or type> only include media from one track (pass a track ID, 'audio', 'video' or 'subtitles')\n"
            "  --index (re)create the segment index\n"
            "  --trim trim excess media in longer tracks\n"
            "  --no-tfdt don't add 'tfdt' boxes in the fragments (may be needed for legacy Smooth Streaming clients)\n"
            "  --tfdt-start <start> Value of the first tfdt timestamp, expressed either as a floating point number in seconds)\n"
            "  --sequence-number-start <start> Value of the first segment sequence number (default: 1)\n"
            "  --force-i-frame-sync <auto|all> treat all I-frames as sync samples (for open-gop sequences)\n"
            "    'auto' only forces the flag if an open-gop source is detected, 'all' forces the flag in all cases\n"
            "  --copy-udta copy the moov/udta atom from input to output\n"
            );
    exit(1);
}

/*----------------------------------------------------------------------
|   platform adaptation
+---------------------------------------------------------------------*/
#if defined(_MSC_VER)
static double 
strtof(char* s, char** /* end */)
{
	_CRT_DOUBLE value = {0.0};
    int result = _atodbl(&value, s);
	return result == 0 ? (double)value.x : 0.0;
}
#endif



/*----------------------------------------------------------------------
|   SampleArray
+---------------------------------------------------------------------*/
class SampleArray {
public:
    SampleArray(AP4_Track* track) :
        m_Track(track) {
        m_SampleCount = m_Track->GetSampleCount();
        if (m_SampleCount) {
            m_ForcedSync = new bool[m_SampleCount];
            for (unsigned int i=0; i<m_SampleCount; i++) {
                m_ForcedSync[i] = false;
            }
        } else {
            m_ForcedSync = NULL;
        }
    }
    virtual ~SampleArray() {
        delete[] m_ForcedSync;
    }

    virtual AP4_Cardinal GetSampleCount() {
        return m_SampleCount;
    }
    virtual AP4_Result GetSample(AP4_Ordinal index, AP4_Sample& sample) {
        AP4_Result result = m_Track->GetSample(index, sample);
        if (AP4_SUCCEEDED(result)) {
            if (m_ForcedSync[index]) {
                sample.SetSync(true);
            }
        }
        return result;
    }
    virtual AP4_Result AddSample(AP4_Sample& /*sample*/) {
        return AP4_ERROR_NOT_SUPPORTED;
    }
    virtual void ForceSync(AP4_Ordinal index) {
        if (index < m_SampleCount) {
            m_ForcedSync[index] = true;
        }
    }
    
protected:
    AP4_Track*   m_Track;
    AP4_Cardinal m_SampleCount;
    bool*        m_ForcedSync;
};

/*----------------------------------------------------------------------
|   CachedSampleArray
+---------------------------------------------------------------------*/
class CachedSampleArray : public SampleArray {
public:
    CachedSampleArray(AP4_Track* track) :
        SampleArray(track) {}

    virtual AP4_Cardinal GetSampleCount() {
        return m_Samples.ItemCount();
    }
    virtual AP4_Result GetSample(AP4_Ordinal index, AP4_Sample& sample) {
        if (index >= m_Samples.ItemCount()) {
            return AP4_ERROR_OUT_OF_RANGE;
        } else {
            sample = m_Samples[index];
            return AP4_SUCCESS;
        }
    }
    virtual AP4_Result AddSample(AP4_Sample& sample) {
        return m_Samples.Append(sample);
    }
    
protected:
    AP4_Array<AP4_Sample> m_Samples;
};

/*----------------------------------------------------------------------
|   TrackCursor
+---------------------------------------------------------------------*/
class TrackCursor
{
public:
    TrackCursor(AP4_Track* track, SampleArray* samples);
    ~TrackCursor();
    
    AP4_Result    Init();
    AP4_Result    SetSampleIndex(AP4_Ordinal sample_index);
    
    AP4_Track*    m_Track;
    SampleArray*  m_Samples;
    AP4_Ordinal   m_SampleIndex;
    AP4_Ordinal   m_FragmentIndex;
    AP4_Sample    m_Sample;
    AP4_UI64      m_Timestamp;
    AP4_UI64      m_UnscaledTimestamp;
    bool          m_Eos;
    AP4_TfraAtom* m_Tfra;
};

/*----------------------------------------------------------------------
|   TrackCursor::TrackCursor
+---------------------------------------------------------------------*/
TrackCursor::TrackCursor(AP4_Track* track, SampleArray* samples) :
    m_Track(track),
    m_Samples(samples),
    m_SampleIndex(0),
    m_FragmentIndex(0),
    m_Timestamp(0),
    m_UnscaledTimestamp(0),
    m_Eos(false),
    m_Tfra(new AP4_TfraAtom(0))
{
}

/*----------------------------------------------------------------------
|   TrackCursor::~TrackCursor
+---------------------------------------------------------------------*/
TrackCursor::~TrackCursor()
{
    delete m_Tfra;
    delete m_Samples;
}

/*----------------------------------------------------------------------
|   TrackCursor::Init
+---------------------------------------------------------------------*/
AP4_Result
TrackCursor::Init()
{
    return m_Samples->GetSample(0, m_Sample);
}

/*----------------------------------------------------------------------
|   TrackCursor::SetSampleIndex
+---------------------------------------------------------------------*/
AP4_Result
TrackCursor::SetSampleIndex(AP4_Ordinal sample_index)
{
    m_SampleIndex = sample_index;
    
    // check if we're at the end
    if (sample_index >= m_Samples->GetSampleCount()) {
        AP4_UI64 end_dts = m_Sample.GetDts()+m_Sample.GetDuration();
        m_Sample.Reset();
        m_Sample.SetDts(end_dts);
        m_Eos = true;
    } else {
        return m_Samples->GetSample(m_SampleIndex, m_Sample);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   FragmentInfo
+---------------------------------------------------------------------*/
class FragmentInfo {
public:
    FragmentInfo(SampleArray* samples, AP4_TfraAtom* tfra, AP4_UI64 timestamp, AP4_ContainerAtom* moof) :
        m_Samples(samples),
        m_Tfra(tfra),
        m_Timestamp(timestamp),
        m_Duration(0),
        m_Moof(moof),
        m_MoofPosition(0),
        m_MdatSize(0) {}
    
    SampleArray*        m_Samples;
    AP4_TfraAtom*       m_Tfra;
    AP4_UI64            m_Timestamp;
    AP4_UI32            m_Duration;
    AP4_Array<AP4_UI32> m_SampleIndexes;
    AP4_ContainerAtom*  m_Moof;
    AP4_Position        m_MoofPosition;
    AP4_UI32            m_MdatSize;
};

/*----------------------------------------------------------------------
|   Fragment
+---------------------------------------------------------------------*/
static void
Fragment(AP4_File&                input_file,
         AP4_ByteStream&          output_stream,
         AP4_Array<TrackCursor*>& cursors,
         unsigned int             fragment_duration,
         AP4_UI32                 timescale,
         AP4_UI32                 track_id,
         bool                     create_segment_index,
         bool                     copy_udta)
{
    AP4_List<FragmentInfo> fragments;
    TrackCursor*           index_cursor = NULL;
    AP4_Result             result;
    
    AP4_Movie* input_movie = input_file.GetMovie();
    if (input_movie == NULL) {
        fprintf(stderr, "ERROR: no moov found in the input file\n");
        return;
    }

    // create the output file object
    AP4_Movie* output_movie = new AP4_Movie(AP4_FRAGMENTER_OUTPUT_MOVIE_TIMESCALE);
    
    // create an mvex container
    AP4_ContainerAtom* mvex = new AP4_ContainerAtom(AP4_ATOM_TYPE_MVEX);
    AP4_MehdAtom*      mehd = new AP4_MehdAtom(0);
    mvex->AddChild(mehd);
    
    // add an output track for each track in the input file
    for (unsigned int i=0; i<cursors.ItemCount(); i++) {
        AP4_Track* track = cursors[i]->m_Track;
        
        // skip non matching tracks if we have a selector
        if (track_id && track->GetId() != track_id) {
            continue;
        }
        
        result = cursors[i]->Init();
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: failed to init sample cursor (%d), skipping track %d\n", result, track->GetId());
            return;
        }

        // create a sample table (with no samples) to hold the sample description
        AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();
        for (unsigned int j=0; j<track->GetSampleDescriptionCount(); j++) {
            AP4_SampleDescription* sample_description = track->GetSampleDescription(j);
            sample_table->AddSampleDescription(sample_description, false);
        }
        
        // create the track
        AP4_Track* output_track = new AP4_Track(sample_table,
                                                track->GetId(),
                                                timescale?timescale:AP4_FRAGMENTER_OUTPUT_MOVIE_TIMESCALE,
                                                AP4_ConvertTime(track->GetDuration(),
                                                                input_movie->GetTimeScale(),
                                                                timescale?timescale:AP4_FRAGMENTER_OUTPUT_MOVIE_TIMESCALE),
                                                timescale?timescale:track->GetMediaTimeScale(),
                                                0,//track->GetMediaDuration(),
                                                track);
        
        // add an edit list if needed
        if (const AP4_TrakAtom* trak = track->GetTrakAtom()) {
            AP4_ContainerAtom* edts = AP4_DYNAMIC_CAST(AP4_ContainerAtom, trak->GetChild(AP4_ATOM_TYPE_EDTS));
            if (edts) {
                // create an 'edts' container
                AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
                
                // create a new 'edts' for each original 'edts'
                for (AP4_List<AP4_Atom>::Item* edts_entry = edts->GetChildren().FirstItem();
                     edts_entry;
                     edts_entry = edts_entry->GetNext()) {
                    AP4_ElstAtom* elst = AP4_DYNAMIC_CAST(AP4_ElstAtom, edts_entry->GetData());
                    AP4_ElstAtom* new_elst = new AP4_ElstAtom();
                    
                    // adjust the fields to match the correct timescale
                    for (unsigned int j=0; j<elst->GetEntries().ItemCount(); j++) {
                        AP4_ElstEntry new_elst_entry = elst->GetEntries()[j];
                        new_elst_entry.m_SegmentDuration = AP4_ConvertTime(new_elst_entry.m_SegmentDuration,
                                                                           input_movie->GetTimeScale(),
                                                                           AP4_FRAGMENTER_OUTPUT_MOVIE_TIMESCALE);
                        if (new_elst_entry.m_MediaTime > 0 && timescale) {
                            new_elst_entry.m_MediaTime = (AP4_SI64)AP4_ConvertTime(new_elst_entry.m_MediaTime,
                                                                                   track->GetMediaTimeScale(),
                                                                                   timescale?timescale:track->GetMediaTimeScale());
                                                                               
                        }
                        new_elst->AddEntry(new_elst_entry);
                    }
                    
                    // add the 'elst' to the 'edts' container
                    new_edts->AddChild(new_elst);
                }
                
                // add the edit list to the output track (just after the 'tkhd' atom)
                output_track->UseTrakAtom()->AddChild(new_edts, 1);
            }
        }
        
        // add the track to the output
        output_movie->AddTrack(output_track);
        
        // add a trex entry to the mvex container
        AP4_TrexAtom* trex = new AP4_TrexAtom(track->GetId(),
                                              1,
                                              0,
                                              0,
                                              0);
        mvex->AddChild(trex);
    }
    
    // select the anchor cursor
    TrackCursor* anchor_cursor = NULL;
    for (unsigned int i=0; i<cursors.ItemCount(); i++) {
        if (cursors[i]->m_Track->GetId() == track_id) {
            anchor_cursor = cursors[i];
        }
    }
    if (anchor_cursor == NULL) {
        for (unsigned int i=0; i<cursors.ItemCount(); i++) {
            // use this as the anchor track if it is the first video track
            if (cursors[i]->m_Track->GetType() == AP4_Track::TYPE_VIDEO) {
                anchor_cursor = cursors[i];
                break;
            }
        }
    }
    if (anchor_cursor == NULL) {
        // no video track to anchor with, pick the first audio track
        for (unsigned int i=0; i<cursors.ItemCount(); i++) {
            if (cursors[i]->m_Track->GetType() == AP4_Track::TYPE_AUDIO) {
                anchor_cursor = cursors[i];
                break;
            }
        }
        // no audio track to anchor with, pick the first subtitles track
        for (unsigned int i=0; i<cursors.ItemCount(); i++) {
            if (cursors[i]->m_Track->GetType() == AP4_Track::TYPE_SUBTITLES) {
                anchor_cursor = cursors[i];
                break;
            }
        }
    }
    if (anchor_cursor == NULL) {
        // this shoudl never happen
        fprintf(stderr, "ERROR: no anchor track\n");
        return;
    }
    if (create_segment_index) {
        index_cursor = anchor_cursor;
    }
    if (Options.debug) {
        printf("Using track ID %d as anchor\n", anchor_cursor->m_Track->GetId());
    }
    
    // update the mehd duration
    mehd->SetDuration(output_movie->GetDuration());
    
    // add the mvex container to the moov container
    output_movie->GetMoovAtom()->AddChild(mvex);

    // copy the moov/udta atom to the moov container
    if (copy_udta) {
        AP4_Atom* udta = input_movie->GetMoovAtom()->GetChild(AP4_ATOM_TYPE_UDTA);
        if (udta != NULL) {
            output_movie->GetMoovAtom()->AddChild(udta->Clone());
        }
    }
    
    // compute all the fragments
    unsigned int sequence_number = Options.sequence_number_start;
    for(;;) {
        TrackCursor* cursor = NULL;

        // pick the first track with a fragment index lower than the anchor's
        for (unsigned int i=0; i<cursors.ItemCount(); i++) {
            if (track_id && cursors[i]->m_Track->GetId() != track_id) continue;
            if (cursors[i]->m_Eos) continue;
            if (cursors[i]->m_FragmentIndex < anchor_cursor->m_FragmentIndex) {
                cursor = cursors[i];
                break;
            }
        }
        
        // check if we found a non-anchor cursor to use
        if (cursor == NULL) {
            // the anchor should be used in this round, check if we can use it
            if (anchor_cursor->m_Eos) {
                // the anchor is done, pick a new anchor unless we need to trim
                anchor_cursor = NULL;
                if (!Options.trim) {
                    for (unsigned int i=0; i<cursors.ItemCount(); i++) {
                        if (track_id && cursors[i]->m_Track->GetId() != track_id) continue;
                        if (cursors[i]->m_Eos) continue;
                        if (anchor_cursor == NULL ||
                            cursors[i]->m_Track->GetType() == AP4_Track::TYPE_VIDEO ||
                            cursors[i]->m_Track->GetType() == AP4_Track::TYPE_AUDIO) {
                            anchor_cursor = cursors[i];
                            if (Options.debug) {
                                printf("+++ New anchor: Track ID %d\n", anchor_cursor->m_Track->GetId());
                            }
                        }
                    }
                }
            }
            cursor = anchor_cursor;
        }
        if (cursor == NULL) break; // all done
        
        // decide how many samples go into this fragment
        AP4_UI64 target_dts;
        if (cursor == anchor_cursor) {
            // compute the current dts in milliseconds
            AP4_UI64 anchor_dts_ms = AP4_ConvertTime(cursor->m_Sample.GetDts(),
                                                     cursor->m_Track->GetMediaTimeScale(),
                                                     1000);
            // round to the nearest multiple of fragment_duration
            AP4_UI64 anchor_position = (anchor_dts_ms + (fragment_duration/2))/fragment_duration;
            
            // pick the next fragment_duration multiple at our target
            target_dts = AP4_ConvertTime(fragment_duration*(anchor_position+1),
                                         1000,
                                         cursor->m_Track->GetMediaTimeScale());
        } else {
            target_dts = AP4_ConvertTime(anchor_cursor->m_Sample.GetDts(),
                                         anchor_cursor->m_Track->GetMediaTimeScale(),
                                         cursor->m_Track->GetMediaTimeScale());
            if (target_dts <= cursor->m_Sample.GetDts()) {
                // we must be at the end, past the last anchor sample, just use the target duration
                target_dts = AP4_ConvertTime(fragment_duration*(cursor->m_FragmentIndex+1),
                                            1000,
                                            cursor->m_Track->GetMediaTimeScale());
                
                if (target_dts <= cursor->m_Sample.GetDts()) {
                    // we're still behind, there may have been an alignment/rounding error, just advance by one segment duration
                    target_dts = cursor->m_Sample.GetDts()+AP4_ConvertTime(fragment_duration,
                                                                           1000,
                                                                           cursor->m_Track->GetMediaTimeScale());
                }
            }
        }

        unsigned int end_sample_index = cursor->m_Samples->GetSampleCount();
        AP4_UI64 smallest_diff = (AP4_UI64)(0xFFFFFFFFFFFFFFFFULL);
        AP4_Sample sample;
        for (unsigned int i=cursor->m_SampleIndex+1; i<=cursor->m_Samples->GetSampleCount(); i++) {
            AP4_UI64 dts;
            if (i < cursor->m_Samples->GetSampleCount()) {
                result = cursor->m_Samples->GetSample(i, sample);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: failed to get sample %d (%d)\n", i, result);
                    return;
                }
                if (!sample.IsSync()) continue; // only look for sync samples
                dts = sample.GetDts();
            } else {
                result = cursor->m_Samples->GetSample(i-1, sample);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: failed to get sample %d (%d)\n", i-1, result);
                    return;
                }
                dts = sample.GetDts()+sample.GetDuration();
            }
            AP4_SI64 diff = dts-target_dts;
            AP4_UI64 abs_diff = diff<0?-diff:diff;
            if (abs_diff < smallest_diff) {
                // this sample is the closest to the target so far
                end_sample_index = i;
                smallest_diff = abs_diff;
            }
            if (diff >= 0) {
                // this sample is past the target, it is not going to get any better, stop looking
                break;
            }
        }
        if (cursor->m_Eos) continue;
        
        if (Options.debug) {
            if (cursor == anchor_cursor) {
                printf("====");
            } else {
                printf("----");
            }
            printf(" Track ID %d - dts=%lld, target=%lld, start=%d, end=%d/%d\n",
                   cursor->m_Track->GetId(),
                   cursor->m_Sample.GetDts(),
                   target_dts,
                   cursor->m_SampleIndex,
                   end_sample_index,
                   cursor->m_Track->GetSampleCount());
        }
        
        // emit a fragment for the selected track
        if (Options.verbosity > 1) {
            printf("fragment: track ID %d\n", cursor->m_Track->GetId());
        }

        // decide which sample description index to use
        // (this is not very sophisticated, we only look at the sample description
        // index of the first sample in the group, which may not be correct. This
        // should be fixed later)
        unsigned int sample_desc_index = cursor->m_Sample.GetDescriptionIndex();
        unsigned int tfhd_flags = AP4_TFHD_FLAG_DEFAULT_BASE_IS_MOOF;
        if (sample_desc_index > 0) {
            tfhd_flags |= AP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT;
        }
        if (cursor->m_Track->GetType() == AP4_Track::TYPE_VIDEO) {
            tfhd_flags |= AP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT;
        }
        
        // setup the moof structure
        AP4_ContainerAtom* moof = new AP4_ContainerAtom(AP4_ATOM_TYPE_MOOF);
        AP4_MfhdAtom* mfhd = new AP4_MfhdAtom(sequence_number++);
        moof->AddChild(mfhd);
        AP4_ContainerAtom* traf = new AP4_ContainerAtom(AP4_ATOM_TYPE_TRAF);
        AP4_TfhdAtom* tfhd = new AP4_TfhdAtom(tfhd_flags,
                                              cursor->m_Track->GetId(),
                                              0,
                                              sample_desc_index+1,
                                              0,
                                              0,
                                              0);
        if (tfhd_flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT) {
            tfhd->SetDefaultSampleFlags(0x1010000); // sample_is_non_sync_sample=1, sample_depends_on=1 (not I frame)
        }
        
        traf->AddChild(tfhd);
        if (!Options.no_tfdt) {
            AP4_TfdtAtom* tfdt = new AP4_TfdtAtom(1, cursor->m_Timestamp + (AP4_UI64)(Options.tfdt_start * (double)cursor->m_Track->GetMediaTimeScale()));
            traf->AddChild(tfdt);
        }
        AP4_UI32 trun_flags = AP4_TRUN_FLAG_DATA_OFFSET_PRESENT |
                              AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT;
        AP4_UI32 first_sample_flags = 0;
        if (cursor->m_Track->GetType() == AP4_Track::TYPE_VIDEO) {
            trun_flags |= AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT;
            first_sample_flags = 0x2000000; // sample_depends_on=2 (I frame)
        }
        AP4_TrunAtom* trun = new AP4_TrunAtom(trun_flags, 0, first_sample_flags);
        
        traf->AddChild(trun);
        moof->AddChild(traf);
        
        // create a new FragmentInfo object to store the fragment details
        FragmentInfo* fragment = new FragmentInfo(cursor->m_Samples, cursor->m_Tfra, cursor->m_Timestamp, moof);
        fragments.Add(fragment);
        
        // add samples to the fragment
        unsigned int sample_count = 0;
        AP4_Array<AP4_TrunAtom::Entry> trun_entries;
        fragment->m_MdatSize = AP4_ATOM_HEADER_SIZE;
        AP4_UI32 constant_sample_duration = 0;
        bool all_segment_durations_equal = true;
        for (;;) {
            // if we have one non-zero CTS delta, we'll need to express it
            if (cursor->m_Sample.GetCtsDelta()) {
                trun->SetFlags(trun->GetFlags() | AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT);
            }
            
            // add one sample
            trun_entries.SetItemCount(sample_count+1);
            AP4_TrunAtom::Entry& trun_entry = trun_entries[sample_count];
            AP4_UI64 next_unscaled_timestamp = cursor->m_UnscaledTimestamp+cursor->m_Sample.GetDuration();
            AP4_UI64 next_scaled_timestamp   = timescale?
                                               AP4_ConvertTime(next_unscaled_timestamp,
                                                               cursor->m_Track->GetMediaTimeScale(),
                                                               timescale):
                                               next_unscaled_timestamp;
            trun_entry.sample_duration                = (AP4_UI32)(next_scaled_timestamp-cursor->m_Timestamp);
            trun_entry.sample_size                    = cursor->m_Sample.GetSize();
            trun_entry.sample_composition_time_offset = timescale?
                                                        (AP4_UI32)AP4_ConvertTime(cursor->m_Sample.GetCtsDelta(),
                                                                                  cursor->m_Track->GetMediaTimeScale(),
                                                                                  timescale):
                                                        cursor->m_Sample.GetCtsDelta();
                        
            fragment->m_SampleIndexes.SetItemCount(sample_count+1);
            fragment->m_SampleIndexes[sample_count] = cursor->m_SampleIndex;
            fragment->m_MdatSize += trun_entry.sample_size;
            fragment->m_Duration += trun_entry.sample_duration;
            
            // check if the durations are all the same
            if (all_segment_durations_equal) {
                if (constant_sample_duration == 0) {
                    constant_sample_duration = trun_entry.sample_duration;
                } else {
                    if (constant_sample_duration != trun_entry.sample_duration) {
                        all_segment_durations_equal = false;
                    }
                }
            }
            
            // next sample
            cursor->m_UnscaledTimestamp = next_unscaled_timestamp;
            cursor->m_Timestamp         = next_scaled_timestamp;
            result = cursor->SetSampleIndex(cursor->m_SampleIndex+1);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to get sample %d (%d)\n", cursor->m_SampleIndex+1, result);
                return;
            }
            sample_count++;
            if (cursor->m_Eos) {
                if (Options.debug) {
                    printf("[Track ID %d has reached the end]\n", cursor->m_Track->GetId());
                }
                break;
            }
            if (cursor->m_SampleIndex >= end_sample_index) {
                break; // done with this fragment
            }
        }
        if (Options.verbosity > 2) {
            printf(" %d samples\n", sample_count);
            printf(" constant sample duration: %s\n", all_segment_durations_equal?"yes":"no");
        }
        
        // update the 'trun' flags if needed
        if (all_segment_durations_equal) {
            tfhd->SetDefaultSampleDuration(constant_sample_duration);
            tfhd->UpdateFlags(tfhd->GetFlags() | AP4_TFHD_FLAG_DEFAULT_SAMPLE_DURATION_PRESENT);
        } else {
            trun->SetFlags(trun->GetFlags() | AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT);
        }
        
        // update moof and children
        trun->SetEntries(trun_entries);
        trun->SetDataOffset((AP4_UI32)moof->GetSize()+AP4_ATOM_HEADER_SIZE);
        
        // advance the cursor's fragment index
        ++cursor->m_FragmentIndex;
    }
    
    // write the ftyp atom
    AP4_FtypAtom* ftyp = input_file.GetFileType();
    if (ftyp) {
        // keep the existing brand and compatible brands
        AP4_Array<AP4_UI32> compatible_brands;
        compatible_brands.EnsureCapacity(ftyp->GetCompatibleBrands().ItemCount()+1);
        for (unsigned int i=0; i<ftyp->GetCompatibleBrands().ItemCount(); i++) {
            compatible_brands.Append(ftyp->GetCompatibleBrands()[i]);
        }
        
        // add the compatible brand if it is not already there
        if (!ftyp->HasCompatibleBrand(AP4_FILE_BRAND_ISO5)) {
            compatible_brands.Append(AP4_FILE_BRAND_ISO5);
        }

        // create a replacement
        AP4_FtypAtom* new_ftyp = new AP4_FtypAtom(ftyp->GetMajorBrand(),
                                                  ftyp->GetMinorVersion(),
                                                  &compatible_brands[0],
                                                  compatible_brands.ItemCount());
        ftyp = new_ftyp;
    } else {
        AP4_UI32 compat = AP4_FILE_BRAND_ISO5;
        ftyp = new AP4_FtypAtom(AP4_FTYP_BRAND_MP42, 0, &compat, 1);
    }
    ftyp->Write(output_stream);
    delete ftyp;
    
    // write the moov atom
    output_movie->GetMoovAtom()->Write(output_stream);

    // write the (not-yet fully computed) index if needed
    AP4_SidxAtom* sidx = NULL;
    AP4_Position  sidx_position = 0;
    output_stream.Tell(sidx_position);
    if (create_segment_index) {
        sidx = new AP4_SidxAtom(index_cursor->m_Track->GetId(),
                                timescale?timescale:index_cursor->m_Track->GetMediaTimeScale(),
                                0,
                                0);
        // reserve space for the entries now, but they will be computed and updated later
        sidx->SetReferenceCount(fragments.ItemCount());
        sidx->Write(output_stream);
    }
    
    // write all fragments
    for (AP4_List<FragmentInfo>::Item* item = fragments.FirstItem();
                                       item;
                                       item = item->GetNext()) {
        FragmentInfo* fragment = item->GetData();

        // remember the time and position of this fragment
        output_stream.Tell(fragment->m_MoofPosition);
        fragment->m_Tfra->AddEntry(fragment->m_Timestamp, fragment->m_MoofPosition);
        
        // write the moof
        fragment->m_Moof->Write(output_stream);
        
        // write mdat
        output_stream.WriteUI32(fragment->m_MdatSize);
        output_stream.WriteUI32(AP4_ATOM_TYPE_MDAT);
        AP4_DataBuffer sample_data;
        AP4_Sample     sample;
        for (unsigned int i=0; i<fragment->m_SampleIndexes.ItemCount(); i++) {
            // get the sample
            result = fragment->m_Samples->GetSample(fragment->m_SampleIndexes[i], sample);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to get sample %d (%d)\n", fragment->m_SampleIndexes[i], result);
                return;
            }

            // read the sample data
            result = sample.ReadData(sample_data);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to read sample data for sample %d (%d)\n", fragment->m_SampleIndexes[i], result);
                return;
            }
            
            // write the sample data
            result = output_stream.Write(sample_data.GetData(), sample_data.GetDataSize());
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to write sample data (%d)\n", result);
                return;
            }
        }
    }

    // update the index and re-write it if needed
    if (create_segment_index) {
        unsigned int segment_index = 0;
        AP4_SidxAtom::Reference reference;
        for (AP4_List<FragmentInfo>::Item* item = fragments.FirstItem();
                                           item;
                                           item = item->GetNext()) {
            FragmentInfo* fragment = item->GetData();
            reference.m_ReferencedSize     = (AP4_UI32)(fragment->m_Moof->GetSize()+fragment->m_MdatSize);
            reference.m_SubsegmentDuration = fragment->m_Duration;
            reference.m_StartsWithSap      = true;
            sidx->SetReference(segment_index++, reference);
        }
        AP4_Position here = 0;
        output_stream.Tell(here);
        output_stream.Seek(sidx_position);
        sidx->Write(output_stream);
        output_stream.Seek(here);
        delete sidx;
    }
    
    // create an mfra container and write out the index
    AP4_ContainerAtom mfra(AP4_ATOM_TYPE_MFRA);
    for (unsigned int i=0; i<cursors.ItemCount(); i++) {
        if (track_id && cursors[i]->m_Track->GetId() != track_id) {
            continue;
        }
        mfra.AddChild(cursors[i]->m_Tfra);
        cursors[i]->m_Tfra = NULL;
    }
    AP4_MfroAtom* mfro = new AP4_MfroAtom((AP4_UI32)mfra.GetSize()+16);
    mfra.AddChild(mfro);
    result = mfra.Write(output_stream);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to write 'mfra' (%d)\n", result);
        return;
    }
    
    // cleanup
    fragments.DeleteReferences();
    for (unsigned int i=0; i<cursors.ItemCount(); i++) {
        delete cursors[i];
    }
    for (AP4_List<FragmentInfo>::Item* item = fragments.FirstItem();
                                       item;
                                       item = item->GetNext()) {
        FragmentInfo* fragment = item->GetData();
        delete fragment->m_Moof;
    }
    delete output_movie;
}

/*----------------------------------------------------------------------
|   AutoDetectFragmentDuration
+---------------------------------------------------------------------*/
static unsigned int 
AutoDetectFragmentDuration(TrackCursor* cursor)
{
    AP4_Sample   sample;
    unsigned int sample_count = cursor->m_Samples->GetSampleCount();
    
    // get the first sample as the starting point
    AP4_Result result = cursor->m_Samples->GetSample(0, sample);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to read first sample\n");
        return 0;
    }
    if (!sample.IsSync()) {
        fprintf(stderr, "ERROR: first sample is not an I frame\n");
        return 0;
    }
    
    for (unsigned int interval = 1; interval < sample_count; interval++) {
        bool irregular = false;
        unsigned int sync_count = 0;
        unsigned int i;
        for (i = 0; i < sample_count; i += interval) {
            result = cursor->m_Samples->GetSample(i, sample);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to read sample %d\n", i);
                return 0;
            }
            if (!sample.IsSync()) {
                irregular = true;
                break;
            }
            ++sync_count;
        }
        if (sync_count < 1) continue;
        if (!irregular) {
            // found a pattern
            AP4_UI64 duration = sample.GetDts();
            double fps = (double)(interval*(sync_count-1))/((double)duration/(double)cursor->m_Track->GetMediaTimeScale());
            if (Options.verbosity > 0) {
                printf("found regular I-frame interval: %d frames (at %.3f frames per second)\n",
                       interval, (float)fps);
            }
            return (unsigned int)(1000.0*(double)interval/fps);
        }
    }
    
    return 0;
}

/*----------------------------------------------------------------------
|   AutoDetectAudioFragmentDuration
+---------------------------------------------------------------------*/
static unsigned int 
AutoDetectAudioFragmentDuration(AP4_ByteStream& stream, TrackCursor* cursor)
{
    // remember where we are in the stream
    AP4_Position where = 0;
    stream.Tell(where);
    AP4_LargeSize stream_size = 0;
    stream.GetSize(stream_size);
    AP4_LargeSize bytes_available = stream_size-where;
    
    AP4_UI64  fragment_count = 0;
    AP4_UI32  last_fragment_size = 0;
    AP4_Atom* atom = NULL;
    AP4_DefaultAtomFactory atom_factory;
    while (AP4_SUCCEEDED(atom_factory.CreateAtomFromStream(stream, bytes_available, atom))) {
        if (atom && atom->GetType() == AP4_ATOM_TYPE_MOOF) {
            AP4_ContainerAtom* moof = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
            AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, moof->FindChild("traf/tfhd"));
            if (tfhd && tfhd->GetTrackId() == cursor->m_Track->GetId()) {
                ++fragment_count;
                AP4_TrunAtom* trun = AP4_DYNAMIC_CAST(AP4_TrunAtom, moof->FindChild("traf/trun"));
                if (trun) {
                    last_fragment_size = trun->GetEntries().ItemCount();
                }
            }
        }
        delete atom;
        atom = NULL;
    }
    
    // restore the stream to its original position
    stream.Seek(where);
    
    // decide if we can infer an fragment size
    if (fragment_count == 0 || cursor->m_Samples->GetSampleCount() == 0) {
        return 0;
    }
    // don't count the last fragment if we have more than one
    if (fragment_count > 1 && last_fragment_size) {
        --fragment_count;
    }
    if (fragment_count <= 1 || cursor->m_Samples->GetSampleCount() < last_fragment_size) {
        last_fragment_size = 0;
    }
    AP4_Sample sample;
    AP4_UI64 total_duration = 0;
    for (unsigned int i=0; i<cursor->m_Samples->GetSampleCount()-last_fragment_size; i++) {
        cursor->m_Samples->GetSample(i, sample);
        total_duration += sample.GetDuration();
    }
    return (unsigned int)AP4_ConvertTime(total_duration/fragment_count, cursor->m_Track->GetMediaTimeScale(), 1000);
}

/*----------------------------------------------------------------------
|   ReadGolomb
+---------------------------------------------------------------------*/
static unsigned int
ReadGolomb(AP4_BitStream& bits)
{
    unsigned int leading_zeros = 0;
    while (bits.ReadBit() == 0) {
        leading_zeros++;
    }
    if (leading_zeros) {
        return (1<<leading_zeros)-1+bits.ReadBits(leading_zeros);
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|   IsIFrame
+---------------------------------------------------------------------*/
static bool
IsIFrame(AP4_Sample& sample, AP4_AvcSampleDescription* avc_desc) {
    AP4_DataBuffer sample_data;
    if (AP4_FAILED(sample.ReadData(sample_data))) {
        return false;
    }

    const unsigned char* data = sample_data.GetData();
    AP4_Size             size = sample_data.GetDataSize();

    while (size >= avc_desc->GetNaluLengthSize()) {
        unsigned int nalu_length = 0;
        if (avc_desc->GetNaluLengthSize() == 1) {
            nalu_length = *data++;
            --size;
        } else if (avc_desc->GetNaluLengthSize() == 2) {
            nalu_length = AP4_BytesToUInt16BE(data);
            data += 2;
            size -= 2;
        } else if (avc_desc->GetNaluLengthSize() == 4) {
            nalu_length = AP4_BytesToUInt32BE(data);
            data += 4;
            size -= 4;
        } else {
            return false;
        }
        if (nalu_length <= size) {
            size -= nalu_length;
        } else {
            size = 0;
        }
        
        switch (*data & 0x1F) {
            case 1: {
                AP4_BitStream bits;
                bits.WriteBytes(data+1, 8);
                ReadGolomb(bits);
                unsigned int slice_type = ReadGolomb(bits);
                if (slice_type == 2 || slice_type == 7) {
                    return true;
                } else {
                    return false; // only show first slice type
                }
            }
            
            case 5: 
                return true;
        }
        
        data += nalu_length;
    }
 
    return false;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 2) {
        PrintUsageAndExit();
    }

    // init the variables
    const char*  input_filename                = NULL;
    const char*  output_filename               = NULL;
    const char*  track_selector                = NULL;
    AP4_UI32     selected_track_id             = 0;
    unsigned int fragment_duration             = 0;
    bool         auto_detect_fragment_duration = true;
    bool         create_segment_index          = false;
    bool         quiet                         = false;
    bool         copy_udta                     = false;
    AP4_UI32     timescale                     = 0;
    AP4_Result   result;

    Options.verbosity             = 1;
    Options.debug                 = false;
    Options.trim                  = false;
    Options.no_tfdt               = false;
    Options.tfdt_start            = 0.0;
    Options.sequence_number_start = 1;
    Options.force_i_frame_sync    = AP4_FRAGMENTER_FORCE_SYNC_MODE_NONE;
    
    // parse the command line
    argv++;
    char* arg;
    while ((arg = *argv++)) {
        if (!strcmp(arg, "--verbosity")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --verbosity option\n");
                return 1;
            }
            Options.verbosity = (unsigned int)strtoul(arg, NULL, 10);
        } else if (!strcmp(arg, "--debug")) {
            Options.debug = true;
        } else if (!strcmp(arg, "--index")) {
            create_segment_index = true;
        } else if (!strcmp(arg, "--quiet")) {
            quiet = true;
        } else if (!strcmp(arg, "--trim")) {
            Options.trim = true;
        } else if (!strcmp(arg, "--no-tfdt")) {
            Options.no_tfdt = true;
        } else if (!strcmp(arg, "--tfdt-start")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --tfdt-start option\n");
                return 1;
            }
            Options.tfdt_start = strtof(arg, NULL);
        } else if (!strcmp(arg, "--sequence-number-start")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --sequence-number-start option\n");
                return 1;
            }
            Options.sequence_number_start = (unsigned int)strtoul(arg, NULL, 10);
        } else if (!strcmp(arg, "--force-i-frame-sync")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --fragment-duration option\n");
                return 1;
            }
            if (!strcmp(arg, "all")) {
                Options.force_i_frame_sync = AP4_FRAGMENTER_FORCE_SYNC_MODE_ALL;
            } else if (!strcmp(arg, "auto")) {
                Options.force_i_frame_sync = AP4_FRAGMENTER_FORCE_SYNC_MODE_AUTO;
            } else {
                fprintf(stderr, "ERROR: unknown mode for --force-i-frame-sync\n");
                return 1;
            }
        } else if (!strcmp(arg, "--fragment-duration")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --fragment-duration option\n");
                return 1;
            }
            fragment_duration = (unsigned int)strtoul(arg, NULL, 10);
            auto_detect_fragment_duration = false;
        } else if (!strcmp(arg, "--timescale")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --timescale option\n");
                return 1;
            }
            timescale = (unsigned int)strtoul(arg, NULL, 10);
        } else if (!strcmp(arg, "--track")) {
            track_selector = *argv++;
            if (track_selector == NULL) {
                fprintf(stderr, "ERROR: missing argument after --track option\n");
                return 1;
            }
        } else if (!strcmp(arg, "--copy-udta")) {
            copy_udta = true;
        } else {
            if (input_filename == NULL) {
                input_filename = arg;
            } else if (output_filename == NULL) {
                output_filename = arg;
            } else {
                fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
                return 1;
            }
        }
    }
    if (Options.debug && Options.verbosity == 0) {
        Options.verbosity = 1;
    }
    
    if (input_filename == NULL) {
        fprintf(stderr, "ERROR: no input specified\n");
        return 1;
    }
    AP4_ByteStream* input_stream = NULL;
    result = AP4_FileByteStream::Create(input_filename, 
                                        AP4_FileByteStream::STREAM_MODE_READ, 
                                        input_stream);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
        return 1;
    }
    if (output_filename == NULL) {
        fprintf(stderr, "ERROR: no output specified\n");
        return 1;
    }
    AP4_ByteStream* output_stream = NULL;
    result = AP4_FileByteStream::Create(output_filename, 
                                        AP4_FileByteStream::STREAM_MODE_WRITE,
                                        output_stream);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot create/open output (%d)\n", result);
        return 1;
    }
    
    // parse the input MP4 file (moov only)
    AP4_File input_file(*input_stream, true);
    
    // check the file for basic properties
    if (input_file.GetMovie() == NULL) {
        fprintf(stderr, "ERROR: no movie found in the file\n");
        return 1;
    }
    if (!quiet && input_file.GetMovie()->HasFragments()) {
        fprintf(stderr, "NOTICE: file is already fragmented, it will be re-fragmented\n");
    }
    
    // create a cusor list to keep track of the tracks we will read from
    AP4_Array<TrackCursor*> cursors;
    
    // iterate over all tracks
    TrackCursor*  video_track = NULL;
    TrackCursor*  audio_track = NULL;
    TrackCursor*  subtitles_track = NULL;
    unsigned int video_track_count = 0;
    unsigned int audio_track_count = 0;
    unsigned int subtitles_track_count = 0;
    for (AP4_List<AP4_Track>::Item* track_item = input_file.GetMovie()->GetTracks().FirstItem();
                                    track_item;
                                    track_item = track_item->GetNext()) {
        AP4_Track* track = track_item->GetData();

        // sanity check
        if (track->GetSampleCount() == 0 && !input_file.GetMovie()->HasFragments()) {
            fprintf(stderr, "WARNING: track %d has no samples, it will be skipped\n", track->GetId());
            continue;
        }

        // create a sample array for this track
        SampleArray* sample_array;
        if (input_file.GetMovie()->HasFragments()) {
            sample_array = new CachedSampleArray(track);
        } else {
            sample_array = new SampleArray(track);
        }

        // create a cursor for the track
        TrackCursor* cursor = new TrackCursor(track, sample_array);
        cursor->m_Tfra->SetTrackId(track->GetId());
        cursors.Append(cursor);

        if (track->GetType() == AP4_Track::TYPE_VIDEO) {
            if (video_track) {
                fprintf(stderr, "WARNING: more than one video track found\n");
            } else {
                video_track = cursor;
            }
            video_track_count++;
        } else if (track->GetType() == AP4_Track::TYPE_AUDIO) {
            if (audio_track == NULL) {
                audio_track = cursor;
            }
            audio_track_count++;
        } else if (track->GetType() == AP4_Track::TYPE_SUBTITLES) {
            if (subtitles_track == NULL) {
                subtitles_track = cursor;
            }
            subtitles_track_count++;
        }
    }

    if (cursors.ItemCount() == 0) {
        fprintf(stderr, "ERROR: no valid track found\n");
        return 1;
    }
    
    if (track_selector) {
        if (!strncmp("audio", track_selector, 5)) {
            if (audio_track) {
                selected_track_id = audio_track->m_Track->GetId();
            } else {
                fprintf(stderr, "ERROR: no audio track found\n");
                return 1;
            }
        } else if (!strncmp("video", track_selector, 5)) {
            if (video_track) {
                selected_track_id = video_track->m_Track->GetId();
            } else {
                fprintf(stderr, "ERROR: no video track found\n");
                return 1;
            }
        } else if (!strncmp("subtitles", track_selector, 9)) {
            if (subtitles_track) {
                selected_track_id = subtitles_track->m_Track->GetId();
            } else {
                fprintf(stderr, "ERROR: no subtitles track found\n");
                return 1;
            }
        } else {
            selected_track_id = (AP4_UI32)strtol(track_selector, NULL, 10);
            bool found = false;
            for (unsigned int i=0; i<cursors.ItemCount(); i++) {
                if (cursors[i]->m_Track->GetId() == selected_track_id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "ERROR: track not found\n");
                return 1;
            }
        }
    }
    
    if (video_track_count == 0 && audio_track_count == 0 && subtitles_track_count == 0) {
        fprintf(stderr, "ERROR: no audio, video, or subtitles track in the file\n");
        return 1;
    }
    AP4_AvcSampleDescription* avc_desc = NULL;
    if (video_track && (Options.force_i_frame_sync != AP4_FRAGMENTER_FORCE_SYNC_MODE_NONE)) {
        // that feature is only supported for AVC
        AP4_SampleDescription* sdesc = video_track->m_Track->GetSampleDescription(0);
        if (sdesc) {
            avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sdesc);
        }
        if (avc_desc == NULL) {
            fprintf(stderr, "--force-i-frame-sync can only be used with AVC/H.264 video\n");
            return 1;
        }
    }
    
    // remember where the stream was
    AP4_Position position;
    input_stream->Tell(position);

    // for fragmented input files, we need to populate the sample arrays
    if (input_file.GetMovie()->HasFragments()) {
        AP4_LinearReader reader(*input_file.GetMovie(), input_stream);
        for (unsigned int i=0; i<cursors.ItemCount(); i++) {
            reader.EnableTrack(cursors[i]->m_Track->GetId());
        }
        AP4_UI32 track_id;
        AP4_Sample sample;
        do {
            result = reader.GetNextSample(sample, track_id);
            if (AP4_SUCCEEDED(result)) {
                for (unsigned int i=0; i<cursors.ItemCount(); i++) {
                    if (cursors[i]->m_Track->GetId() == track_id) {
                        cursors[i]->m_Samples->AddSample(sample);
                        break;
                    }
                }
            }
        } while (AP4_SUCCEEDED(result));
        
    } else if (video_track && (Options.force_i_frame_sync != AP4_FRAGMENTER_FORCE_SYNC_MODE_NONE)) {
        AP4_Sample sample;
        if (Options.force_i_frame_sync == AP4_FRAGMENTER_FORCE_SYNC_MODE_AUTO) {
            // detect if this looks like an open-gop source
            for (unsigned int i=1; i<video_track->m_Samples->GetSampleCount(); i++) {
                if (AP4_SUCCEEDED(video_track->m_Samples->GetSample(i, sample))) {
                    if (sample.IsSync()) {
                        // we found a sync i-frame, assume this is *not* an open-gop source
                        Options.force_i_frame_sync = AP4_FRAGMENTER_FORCE_SYNC_MODE_NONE;
                        if (Options.debug) {
                            printf("this does not look like an open-gop source, not forcing i-frame sync flags\n");
                        }
                        break;
                    }
                }
            }
        }
        if (Options.force_i_frame_sync != AP4_FRAGMENTER_FORCE_SYNC_MODE_NONE) {
            for (unsigned int i=0; i<video_track->m_Samples->GetSampleCount(); i++) {
                if (AP4_SUCCEEDED(video_track->m_Samples->GetSample(i, sample))) {
                    if (IsIFrame(sample, avc_desc)) {
                        video_track->m_Samples->ForceSync(i);
                    }
                }
            }
        }
    }

    // return the stream to its original position
    input_stream->Seek(position);

    // auto-detect the fragment duration if needed
    if (auto_detect_fragment_duration) {
        if (video_track) {
            fragment_duration = AutoDetectFragmentDuration(video_track);
        } else if (audio_track && input_file.GetMovie()->HasFragments()) {
            fragment_duration = AutoDetectAudioFragmentDuration(*input_stream, audio_track);
        }
        if (fragment_duration == 0) {
            if (Options.verbosity > 0) {
                fprintf(stderr, "unable to autodetect fragment duration, using default\n");
            }
            fragment_duration = AP4_FRAGMENTER_DEFAULT_FRAGMENT_DURATION;
        } else if (fragment_duration > AP4_FRAGMENTER_MAX_AUTO_FRAGMENT_DURATION) {
            if (Options.verbosity > 0) {
                fprintf(stderr, "auto-detected fragment duration too large, using default\n");
            }
            fragment_duration = AP4_FRAGMENTER_DEFAULT_FRAGMENT_DURATION;
        }
    }
    
    // fragment the file
    Fragment(input_file, *output_stream, cursors, fragment_duration, timescale, selected_track_id, create_segment_index, copy_udta);
    
    // cleanup and exit
    if (input_stream)  input_stream->Release();
    if (output_stream) output_stream->Release();

    return 0;
}
