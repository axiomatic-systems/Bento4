/*****************************************************************
|
|    AP4 - AP4_FragmentCreator test
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
#include <stdio.h>
#include <stdlib.h>

#include "Ap4.h"
#if defined(_WIN32)
#define snprintf _snprintf
#endif

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc != 8) {
        printf("usage: fragmentcreatortest audio|video <media-input-filename> <track-id> <frames-per-segment>|<segment-duration> <frames-per-second>|0 <output-media-segment-filename-pattern> <output-init-segment-filename>\n");
        return 1;
    }

    // init the variables
    const char*  input_filename       = argv[2];
    unsigned int track_id             = (unsigned int)strtoul(argv[3], NULL, 10);
    double       segment_duration     = strtod(argv[4], NULL);
    double       frames_per_second    = strtod(argv[5], NULL);
    const char*  output_media_segment_filename_pattern = argv[6];
    const char*  output_init_segment_filename          = argv[7];
    AP4_Result   result;

    AP4_ByteStream* input_stream = NULL;
    result = AP4_FileByteStream::Create(input_filename,
                                        AP4_FileByteStream::STREAM_MODE_READ, 
                                        input_stream);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open video input file (%d)\n", result);
        return 1;
    }
    
    // instantiate a video segment builder or an audio sergment builder
    AP4_AvcSegmentBuilder* video_builder = NULL;
    AP4_AacSegmentBuilder* audio_builder = NULL;
    AP4_FeedSegmentBuilder* feed_builder = NULL;
    if (!strcmp(argv[1], "video")) {
        video_builder = new AP4_AvcSegmentBuilder(track_id, frames_per_second);
        feed_builder = video_builder;
    } else {
        audio_builder = new AP4_AacSegmentBuilder(track_id);
        feed_builder = audio_builder;
    }
    
    // parse the input until the end of the stream
    unsigned int segment_count = 0;
    for (;;) {
        bool eos;
        unsigned char input_buffer[4096];
        AP4_Size bytes_in_buffer = 0;
        result = input_stream->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        do {
            AP4_Size bytes_consumed = 0;
            result = feed_builder->Feed(eos?NULL:&input_buffer[offset],
                                        bytes_in_buffer,
                                        bytes_consumed);
            if (result < 0) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
        
            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
            
            bool flush = false;
            if (video_builder) {
                if (video_builder->GetSamples().ItemCount() >= (unsigned int)segment_duration) {
                    flush = true;
                }
            } else {
                double target_time = (segment_count+1)*segment_duration;
                double elapsed_time = (double)(audio_builder->GetMediaDuration()+audio_builder->GetMediaStartTime())/(double)audio_builder->GetTimescale();
                if (elapsed_time >= target_time) {
                    flush = true;
                }
            }
            if (flush) {
                unsigned int max_name_size = (unsigned int)strlen(output_media_segment_filename_pattern)+256;
                char* media_segment_filename = new char[max_name_size+1];
                snprintf(media_segment_filename, max_name_size, output_media_segment_filename_pattern, segment_count);
                AP4_ByteStream* media_segment_stream = NULL;
                
                result = AP4_FileByteStream::Create(media_segment_filename, AP4_FileByteStream::STREAM_MODE_WRITE, media_segment_stream);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: cannot create media segment file (%d)\n", result);
                    return 1;
                }
                
                feed_builder->WriteMediaSegment(*media_segment_stream, segment_count);
                
                delete[] media_segment_filename;
                media_segment_stream->Release();
                ++segment_count;
            }
        } while (bytes_in_buffer || result > 0);
        if (eos) break;
    }

    AP4_ByteStream* init_segment_stream = NULL;
    result = AP4_FileByteStream::Create(output_init_segment_filename, AP4_FileByteStream::STREAM_MODE_WRITE, init_segment_stream);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot create init segment file (%d)\n", result);
        return 1;
    }
    feed_builder->WriteInitSegment(*init_segment_stream);
    init_segment_stream->Release();
    
    // cleanup and exit
    if (input_stream) input_stream->Release();
    delete video_builder;
    delete audio_builder;

    return 0;
}
