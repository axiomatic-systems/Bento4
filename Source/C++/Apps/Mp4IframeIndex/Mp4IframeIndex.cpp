/*****************************************************************
|
|    AP4 - MP4 I-Frame Indexer
|
|    Copyright 2002-2017 Axiomatic Systems, LLC
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
#define BANNER "MP4 Iframe Index - Version 1.0.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2017 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4iframeindex [options] <input> [<output>]\n"
            "  options:\n"
            "    --track <id>: ID of the video track\n"
            );
    exit(1);
}

/*----------------------------------------------------------------------
|   IndexTrack
+---------------------------------------------------------------------*/
static AP4_Result
IndexTrack(AP4_Track& track, const char** separator, AP4_ByteStream* output)
{
    AP4_Sample sample;
    for (unsigned int i=0; i<track.GetSampleCount(); i++) {
        // get the sample
        AP4_Result result = track.GetSample(i, sample);
        if (AP4_FAILED(result)) return result;
        
        if (sample.IsSync()) {
            AP4_Offset offset = sample.GetOffset();
            char workspace[256];
            AP4_FormatString(workspace, sizeof(workspace),
                             "%s{ \"size\": %u, \"offset\": %llu, \"fragmentStart\": 0 }",
                             *separator,
                             (unsigned int)sample.GetSize(),
                             (unsigned long long)offset);
            *separator = ",\n";
            output->WriteString(workspace);
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   IndexFragments
+---------------------------------------------------------------------*/
static AP4_Result
IndexFragments(AP4_Movie& movie, unsigned int track_id, AP4_ByteStream* stream, const char** separator, AP4_ByteStream* output)
{
    stream->Seek(0);
    AP4_LinearReader reader(movie, stream);
    reader.EnableTrack(track_id);
    
    AP4_Sample sample;
    AP4_Position last_fragment_position = (AP4_Position)(-1);
    for (unsigned int i=0; ; i++) {
        AP4_UI32 found_track_id = 0;
        AP4_Result result = reader.GetNextSample(sample, found_track_id);
        if (AP4_FAILED(result)) break;
        
        // only output the first sync sample of each fragment
        if (reader.GetCurrentFragmentPosition() == last_fragment_position) {
            continue;
        }
        if (found_track_id == track_id && sample.IsSync()) {
            AP4_Offset offset = sample.GetOffset();
            char workspace[256];
            AP4_FormatString(workspace, sizeof(workspace),
                             "%s{ \"size\": %u, \"offset\": %llu, \"fragmentStart\": %llu }",
                             *separator,
                             (unsigned int)sample.GetSize(),
                             (unsigned long long)offset,
                             (unsigned long long)reader.GetCurrentFragmentPosition());
            *separator = ",\n";
            output->WriteString(workspace);
        }
        last_fragment_position = reader.GetCurrentFragmentPosition();
    }

    return AP4_SUCCESS;
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
    const char*  input_filename  = NULL;
    const char*  output_filename = NULL;
    const char*  moov_filename   = NULL;
    unsigned int track_id = 0;

    ++argv;
    while (char* arg = *argv++) {
        if (!strcmp(arg, "--track")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --track option\n");
                return 1;
            }
            track_id = (unsigned int)strtoul(arg, NULL, 10);
        } else if (!strcmp(arg, "--fragments-info")) {
            moov_filename = *argv++;
            if (moov_filename == NULL) {
                fprintf(stderr, "ERROR: missing argument after --fragments-info option\n");
            }
        } else if (input_filename == NULL) {
            input_filename = arg;
        } else if (output_filename == NULL) {
            output_filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
            return 1;
        }
    }
    if (input_filename == NULL) {
        fprintf(stderr, "ERROR: input filename missing\n");
        return 1;
    }
    if (output_filename == NULL) {
        output_filename = "-stdout";
    }
    
    AP4_ByteStream* output = NULL;
    AP4_Result result = AP4_FileByteStream::Create(output_filename,
                                                   AP4_FileByteStream::STREAM_MODE_WRITE,
                                                   output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file %s (%d)\n", input_filename, result);
        return 1;
    }

    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(input_filename,
                                        AP4_FileByteStream::STREAM_MODE_READ,
                                        input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file %s (%d)\n", input_filename, result);
        return 1;
    }
    
    AP4_ByteStream* moov = NULL;
    if (moov_filename) {
        result = AP4_FileByteStream::Create(moov_filename,
                                            AP4_FileByteStream::STREAM_MODE_READ,
                                            moov);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot open fragments info file %s (%d)\n", moov_filename, result);
            return 1;
        }
    }
    
    AP4_File* input_file = new AP4_File(*input, true);
    AP4_Movie* input_movie = input_file->GetMovie();
    AP4_File* moov_file = moov ? new AP4_File(*moov, true) : NULL;
    AP4_Track* track = NULL;
    const char* separator = "";

    if (input_movie == NULL) {
        // we need a movie to get to the fragments
        if (moov_file == NULL) {
            fprintf(stderr, "ERROR: input file does not contain a movie (use --fragments-info option)\n");
            goto end;
        }
        input_movie = moov_file->GetMovie();
    }
    
    // select the track
    if (track_id == 0) {
        track = input_movie->GetTrack(AP4_Track::TYPE_VIDEO);
        if (track) {
            track_id = track->GetId();
        }
    } else {
        track = input_movie->GetTrack(track_id);
    }
        
    // check that we found the track
    if (!track) {
        fprintf(stderr, "ERROR: video track not found\n");
        goto end;
    }
        
    // check the track type
    if (track->GetType() != AP4_Track::TYPE_VIDEO) {
        fprintf(stderr, "ERROR: track ID %d is not a video track\n", track->GetId());
        goto end;
    }
    
    // start
    output->WriteString("[\n");
    
    // index the track
    result = IndexTrack(*track, &separator, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to index track (%d)\n", result);
    }
    
    // index the fragments
    result = IndexFragments(*input_movie, track_id, input, &separator, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to index fragments (%d)\n", result);
    }

    // end
    output->WriteString("\n]\n");

end:
    delete input_file;
    delete moov_file;
    if (moov) moov->Release();
    input->Release();
    output->Release();

    return result == AP4_SUCCESS ? 0 : 1;
}
