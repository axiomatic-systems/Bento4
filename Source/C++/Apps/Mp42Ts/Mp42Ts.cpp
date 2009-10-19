/*****************************************************************
|
|    AP4 - MP4 to MPEG2-TS File Converter
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
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 To MPEG2-TS File Converter - Version 1.0\n"\
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
            "\n\nusage: mp42ts [options] <input> <output>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   WriteSamples
+---------------------------------------------------------------------*/
static void
WriteSamples(AP4_Track*                       audio_track, 
             AP4_Mpeg2TsWriter::SampleStream* audio_stream,
             AP4_Track*                       video_track, 
             AP4_Mpeg2TsWriter::SampleStream* video_stream,
             AP4_ByteStream& output)
{
    AP4_Sample   audio_sample;
    unsigned int audio_sample_index = 0;
    AP4_Sample   video_sample;
    unsigned int video_sample_index = 0;

    for (;;) {
        AP4_Track* chosen_track= NULL;
        if (audio_track) {
            if (AP4_SUCCEEDED(audio_track->GetSample(audio_sample_index, audio_sample))) {
                chosen_track = audio_track;
            }
        }
        if (video_track) {
            if (AP4_SUCCEEDED(video_track->GetSample(video_sample_index, video_sample))) {
                if (audio_track) {
                    AP4_UI64 audio_ts = AP4_ConvertTime(audio_sample.GetDts(), audio_track->GetMediaTimeScale(), 1000000);
                    AP4_UI64 video_ts = AP4_ConvertTime(video_sample.GetDts(), video_track->GetMediaTimeScale(), 1000000);
                    if (video_ts < audio_ts) {
                        chosen_track = video_track;
                    }
                } else {
                    chosen_track = video_track;
                }
            }
        }

        if (chosen_track == audio_track) {
            audio_stream->WriteSample(audio_sample, 
                                      audio_track->GetSampleDescription(audio_sample.GetDescriptionIndex()), 
                                      video_track==NULL, 
                                      output);
            audio_sample_index++;
        } else if (chosen_track == video_track) {
            video_stream->WriteSample(video_sample, 
                                      video_track->GetSampleDescription(video_sample.GetDescriptionIndex()),
                                      true, 
                                      output);
            video_sample_index++;
        } else {
            break;
        }
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 3) {
        PrintUsageAndExit();
    }
        
    // parse command line
    AP4_Result result;
    char** args = argv+1;

	// create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(*args++, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
    }
    
	// create the output stream
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(*args++, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output (%d)\n", result);
    }

	// open the file
    AP4_File* input_file = new AP4_File(*input);   

    // get the movie
    AP4_SampleDescription* sample_description;
    AP4_Movie* movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        return 1;
    }

    // create an MPEG2 TS Writer
    AP4_Mpeg2TsWriter writer;
    AP4_Mpeg2TsWriter::SampleStream* audio_stream = NULL;
    AP4_Mpeg2TsWriter::SampleStream* video_stream = NULL;
    
    // get the audio and video tracks
    AP4_Track* audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    AP4_Track* video_track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    if (audio_track == NULL && video_track == NULL) {
        fprintf(stderr, "ERROR: no suitable tracks found\n");
        goto end;
    }

    // add the audio stream
    if (audio_track) {
        sample_description = audio_track->GetSampleDescription(0);
        if (sample_description == NULL) {
            fprintf(stderr, "ERROR: unable to parse audio sample description\n");
            goto end;
        }
        AP4_Debug("Audio Track:\n");
        AP4_Debug("  duration: %ld ms\n", audio_track->GetDurationMs());
        AP4_Debug("  sample count: %ld\n", audio_track->GetSampleCount());
        result = writer.SetAudioStream(audio_track->GetMediaTimeScale(),
                                       audio_stream);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "could not create audio stream (%d)\n", result);
            goto end;
        }
    }
    
    // add the video stream
    if (video_track) {
        sample_description = video_track->GetSampleDescription(0);
        if (sample_description == NULL) {
            fprintf(stderr, "ERROR: unable to parse video sample description\n");
            goto end;
        }
        AP4_Debug("Video Track:\n");
        AP4_Debug("  duration: %ld ms\n", video_track->GetDurationMs());
        AP4_Debug("  sample count: %ld\n", video_track->GetSampleCount());
        result = writer.SetVideoStream(video_track->GetMediaTimeScale(),
                                       video_stream);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "could not create video stream (%d)\n", result);
            goto end;
        }
    }
    
    writer.WritePAT(*output);
    writer.WritePMT(*output);

    WriteSamples(audio_track, audio_stream,
                 video_track, video_stream,
                 *output);

end:
    delete input_file;
    input->Release();
    output->Release();

    return 0;
}

