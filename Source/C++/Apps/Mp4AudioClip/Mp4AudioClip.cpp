/*****************************************************************
|
|    AP4 - MP4 AUdio Clip Extractor
|
|    Copyright 2002-2013 Axiomatic Systems, LLC
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
#define BANNER "MP4 Audio Clip Extractor - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2013 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
        BANNER 
        "\n\n"
        "usage: mp4audioclip [options] <input> <output>\n"
        "Options:\n"
        "  --start <time>         : start time in milliseconds (default = 0)\n"
        "  --duration <duration>  : clip duration in milliseconds (default = until end)\n"
        );
    exit(1);
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc == 1) PrintUsageAndExit();

    // parse options
    const char*  input_filename  = NULL;
    const char*  output_filename = NULL;
    unsigned int start_time_ms   = 0;
    unsigned int duration_ms     = 0;
    AP4_Result   result;

    // parse the command line arguments
    char* arg;
    while ((arg = *++argv)) {
        if (!AP4_CompareStrings(arg, "--start")) {
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --start option\n");
                return 1;
            }
            start_time_ms = AP4_ParseIntegerU(arg);
        } else if (!AP4_CompareStrings(arg, "--duration")) {
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --duration option\n");
                return 1;
            }
            duration_ms = AP4_ParseIntegerU(arg);
        } else if (input_filename == NULL) {
            input_filename = arg;
        } else if (output_filename == NULL) {
            output_filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument (%s)\n", arg);
            return 1;
        }
    }

    // check arguments
    if (!input_filename) {
        fprintf(stderr, "ERROR: missing input file name argument\n");
        return 1;
    }
    if (!output_filename) {
        fprintf(stderr, "ERROR: missing output file name argument\n");
        return 1;
    }

    // create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }

    // parse the input
    AP4_File input_file(*input);
    AP4_Movie* input_movie = input_file.GetMovie();
    if (input_movie == NULL) {
        fprintf(stderr, "ERROR: cannot parse input (%d)\n", result);
        return 1;
    }
    
    // find the first audio track
    AP4_Track* input_track = input_movie->GetTrack(AP4_Track::TYPE_AUDIO);
    if (input_track == NULL) {
        fprintf(stderr, "ERROR: no audio track found\n");
        return 1;
    }
    
    // create a synthetic sample table to hold the output samples
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();
    
    // use the same sample description(s) in the output as what's in the input
    // (in almost all cases, there will only be one sample description)
    // NOTE: we don't transfer the ownership of the object
    for (unsigned int i=0; i<input_track->GetSampleDescriptionCount(); i++) {
        AP4_SampleDescription* sample_description = input_track->GetSampleDescription(i);
        if (!sample_description) {
            fprintf(stderr, "ERROR: invalid/unsupported sample description\n");
            return 1;
        }
        sample_table->AddSampleDescription(sample_description, false);
    }
    
    // the first output timestamp is 0
    AP4_UI64 dts = 0;

    // compute the maximum timestamp based on the duration
    AP4_UI64 max_dts;
    if (duration_ms == 0) {
        // go until the end
        max_dts = input_track->GetDuration();
    } else {
        max_dts = AP4_ConvertTime(duration_ms, 1000, input_track->GetMediaTimeScale());
    }
    
    // compute the first sample index
    AP4_Ordinal start_sample = 0;
    if (AP4_FAILED(input_track->GetSampleIndexForTimeStampMs(start_time_ms, start_sample))) {
        fprintf(stderr, "ERROR: start time out of range\n");
        return 1;
    }
    
    // add all the input samples to the sample table
    for (unsigned int i=start_sample; i<input_track->GetSampleCount(); i++) {
        AP4_Sample sample;
        if (AP4_FAILED(input_track->GetSample(i, sample))) break;
        
        // obtain the sample data stream in a variable so that we can release the reference
        AP4_ByteStream* sample_stream = sample.GetDataStream();
        
        // add a sample to the sample table
        sample_table->AddSample(*sample_stream,
                                sample.GetOffset(),
                                sample.GetSize(),
                                sample.GetDuration(),
                                sample.GetDescriptionIndex(),
                                dts,
                                0,
                                true);
        sample_stream->Release(); // the sample table has kept its own reference
        
        // update the current timestamp
        dts += sample.GetDuration();
        
        // stop if we've exceeded the duration
        if (dts > max_dts) break;
    }
    
    // compute the actual effective duration (could be less than what was asked for)
    duration_ms = (AP4_UI32)AP4_ConvertTime(dts, input_track->GetMediaTimeScale(), 1000);
    
    // create an output track
    AP4_Track* output_track =
        new AP4_Track(AP4_Track::TYPE_AUDIO,
                      sample_table,
                      1,                                                                    // track id
                      input_movie->GetTimeScale(),                                          // movie time scale
                      AP4_ConvertTime(duration_ms, 1000, input_movie->GetTimeScale()),      // track duration (in the timescale of the movie)
                      input_track->GetMediaTimeScale(),                                     // media time scale
                      AP4_ConvertTime(duration_ms, 1000, input_track->GetMediaTimeScale()), // media duration (in the timescale of the media)
                      "eng",                                                                // language
                      0, 0);                                                                // width, height

    
    // create the output movie
    AP4_Movie* output_movie = new AP4_Movie(input_movie->GetTimeScale());
    
    // create the output file
    AP4_File output_file(output_movie);

    // add the output track
    output_movie->AddTrack(output_track);

    // set the output file type
    AP4_UI32 compatible_brands[2] = {
        AP4_FILE_BRAND_ISOM,
        AP4_FILE_BRAND_MP42
    };
    output_file.SetFileType(AP4_FILE_BRAND_M4A_, 0, compatible_brands, 2);

    // create the output stream
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%s)\n", output_filename);
        return 1;
    }

    // write the output file
    AP4_FileWriter::Write(output_file, *output);

    // cleanup
    input->Release();
    output->Release();

    return 0;
}
