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
#define BANNER "MP4 Fragmenter - Version 1.1\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2010 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_FRAGMENTER_AUDIO_TRACK_ID            = 1;
const unsigned int AP4_FRAGMENTER_VIDEO_TRACK_ID            = 2;
const unsigned int AP4_FRAGMENTER_DEFAULT_FRAGMENT_DURATION = 2;

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
    
    AP4_Result   SetTrack(AP4_Track* track);
    AP4_Track*   m_Track;
    unsigned int m_TrackId;
    AP4_Ordinal  m_SampleIndex;
    AP4_Sample   m_Sample;
    bool         m_Eos;
};

/*----------------------------------------------------------------------
|   TrackCursor::TrackCursor
+---------------------------------------------------------------------*/
TrackCursor::TrackCursor() :
    m_Track(NULL), 
    m_TrackId(0),
    m_SampleIndex(0),
    m_Eos(false)
{
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
Fragment(AP4_File& input_file, AP4_ByteStream& output_stream)
{
    AP4_Result result;
    
    AP4_Movie* input_movie = input_file.GetMovie();
    if (input_movie == NULL) {
        fprintf(stderr, "ERROR: no moov found in the input file\n");
        return;
    }

    // create the output file object
    AP4_Movie* output_movie = new AP4_Movie();
    
    // create an mvex container
    AP4_ContainerAtom* mvex = new AP4_ContainerAtom(AP4_ATOM_TYPE_MVEX);
    AP4_MehdAtom* mehd = new AP4_MehdAtom(0); 
    mvex->AddChild(mehd);
    
    // create a cusor list to keep track of the tracks we will read from
    TrackCursor* audio_cursor = NULL;
    TrackCursor* video_cursor = NULL;
    
    // add an output track for each audio or video in the input file
    for (AP4_List<AP4_Track>::Item* track_item = input_movie->GetTracks().FirstItem();
                                    track_item;
                                    track_item = track_item->GetNext()) {
        AP4_Track* track = track_item->GetData();
        TrackCursor* cursor = NULL;
        if (track->GetType() == AP4_Track::TYPE_AUDIO) {
            if (audio_cursor) continue;
            audio_cursor = new TrackCursor();
            cursor = audio_cursor;
            cursor->m_TrackId = AP4_FRAGMENTER_AUDIO_TRACK_ID;
        } else if (track->GetType() == AP4_Track::TYPE_VIDEO) {
            if (video_cursor) continue;
            video_cursor = new TrackCursor();
            cursor = video_cursor;
            cursor->m_TrackId = AP4_FRAGMENTER_VIDEO_TRACK_ID;
        } else {
            continue;
        }
            
        // create a sample table (with no samples) to hold the sample description
        // (we only use sample description 0 here)
        AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();
        AP4_SampleDescription* sample_description = track->GetSampleDescription(0);
        sample_table->AddSampleDescription(sample_description, false);
        
        // create the track
        AP4_Track* output_track = new AP4_Track(track->GetType(),
                                                sample_table,
                                                cursor->m_TrackId,
                                                1000, // timescale in milliseconds
                                                track->GetDurationMs(),
                                                track->GetMediaTimeScale(),
                                                track->GetMediaDuration(),
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
    
    // count how many tracks we have
    TrackCursor* cursors[2] = {NULL, NULL};
    unsigned int track_count = 0;
    if (video_cursor) cursors[track_count++] = video_cursor;
    if (audio_cursor) cursors[track_count++] = audio_cursor;
    if (track_count == 0) {
        fprintf(stderr, "ERROR: no audio or video track found\n");
        return;
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
    TrackCursor* cursor             = NULL;
    AP4_UI64     target_duration_ms = AP4_FRAGMENTER_DEFAULT_FRAGMENT_DURATION*1000;
    unsigned int sequence_number    = 1;
    while ((cursors[0] == NULL || !cursors[0]->m_Eos) && 
           (cursors[1] == NULL || !cursors[1]->m_Eos)) {
        // select the next track to  read from
        if (track_count == 1) {
            cursor = cursors[0];
        } else {
            if (cursor == NULL || cursor == audio_cursor) {
                cursor = video_cursor;
            } else {
                cursor = audio_cursor;
            }
        }
        
        // emit a fragment for the selected track
        printf("Frag: %s (%d) ", cursor==audio_cursor?"audio":"video", cursor->m_Track->GetId());
        if (cursor == video_cursor && target_duration_ms >= 1000) {
            // for video tracks, allow the fragment to be 'close' to the target
            // duration without being exactly equal or larger
            target_duration_ms -= 100;
        }
        AP4_UI64 max_duration = AP4_ConvertTime(target_duration_ms, 
                                                1000, 
                                                cursor->m_Track->GetMediaTimeScale());
        AP4_UI64 start_dts = cursor->m_Sample.GetDts(); 
        AP4_UI64 max_dts   =  start_dts + max_duration;
        unsigned int sample_count = 0;

        // setup the moof structure
        AP4_ContainerAtom* moof = new AP4_ContainerAtom(AP4_ATOM_TYPE_MOOF);
        AP4_MfhdAtom* mfhd = new AP4_MfhdAtom(sequence_number++);
        moof->AddChild(mfhd);
        AP4_ContainerAtom* traf = new AP4_ContainerAtom(AP4_ATOM_TYPE_TRAF);
        AP4_TfhdAtom* tfhd = new AP4_TfhdAtom(0,
                                              cursor->m_TrackId,
                                              0,
                                              0,
                                              0,
                                              0,
                                              0);
        traf->AddChild(tfhd);
        AP4_TrunAtom* trun = new AP4_TrunAtom(AP4_TRUN_FLAG_DATA_OFFSET_PRESENT     |
                                              AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT |
                                              AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT,
                                              0, 0);
        traf->AddChild(trun);
        moof->AddChild(traf);
            
        // decide which samples go in this fragment
        AP4_Array<AP4_UI32>            sample_indexes;
        AP4_Array<AP4_TrunAtom::Entry> trun_entries;
        AP4_UI32                       mdat_size = AP4_ATOM_HEADER_SIZE;
        for (;;) {
            trun_entries.SetItemCount(sample_count+1);
            AP4_TrunAtom::Entry& trun_entry = trun_entries[sample_count];
            trun_entry.sample_duration = cursor->m_Sample.GetDuration();
            trun_entry.sample_size     = cursor->m_Sample.GetSize();
            
            sample_indexes.SetItemCount(sample_count+1);
            sample_indexes[sample_count] = cursor->m_SampleIndex;
            mdat_size += trun_entry.sample_size;
            
            // next sample
            cursor->m_SampleIndex++;
            sample_count++;
            if (cursor->m_SampleIndex >= cursor->m_Track->GetSampleCount()) {
                cursor->m_Eos = true;
                cursor->m_Sample.Reset();
                break;
            }
            result = cursor->m_Track->GetSample(cursor->m_SampleIndex, cursor->m_Sample);
            if (AP4_FAILED(result)) {
                cursor->m_Eos = true;
                cursor->m_Sample.Reset();
                break;
            }
            if (cursor->m_Sample.IsSync()) {
                if (cursor->m_Sample.GetDts() >= max_dts) {
                    break;
                }
            }
        }
        if (cursor == video_cursor) {
            AP4_UI64 fragment_duration = cursor->m_Sample.GetDts()-start_dts;
            target_duration_ms = AP4_ConvertTime(fragment_duration, cursor->m_Track->GetMediaTimeScale(), 1000);
        } else {
            target_duration_ms = AP4_FRAGMENTER_DEFAULT_FRAGMENT_DURATION*1000;
        }
        printf(" %d samples\n", sample_count);
                
        // update moof and children
        trun->SetEntries(trun_entries);
        trun->SetDataOffset(moof->GetSize()+AP4_ATOM_HEADER_SIZE);
        
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
    
    // cleanup
    if (audio_cursor) delete audio_cursor;
    if (video_cursor) delete video_cursor;
    delete output_movie;
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
    const char*             input_filename  = NULL;
    const char*             output_filename = NULL;
    AP4_Ordinal             verbosity = 0;
    AP4_Result              result;
    
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
            verbosity = strtoul(arg, NULL, 10);
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
    
    // fragment the file
    Fragment(input_file, *output_stream);
    
    // cleanup and exit
    if (input_stream)  input_stream->Release();
    if (output_stream) output_stream->Release();

    return 0;
}
