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
               "(c) 2002-2011 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
struct Options {
    unsigned int pmt_pid;
    unsigned int audio_pid;
    unsigned int video_pid;
    unsigned int segment;
    const char*  playlist;
    const char*  input;
    const char*  output;
} Options;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp42ts [options] <input> <output>\n"
            "Options:\n"
            "  --pmt-pid <pid>   (default: 0x100)\n"
            "  --audio-pid <pid> (default: 0x101)\n"
            "  --video-pid <pid> (default: 0x102)\n"
            "  --segment <segment-duration-in-seconds>\n"
            "    [the <output> name must be a 'printf' template]\n"
            "  --playlist <filename>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   OpenOutput
+---------------------------------------------------------------------*/
static AP4_ByteStream*
OpenOutput(const char* filename_pattern, unsigned int segment_number)
{
    AP4_ByteStream* output = NULL;
    char filename[1024];
    sprintf(filename, filename_pattern, segment_number);
    AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output (%d)\n", result);
        return NULL;
    }
    
    return output;
}

/*----------------------------------------------------------------------
|   WriteSamples
+---------------------------------------------------------------------*/
static void
WriteSamples(AP4_Mpeg2TsWriter&               writer,
             AP4_Track*                       audio_track, 
             AP4_Mpeg2TsWriter::SampleStream* audio_stream,
             AP4_Track*                       video_track, 
             AP4_Mpeg2TsWriter::SampleStream* video_stream)
{
    AP4_Sample      audio_sample;
    unsigned int    audio_sample_index = 0;
    AP4_Sample      video_sample;
    unsigned int    video_sample_index = 0;
    AP4_UI64        last_ts = 0;
    unsigned int    segment_number = 0;
    AP4_UI64        segment_duration = 0;
    AP4_ByteStream* output = NULL;
    AP4_ByteStream* playlist = NULL;
    char            string_buffer[32];
    
    // create the playlist file if needed 
    if (Options.playlist) {
        playlist = OpenOutput(Options.playlist, 0);
        if (playlist == NULL) return;
        playlist->WriteString("#EXTM3U\r\n");
        playlist->WriteString("#EXT-X-MEDIA-SEQUENCE:0\r\n");
        playlist->WriteString("#EXT-X-TARGETDURATION:");
        sprintf(string_buffer, "%d\r\n", Options.segment);
        playlist->WriteString(string_buffer);
    }
    
    for (;;) {
        bool sync_sample = false;
        AP4_Track* chosen_track= NULL;
        AP4_UI64 audio_ts = 0;
        if (audio_track) {
            audio_ts = AP4_ConvertTime(audio_sample.GetDts(), audio_track->GetMediaTimeScale(), 1000);
            if (AP4_SUCCEEDED(audio_track->GetSample(audio_sample_index, audio_sample))) {
                chosen_track = audio_track;
            }
            if (video_track == NULL) sync_sample = true;
        }
        AP4_UI64 video_ts = 0;
        if (video_track) {
            if (AP4_SUCCEEDED(video_track->GetSample(video_sample_index, video_sample))) {
                video_ts = AP4_ConvertTime(video_sample.GetDts(), video_track->GetMediaTimeScale(), 1000);
                if (video_sample.IsSync()) {
                    sync_sample = true;
                }
                if (audio_track) {
                    if (video_ts < audio_ts) {
                        chosen_track = video_track;
                    }
                } else {
                    chosen_track = video_track;
                }
            }
        }
        
        // check if we need to start a new segment
        if (Options.segment && sync_sample) {
            if (video_track) {
                segment_duration = video_ts - last_ts;
            } else {
                segment_duration = audio_ts - last_ts;
            }
            if (segment_duration > (AP4_UI64)Options.segment*1000) {
                if (video_track) {
                    last_ts = video_ts;
                } else {
                    last_ts = audio_ts;
                }
                if (output) {
                    if (Options.segment && playlist) {
                        sprintf(string_buffer, "#EXTINF:%d,\r\n", (unsigned int)(segment_duration/1000));
                        playlist->WriteString(string_buffer);
                        sprintf(string_buffer, Options.output, segment_number);
                        playlist->WriteString(string_buffer);
                        playlist->WriteString("\r\n");
                        printf("Segment %d, duration=%d\n", segment_number, (unsigned int)(segment_duration/1000));
                    }
                    output->Release();
                    output = NULL;
                    ++segment_number;
                }
            }
        }
        if (output == NULL) {
            output = OpenOutput(Options.output, segment_number);
            if (output == NULL) return;
            writer.WritePAT(*output);
            writer.WritePMT(*output);
        }
        
        // write the samples out
        if (chosen_track == NULL) break;
        if (chosen_track == audio_track) {
            audio_stream->WriteSample(audio_sample, 
                                      audio_track->GetSampleDescription(audio_sample.GetDescriptionIndex()), 
                                      video_track==NULL, 
                                      *output);
            audio_sample_index++;
        } else if (chosen_track == video_track) {
            video_stream->WriteSample(video_sample, 
                                      video_track->GetSampleDescription(video_sample.GetDescriptionIndex()),
                                      true, 
                                      *output);
            video_sample_index++;
        } else {
            break;
        }        
    }
    
    // finish the current segment
    if (playlist) {
        sprintf(string_buffer, "#EXTINF:%d,\r\n", (unsigned int)(segment_duration/1000000));
        playlist->WriteString(string_buffer);
        sprintf(string_buffer, Options.output, segment_number);
        playlist->WriteString(string_buffer);
        playlist->WriteString("\r\n#EXT-X-ENDLIST\r\n");
    }

    if (output)   output->Release();
    if (playlist) playlist->Release();
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
    
    // default options
    Options.segment   = 0;
    Options.pmt_pid   = 0x100;
    Options.audio_pid = 0x101;
    Options.video_pid = 0x102;
    Options.playlist  = NULL;
    Options.input     = NULL;
    Options.output    = NULL;
    
    // parse command line
    AP4_Result result;
    char** args = argv+1;
    while (const char* arg = *args++) {
        if (!strcmp(arg, "--segment")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment requires a number\n");
                return 1;
            }
            Options.segment = strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--pmt-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --pmt-pid requires a number\n");
                return 1;
            }
            Options.pmt_pid = strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--audio-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --audio-pid requires a number\n");
                return 1;
            }
            Options.audio_pid = strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--video-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --video-pid requires a number\n");
                return 1;
            }
            Options.video_pid = strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--playlist")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --playlist requires a filename\n");
                return 1;
            }
            Options.playlist = *args++;
        } else if (Options.input == NULL) {
            Options.input = arg;
        } else if (Options.output == NULL) {
            Options.output = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument\n");
            return 1;
        }
    }

	// create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(Options.input, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
        return 1;
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
    AP4_Mpeg2TsWriter writer(Options.pmt_pid);
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
                                       audio_stream,
                                       Options.audio_pid);
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
                                       video_stream,
                                       Options.video_pid);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "could not create video stream (%d)\n", result);
            goto end;
        }
    }
    
    WriteSamples(writer,
                 audio_track, audio_stream,
                 video_track, video_stream);

end:
    delete input_file;
    input->Release();

    return 0;
}

