/*****************************************************************
|
|    AP4 - Linear Test
|
|    Copyright 2002-2009 Axiomatic Systems, LLC
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
|   macros
+---------------------------------------------------------------------*/
#define CHECK(x) do { \
    if (!(x)) { fprintf(stderr, "ERROR line %d", __LINE__); return -1; }\
} while (0)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "Linear Reader Test - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2009 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: linearreadertest <test-filename>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc != 2) {
        PrintUsageAndExit();
    }
    const char* input_filename  = argv[1];
    
    // open the input
    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }
        
    // get the movie
    AP4_File* file = new AP4_File(*input, true);
    AP4_Movie* movie = file->GetMovie();
    CHECK(movie != NULL);
    
    AP4_Track* video_track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    CHECK(video_track != NULL);
    AP4_Track* audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    CHECK(audio_track != NULL);
    
    AP4_LinearReader reader(*movie, input);
    AP4_Sample sample;
    AP4_DataBuffer sample_data;
    reader.EnableTrack(audio_track->GetId());
    reader.EnableTrack(video_track->GetId());
    
    AP4_UI64     offset = 0;
    AP4_Cardinal audio_sample_count = 0;
    AP4_Cardinal video_sample_count = 0;
    do {
        AP4_UI32 track_id = 0;
        result = reader.ReadNextSample(sample, sample_data, track_id);
        if (AP4_SUCCEEDED(result)) {
            CHECK(offset < sample.GetOffset());
            offset = sample.GetOffset();
            CHECK(track_id == audio_track->GetId() || track_id == video_track->GetId());
            if (track_id == audio_track->GetId()) audio_sample_count++;
            if (track_id == video_track->GetId()) video_sample_count++;
            printf("track_id=%d, size=%d, offset=%lld, dts=%lld\n", track_id, (int)sample.GetSize(), sample.GetOffset(), sample.GetDts());
        } else {
            printf("track_id=%d, result=%d\n", track_id, result);
        }
    } while (AP4_SUCCEEDED(result));
    CHECK(result == AP4_ERROR_EOS);
    CHECK(reader.GetBufferFullness() == 0);
    CHECK(audio_sample_count == audio_track->GetSampleCount());
    CHECK(video_sample_count == video_track->GetSampleCount());

    offset = 0;
    audio_sample_count = 0;
    //video_sample_count = 0;
    reader.SetSampleIndex(audio_track->GetId(), 0);
    reader.SetSampleIndex(video_track->GetId(), 0);
    do {
        result = reader.ReadNextSample(audio_track->GetId(), sample, sample_data);
        if (AP4_SUCCEEDED(result)) {
            CHECK(offset < sample.GetOffset());
            offset = sample.GetOffset();
            audio_sample_count++;
            printf("size=%d, offset=%lld\n", (int)sample.GetSize(), sample.GetOffset());
        } else {
            printf("result=%d\n", result);
        }
    } while (AP4_SUCCEEDED(result));
    CHECK(result == AP4_ERROR_NOT_ENOUGH_SPACE);
    CHECK(reader.GetBufferFullness() != 0);
    
    // cleanup
    delete file;
    input->Release();

    return 0;                                            
}

