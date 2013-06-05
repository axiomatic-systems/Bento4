/*****************************************************************
|
|    AP4 - MP4 Fragmenter
|
|    Copyright 2002-2010 Axiomatic Systems, LLC
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
#define BANNER "MP4 Fragmenter - Version 1.4\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2013 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_FRAGMENTER_DEFAULT_FRAGMENT_DURATION   = 2000; // ms
const unsigned int AP4_FRAGMENTER_MAX_AUTO_FRAGMENT_DURATION  = 15000; 
const unsigned int AP4_FRAGMENTER_FRAGMENT_DURATION_TOLERANCE = 100;
const unsigned int AP4_FRAGMENTER_BASE_TIMESCALE              = 1000; // milliseconds

/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
struct _Options {
    unsigned int verbosity;
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
            "  --fragment-duration <milliseconds> (default = automatic)\n"
            "  --timescale <n> (use 10000000 for Smooth Streaming compatibility)\n"
            );
    exit(1);
}

/*----------------------------------------------------------------------
|   TrackCursor
+---------------------------------------------------------------------*/
class TrackCursor
{
public:
    TrackCursor();
    ~TrackCursor();
    
    AP4_Result    SetTrack(AP4_Track* track);
    AP4_Track*    m_Track;
    unsigned int  m_TrackId;
    AP4_Ordinal   m_SampleIndex;
    AP4_Sample    m_Sample;
    AP4_UI64      m_Timestamp;
    bool          m_Eos;
    AP4_UI64      m_TargetDuration;
    AP4_UI64      m_EndDts;
    AP4_TfraAtom* m_Tfra;
};

/*----------------------------------------------------------------------
|   TrackCursor::TrackCursor
+---------------------------------------------------------------------*/
TrackCursor::TrackCursor() :
    m_Track(NULL), 
    m_TrackId(0),
    m_SampleIndex(0),
    m_Timestamp(0),
    m_Eos(false),
    m_TargetDuration(0),
    m_EndDts(0),
    m_Tfra(new AP4_TfraAtom(0))
{
}

/*----------------------------------------------------------------------
|   TrackCursor::~TrackCursor
+---------------------------------------------------------------------*/
TrackCursor::~TrackCursor()
{
    delete m_Tfra;
}

/*----------------------------------------------------------------------
|   TrackCursor::SetTrack
+---------------------------------------------------------------------*/
AP4_Result
TrackCursor::SetTrack(AP4_Track* track)
{
    m_Track = track;
    return track->GetSample(0, m_Sample);
}

/*----------------------------------------------------------------------
|   Fragment
+---------------------------------------------------------------------*/
static void
Fragment(AP4_File&       input_file,
         AP4_ByteStream& output_stream,
         unsigned int    fragment_duration,
         AP4_UI32        timescale)
{
    AP4_Result result;
    
    AP4_Movie* input_movie = input_file.GetMovie();
    if (input_movie == NULL) {
        fprintf(stderr, "ERROR: no moov found in the input file\n");
        return;
    }

    // create the output file object
    AP4_Movie* output_movie = new AP4_Movie(1000);
    
    // create an mvex container
    AP4_ContainerAtom* mvex = new AP4_ContainerAtom(AP4_ATOM_TYPE_MVEX);
    AP4_MehdAtom* mehd = new AP4_MehdAtom(0); 
    mvex->AddChild(mehd);
    
    // create a cusor list to keep track of the tracks we will read from
    AP4_Array<TrackCursor*> cursors;
    
    // add an output track for each track in the input file
    for (AP4_List<AP4_Track>::Item* track_item = input_movie->GetTracks().FirstItem();
                                    track_item;
                                    track_item = track_item->GetNext()) {
        AP4_Track* track = track_item->GetData();
        TrackCursor* cursor = new TrackCursor();
        cursor->m_TrackId = track->GetId();
        cursor->m_Tfra->SetTrackId(track->GetId());
        cursors.Append(cursor);
                    
        // create a sample table (with no samples) to hold the sample description
        AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();
        for (unsigned int i=0; i<track->GetSampleDescriptionCount(); i++) {
            AP4_SampleDescription* sample_description = track->GetSampleDescription(i);
            sample_table->AddSampleDescription(sample_description, false);
        }
        
        // create the track
        AP4_Track* output_track = new AP4_Track(track->GetType(),
                                                sample_table,
                                                cursor->m_TrackId,
                                                timescale?timescale:1000,
                                                AP4_ConvertTime(track->GetDuration(),
                                                                input_movie->GetTimeScale(),
                                                                timescale?timescale:1000),
                                                timescale?timescale:track->GetMediaTimeScale(),
                                                0,//track->GetMediaDuration(),
                                                track->GetTrackLanguage(),
                                                track->GetWidth(),
                                                track->GetHeight());
        output_movie->AddTrack(output_track);
        result = cursor->SetTrack(track);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: failed to read sample (%d)\n", result);
            return;
        }
                
        // add a trex entry to the mvex container
        AP4_TrexAtom* trex = new AP4_TrexAtom(cursor->m_TrackId,
                                              1,
                                              0,
                                              0,
                                              0);
        mvex->AddChild(trex);
    }
    
    if (cursors.ItemCount() == 0) {
        fprintf(stderr, "ERROR: no track found\n");
        return;
    }

    for (unsigned int i=0; i<cursors.ItemCount(); i++) {
        if (cursors[i]->m_Track->GetType() == AP4_Track::TYPE_VIDEO) {
            cursors[i]->m_TargetDuration = AP4_ConvertTime(fragment_duration>AP4_FRAGMENTER_FRAGMENT_DURATION_TOLERANCE ?
                                                           fragment_duration-AP4_FRAGMENTER_FRAGMENT_DURATION_TOLERANCE : 0,
                                                           1000,
                                                           cursors[i]->m_Track->GetMediaTimeScale());
        } else {
            cursors[i]->m_TargetDuration = AP4_ConvertTime(fragment_duration,
                                                           1000,
                                                           cursors[i]->m_Track->GetMediaTimeScale());
        }
    }
    
    // update the mehd duration
    mehd->SetDuration(output_movie->GetDuration());
    
    // the mvex container to the moov container
    output_movie->GetMoovAtom()->AddChild(mvex);
    
    // write the ftyp atom
    AP4_FtypAtom* ftyp = input_file.GetFileType();
    if (ftyp) {
        ftyp->Write(output_stream);
    }
                 
    // write the moov atom
    output_movie->GetMoovAtom()->Write(output_stream);
    
    // write all the fragments
    unsigned int sequence_number = 1;
    for(;;) {
        // select the next track to read from
        TrackCursor* cursor = NULL;
        AP4_UI64 min_dts = (AP4_UI64)(-1);
        for (unsigned int i=0; i<cursors.ItemCount(); i++) {
            if (cursors[i]->m_Eos) continue;
            AP4_UI64 dts = AP4_ConvertTime(cursors[i]->m_Sample.GetDts(),
                                           cursors[i]->m_Track->GetMediaTimeScale(),
                                           AP4_FRAGMENTER_BASE_TIMESCALE);
            if (dts < min_dts) {
                min_dts = dts;
                cursor = cursors[i];
            }
        }
        if (cursor == NULL) break; // all done
        
        // compute the target end for the segment
        cursor->m_EndDts = cursor->m_Sample.GetDts()+cursor->m_TargetDuration;
        
        // emit a fragment for the selected track
        if (Options.verbosity > 0) {
            printf("fragment: track ID %d ", cursor->m_Track->GetId());
        }

        // remember the time and position of this fragment
        AP4_Position moof_offset = 0;
        output_stream.Tell(moof_offset);
        cursor->m_Tfra->AddEntry(cursor->m_Timestamp, moof_offset);
        
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
                                              cursor->m_TrackId,
                                              0,
                                              sample_desc_index+1,
                                              0,
                                              0,
                                              0);
        if (tfhd_flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT) {
            tfhd->SetDefaultSampleFlags(0x1010000); // sample_is_non_sync_sample=1, sample_depends_on=1 (not I frame)
        }
        
        traf->AddChild(tfhd);
        AP4_TfdtAtom* tfdt = new AP4_TfdtAtom(1, cursor->m_Timestamp);
        traf->AddChild(tfdt);
        AP4_UI32 trun_flags = AP4_TRUN_FLAG_DATA_OFFSET_PRESENT     |
                              AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT |
                              AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT;
        AP4_UI32 first_sample_flags = 0;
        if (cursor->m_Track->GetType() == AP4_Track::TYPE_VIDEO) {
            trun_flags |= AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT;
            first_sample_flags = 0x2000000; // sample_depends_on=2 (I frame)
        }
        AP4_TrunAtom* trun = new AP4_TrunAtom(trun_flags, 0, first_sample_flags);
        
        traf->AddChild(trun);
        moof->AddChild(traf);
            
        // decide which samples go in this fragment
        AP4_Array<AP4_UI32>            sample_indexes;
        unsigned int                   sample_count = 0;
        AP4_Array<AP4_TrunAtom::Entry> trun_entries;
        AP4_UI32                       mdat_size = AP4_ATOM_HEADER_SIZE;
        for (;;) {
            // if we have one non-zero CTS delta, we'll need to express it
            if (cursor->m_Sample.GetCtsDelta()) {
                trun->SetFlags(trun->GetFlags() | AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT);
            }
            
            // add one sample
            trun_entries.SetItemCount(sample_count+1);
            AP4_TrunAtom::Entry& trun_entry = trun_entries[sample_count];
            trun_entry.sample_duration                = timescale?
                                                        (AP4_UI32)AP4_ConvertTime(cursor->m_Sample.GetDuration(),
                                                                                  cursor->m_Track->GetMediaTimeScale(),
                                                                                  timescale):
                                                        cursor->m_Sample.GetDuration();
            trun_entry.sample_size                    = cursor->m_Sample.GetSize();
            trun_entry.sample_composition_time_offset = timescale?
                                                        (AP4_UI32)AP4_ConvertTime(cursor->m_Sample.GetCtsDelta(),
                                                                                  cursor->m_Track->GetMediaTimeScale(),
                                                                                  timescale):
                                                        cursor->m_Sample.GetCtsDelta();
                        
            sample_indexes.SetItemCount(sample_count+1);
            sample_indexes[sample_count] = cursor->m_SampleIndex;
            mdat_size += trun_entry.sample_size;
            
            // next sample
            cursor->m_Timestamp += trun_entry.sample_duration;
            cursor->m_SampleIndex++;
            sample_count++;
            if (cursor->m_SampleIndex >= cursor->m_Track->GetSampleCount()) {
                cursor->m_Eos = true;

                AP4_UI64 end_dts = cursor->m_Sample.GetDts()+cursor->m_Sample.GetDuration();
                cursor->m_Sample.Reset();
                cursor->m_Sample.SetDts(end_dts);

                break;
            }
            result = cursor->m_Track->GetSample(cursor->m_SampleIndex, cursor->m_Sample);
            if (AP4_FAILED(result)) {
                cursor->m_Eos = true;

                AP4_UI64 end_dts = cursor->m_Sample.GetDts()+cursor->m_Sample.GetDuration();
                cursor->m_Sample.Reset();
                cursor->m_Sample.SetDts(end_dts);

                break;
            }
            if (cursor->m_Sample.IsSync()) {
                if (cursor->m_Sample.GetDts() >= cursor->m_EndDts) {
                    break; // done with this segment
                }
            }
        }
        if (Options.verbosity) {
            printf(" %d samples\n", sample_count);
        }
                
        // update moof and children
        trun->SetEntries(trun_entries);
        trun->SetDataOffset((AP4_UI32)moof->GetSize()+AP4_ATOM_HEADER_SIZE);
        
        // write moof
        moof->Write(output_stream);
        
        // write mdat
        output_stream.WriteUI32(mdat_size);
        output_stream.WriteUI32(AP4_ATOM_TYPE_MDAT);
        AP4_Sample     sample;
        AP4_DataBuffer sample_data;
        for (unsigned int i=0; i<sample_indexes.ItemCount(); i++) {
            result = cursor->m_Track->ReadSample(sample_indexes[i], sample, sample_data);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to read sample %d (%d)\n", sample_indexes[i], result);
                return;
            }
            result = output_stream.Write(sample_data.GetData(), sample_data.GetDataSize());
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: failed to write sample data (%d)\n", result);
                return;
            }
        }
        
        // cleanup
        delete moof;
    }
    
    // create an mfra container and write out the index
    AP4_ContainerAtom mfra(AP4_ATOM_TYPE_MFRA);
    for (unsigned int i=0; i<cursors.ItemCount(); i++) {
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
    for (unsigned int i=0; i<cursors.ItemCount(); i++) {
        delete cursors[i];
    }
    delete output_movie;
}

/*----------------------------------------------------------------------
|   AutoDetectFragmentDuration
+---------------------------------------------------------------------*/
static unsigned int 
AutoDetectFragmentDuration(AP4_Track* track)
{
    AP4_Sample   sample;
    unsigned int sample_count = track->GetSampleCount();
    
    // get the first sample as the starting point
    AP4_Result result = track->GetSample(0, sample);
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
            result = track->GetSample(i, sample);
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
            double fps = (double)(interval*(sync_count-1))/((double)duration/(double)track->GetMediaTimeScale());
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
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 2) {
        PrintUsageAndExit();
    }

    // init the variables
    const char*  input_filename    = NULL;
    const char*  output_filename   = NULL;
    unsigned int fragment_duration = 0;
    bool         auto_detect_fragment_duration = true;
    AP4_UI32     timescale = 0;
    AP4_Result   result;

    Options.verbosity = 0;
    
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
            Options.verbosity = strtoul(arg, NULL, 10);
        } else if (!strcmp(arg, "--fragment-duration")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --fragment-duration option\n");
                return 1;
            }
            fragment_duration = strtoul(arg, NULL, 10);
            auto_detect_fragment_duration = false;
        } else if (!strcmp(arg, "--timescale")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --timescale option\n");
                return 1;
            }
            timescale = strtoul(arg, NULL, 10);
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
    AP4_File input_file(*input_stream, AP4_DefaultAtomFactory::Instance, true);
    
    // check the file for basic properties
    if (input_file.GetMovie() == NULL) {
        fprintf(stderr, "ERROR: no movie found in the file\n");
        return 1;
    }
    if (input_file.GetMovie()->HasFragments()) {
        fprintf(stderr, "ERROR: file is already fragmented\n");
        return 1;
    }
    AP4_Track* audio_track = input_file.GetMovie()->GetTrack(AP4_Track::TYPE_AUDIO);
    AP4_Track* video_track = input_file.GetMovie()->GetTrack(AP4_Track::TYPE_VIDEO);
    if (audio_track == NULL && video_track == NULL) {
        fprintf(stderr, "ERROR: no audio or video track in the file\n");
        return 1;
    }
    if (audio_track && audio_track->GetSampleCount() == 0) {
        fprintf(stderr, "ERROR: audio track has no samples\n");
        return 1;
    }
    if (video_track && video_track->GetSampleCount() == 0) {
        fprintf(stderr, "ERROR: video track has no samples\n");
        return 1;
    }
    
    // auto-detect the fragment duration if needed
    if (auto_detect_fragment_duration) {
        if (video_track) {
            fragment_duration = AutoDetectFragmentDuration(video_track);
        } else {
            if (Options.verbosity > 0) {
                fprintf(stderr, "no video track, cannot autodetect fragment duration\n");
            }
        }
        if (fragment_duration == 0) {
            if (Options.verbosity > 0) {
                fprintf(stderr, "unable to detect fragment duration, using default\n");
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
    Fragment(input_file, *output_stream, fragment_duration, timescale);
    
    // cleanup and exit
    if (input_stream)  input_stream->Release();
    if (output_stream) output_stream->Release();

    return 0;
}
