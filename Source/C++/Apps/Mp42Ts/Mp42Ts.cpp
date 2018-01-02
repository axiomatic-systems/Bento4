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
#define BANNER "MP4 To MPEG2-TS File Converter - Version 1.2\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2014 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
struct _Options {
    unsigned int pmt_pid;
    unsigned int audio_pid;
    unsigned int video_pid;
    bool         verbose;
    const char*  playlist;
    unsigned int playlist_hls_version;
    const char*  input;
    const char*  output;
    unsigned int segment_duration;
    unsigned int segment_duration_threshold;
} Options;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
static const unsigned int DefaultSegmentDurationThreshold = 50; // milliseconds

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
            "    [with this option, the <output> name must be a 'printf' template,\n"
            "     like \"seg-%cd.ts\"]\n"
            "  --segment-duration-threshold in ms (default = 50)\n"
            "    [only used with the --segment option]\n"
            "  --verbose\n"
            "  --playlist <filename>\n"
            "  --playlist-hls-version <n> (default=3)\n"
            ,'%');
    exit(1);
}

/*----------------------------------------------------------------------
|   SampleReader
+---------------------------------------------------------------------*/
class SampleReader 
{
public:
    virtual ~SampleReader() {}
    virtual AP4_Result ReadSample(AP4_Sample& sample, AP4_DataBuffer& sample_data) = 0;
};

/*----------------------------------------------------------------------
|   TrackSampleReader
+---------------------------------------------------------------------*/
class TrackSampleReader : public SampleReader
{
public:
    TrackSampleReader(AP4_Track& track) : m_Track(track), m_SampleIndex(0) {}
    AP4_Result ReadSample(AP4_Sample& sample, AP4_DataBuffer& sample_data);
    
private:
    AP4_Track&  m_Track;
    AP4_Ordinal m_SampleIndex;
};

/*----------------------------------------------------------------------
|   TrackSampleReader
+---------------------------------------------------------------------*/
AP4_Result 
TrackSampleReader::ReadSample(AP4_Sample& sample, AP4_DataBuffer& sample_data)
{
    if (m_SampleIndex >= m_Track.GetSampleCount()) return AP4_ERROR_EOS;
    return m_Track.ReadSample(m_SampleIndex++, sample, sample_data);
}

/*----------------------------------------------------------------------
|   FragmentedSampleReader
+---------------------------------------------------------------------*/
class FragmentedSampleReader : public SampleReader 
{
public:
    FragmentedSampleReader(AP4_LinearReader& fragment_reader, AP4_UI32 track_id) :
        m_FragmentReader(fragment_reader), m_TrackId(track_id) {
        fragment_reader.EnableTrack(track_id);
    }
    AP4_Result ReadSample(AP4_Sample& sample, AP4_DataBuffer& sample_data);
    
private:
    AP4_LinearReader& m_FragmentReader;
    AP4_UI32          m_TrackId;
};

/*----------------------------------------------------------------------
|   FragmentedSampleReader
+---------------------------------------------------------------------*/
AP4_Result 
FragmentedSampleReader::ReadSample(AP4_Sample& sample, AP4_DataBuffer& sample_data)
{
    return m_FragmentReader.ReadNextSample(m_TrackId, sample, sample_data);
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
|   ReadSample
+---------------------------------------------------------------------*/
static AP4_Result
ReadSample(SampleReader&   reader, 
           AP4_Track&      track,
           AP4_Sample&     sample,
           AP4_DataBuffer& sample_data, 
           double&         ts,
           bool&           eos)
{
    AP4_Result result = reader.ReadSample(sample, sample_data);
    if (AP4_FAILED(result)) {
        if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            return result;
        }
    }
    ts = (double)sample.GetDts()/(double)track.GetMediaTimeScale();
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   WriteSamples
+---------------------------------------------------------------------*/
static AP4_Result
WriteSamples(AP4_Mpeg2TsWriter&               writer,
             AP4_Track*                       audio_track,
             SampleReader*                    audio_reader, 
             AP4_Mpeg2TsWriter::SampleStream* audio_stream,
             AP4_Track*                       video_track,
             SampleReader*                    video_reader, 
             AP4_Mpeg2TsWriter::SampleStream* video_stream,
             unsigned int                     segment_duration_threshold)
{
    AP4_Sample        audio_sample;
    AP4_DataBuffer    audio_sample_data;
    unsigned int      audio_sample_count = 0;
    double            audio_ts = 0.0;
    bool              audio_eos = false;
    AP4_Sample        video_sample;
    AP4_DataBuffer    video_sample_data;
    unsigned int      video_sample_count = 0;
    double            video_ts = 0.0;
    bool              video_eos = false;
    double            last_ts = 0.0;
    unsigned int      segment_number = 0;
    double            segment_duration = 0.0;
    AP4_ByteStream*   output = NULL;
    AP4_ByteStream*   playlist = NULL;
    char              string_buffer[32];
    AP4_Result        result = AP4_SUCCESS;
    AP4_Array<double> segment_durations;
    
    // prime the samples
    if (audio_reader) {
        result = ReadSample(*audio_reader, *audio_track, audio_sample, audio_sample_data, audio_ts, audio_eos);
        if (AP4_FAILED(result)) goto end;
    }
    if (video_reader) {
        result = ReadSample(*video_reader, *video_track, video_sample, video_sample_data, video_ts, video_eos);
        if (AP4_FAILED(result)) goto end;
    }
    
    for (;;) {
        bool sync_sample = false;
        AP4_Track* chosen_track= NULL;
        if (audio_track && !audio_eos) {
            chosen_track = audio_track;
            if (video_track == NULL) sync_sample = true;
        }
        if (video_track && !video_eos) {
            if (audio_track) {
                if (video_ts <= audio_ts) {
                    chosen_track = video_track;
                }
            } else {
                chosen_track = video_track;
            }
            if (chosen_track == video_track && video_sample.IsSync()) {
                sync_sample = true;
            }
        }
        if (chosen_track == NULL) break;
        
        // check if we need to start a new segment
        if (Options.segment_duration && sync_sample) {
            if (video_track) {
                segment_duration = video_ts - last_ts;
            } else {
                segment_duration = audio_ts - last_ts;
            }
            if (segment_duration >= (double)Options.segment_duration - (double)segment_duration_threshold/1000.0) {
                if (video_track) {
                    last_ts = video_ts;
                } else {
                    last_ts = audio_ts;
                }
                if (output) {
                    segment_durations.Append(segment_duration);
                    if (Options.verbose) {
                        printf("Segment %d, duration=%.2f, %d audio samples, %d video samples\n",
                               segment_number, 
                               segment_duration,
                               audio_sample_count, 
                               video_sample_count);
                    }
                    output->Release();
                    output = NULL;
                    ++segment_number;
                    audio_sample_count = 0;
                    video_sample_count = 0;
                }
            }
        }
        if (output == NULL) {
            output = OpenOutput(Options.output, segment_number);
            if (output == NULL) return AP4_ERROR_CANNOT_OPEN_FILE;
            writer.WritePAT(*output);
            writer.WritePMT(*output);
        }
        
        // write the samples out and advance to the next sample
        if (chosen_track == audio_track) {
            result = audio_stream->WriteSample(audio_sample, 
                                               audio_sample_data,
                                               audio_track->GetSampleDescription(audio_sample.GetDescriptionIndex()), 
                                               video_track==NULL, 
                                               *output);
            if (AP4_FAILED(result)) return result;
            
            result = ReadSample(*audio_reader, *audio_track, audio_sample, audio_sample_data, audio_ts, audio_eos);
            if (AP4_FAILED(result)) return result;
            ++audio_sample_count;
        } else if (chosen_track == video_track) {
            result = video_stream->WriteSample(video_sample,
                                               video_sample_data, 
                                               video_track->GetSampleDescription(video_sample.GetDescriptionIndex()),
                                               true, 
                                               *output);
            if (AP4_FAILED(result)) return result;

            result = ReadSample(*video_reader, *video_track, video_sample, video_sample_data, video_ts, video_eos);
            if (AP4_FAILED(result)) return result;
            ++video_sample_count;
        } else {
            break;
        }        
    }
    
    // finish the last segment
    if (output) {
        if (video_track) {
            segment_duration = video_ts - last_ts;
        } else {
            segment_duration = audio_ts - last_ts;
        }
        segment_durations.Append(segment_duration);
        if (Options.verbose) {
            printf("Segment %d, duration=%.2f, %d audio samples, %d video samples\n",
                   segment_number, 
                   segment_duration,
                   audio_sample_count, 
                   video_sample_count);
        }
        output->Release();
        output = NULL;
        ++segment_number;
        audio_sample_count = 0;
        video_sample_count = 0;
    }

    // create the playlist file if needed 
    if (Options.playlist) {
        playlist = OpenOutput(Options.playlist, 0);
        if (playlist == NULL) return AP4_ERROR_CANNOT_OPEN_FILE;

        unsigned int target_duration = 0;
        for (unsigned int i=0; i<segment_durations.ItemCount(); i++) {
            if ((unsigned int)(segment_durations[i]+0.5) > target_duration) {
                target_duration = (unsigned int)segment_durations[i];
            }
        }

        playlist->WriteString("#EXTM3U\r\n");
        if (Options.playlist_hls_version > 1) {
            sprintf(string_buffer, "#EXT-X-VERSION:%d\r\n", Options.playlist_hls_version);
            playlist->WriteString(string_buffer);
        }
        playlist->WriteString("#EXT-X-MEDIA-SEQUENCE:0\r\n");
        playlist->WriteString("#EXT-X-TARGETDURATION:");
        sprintf(string_buffer, "%d\r\n\r\n", target_duration);
        playlist->WriteString(string_buffer);

        for (unsigned int i=0; i<segment_durations.ItemCount(); i++) {
            if (Options.playlist_hls_version >= 3) {
                sprintf(string_buffer, "#EXTINF:%f,\r\n", segment_durations[i]);
            } else {
                sprintf(string_buffer, "#EXTINF:%u,\r\n", (unsigned int)(segment_durations[i]+0.5));
            }
            playlist->WriteString(string_buffer);
            sprintf(string_buffer, Options.output, i);
            playlist->WriteString(string_buffer);
            playlist->WriteString("\r\n");
        }
                        
        playlist->WriteString("\r\n#EXT-X-ENDLIST\r\n");
        playlist->Release();
    }    

    if (Options.verbose) {
        if (video_track) {
            segment_duration = video_ts - last_ts;
        } else {
            segment_duration = audio_ts - last_ts;
        }
        printf("Conversion complete, duration=%.2f secs\n",
               segment_duration);
    }
    
end:
    if (output) output->Release();
    
    return result;
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
    Options.segment_duration           = 0;
    Options.pmt_pid                    = 0x100;
    Options.audio_pid                  = 0x101;
    Options.video_pid                  = 0x102;
    Options.verbose                    = false;
    Options.playlist                   = NULL;
    Options.playlist_hls_version       = 3;
    Options.input                      = NULL;
    Options.output                     = NULL;
    Options.segment_duration_threshold = DefaultSegmentDurationThreshold;
    
    // parse command line
    AP4_Result result;
    char** args = argv+1;
    while (const char* arg = *args++) {
        if (!strcmp(arg, "--segment")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment requires a number\n");
                return 1;
            }
            Options.segment_duration = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--segment-duration-threshold")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-duration-threshold requires a number\n");
                return 1;
            }
            Options.segment_duration_threshold = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--verbose")) {
            Options.verbose = true;
        } else if (!strcmp(arg, "--pmt-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --pmt-pid requires a number\n");
                return 1;
            }
            Options.pmt_pid = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--audio-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --audio-pid requires a number\n");
                return 1;
            }
            Options.audio_pid = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--video-pid")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --video-pid requires a number\n");
                return 1;
            }
            Options.video_pid = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--playlist")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --playlist requires a filename\n");
                return 1;
            }
            Options.playlist = *args++;
        } else if (!strcmp(arg, "--playlist-hls-version")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --playlist-hls-version requires a number\n");
                return 1;
            }
            Options.playlist_hls_version = (unsigned int)strtoul(*args++, NULL, 10);
            if (Options.playlist_hls_version ==0) {
                fprintf(stderr, "ERROR: --playlist-hls-version requires number > 0\n");
                return 1;
            }
        } else if (Options.input == NULL) {
            Options.input = arg;
        } else if (Options.output == NULL) {
            Options.output = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument\n");
            return 1;
        }
    }

    // check args
    if (Options.input == NULL) {
        fprintf(stderr, "ERROR: missing input file name\n");
        return 1;
    }
    if (Options.output == NULL) {
        fprintf(stderr, "ERROR: missing output file name\n");
        return 1;
    }
    
	// create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(Options.input, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
        return 1;
    }
    
	// open the file
    AP4_File* input_file = new AP4_File(*input, true);

    // get the movie
    AP4_SampleDescription* sample_description;
    AP4_Movie* movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        return 1;
    }

    // get the audio and video tracks
    AP4_Track* audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    AP4_Track* video_track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    if (audio_track == NULL && video_track == NULL) {
        fprintf(stderr, "ERROR: no suitable tracks found\n");
        delete input_file;
        input->Release();
        return 1;
    }

    // create the appropriate readers
    AP4_LinearReader* linear_reader = NULL;
    SampleReader*     audio_reader  = NULL;
    SampleReader*     video_reader  = NULL;
    if (movie->HasFragments()) {
        // create a linear reader to get the samples
        linear_reader = new AP4_LinearReader(*movie, input);
    
        if (audio_track) {
            linear_reader->EnableTrack(audio_track->GetId());
            audio_reader = new FragmentedSampleReader(*linear_reader, audio_track->GetId());
        }
        if (video_track) {
            linear_reader->EnableTrack(video_track->GetId());
            video_reader = new FragmentedSampleReader(*linear_reader, video_track->GetId());
        }
    } else {
        if (audio_track) {
            audio_reader = new TrackSampleReader(*audio_track);
        }
        if (video_track) {
            video_reader = new TrackSampleReader(*video_track);
        }
    }
    
    // create an MPEG2 TS Writer
    AP4_Mpeg2TsWriter writer(Options.pmt_pid);
    AP4_Mpeg2TsWriter::SampleStream* audio_stream = NULL;
    AP4_Mpeg2TsWriter::SampleStream* video_stream = NULL;
    
    // add the audio stream
    if (audio_track) {
        sample_description = audio_track->GetSampleDescription(0);
        if (sample_description == NULL) {
            fprintf(stderr, "ERROR: unable to parse audio sample description\n");
            goto end;
        }

        unsigned int stream_type = 0;
        unsigned int stream_id   = 0;
        if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_MP4A) {
            stream_type = AP4_MPEG2_STREAM_TYPE_ISO_IEC_13818_7;
            stream_id   = AP4_MPEG2_TS_DEFAULT_STREAM_ID_AUDIO;
        } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AC_3 ||
                   sample_description->GetFormat() == AP4_SAMPLE_FORMAT_EC_3) {
            stream_type = AP4_MPEG2_STREAM_TYPE_ATSC_AC3;
            stream_id   = AP4_MPEG2_TS_STREAM_ID_PRIVATE_STREAM_1;
        } else {
            fprintf(stderr, "ERROR: audio codec not supported\n");
            return 1;
        }

        result = writer.SetAudioStream(audio_track->GetMediaTimeScale(),
                                       stream_type,
                                       stream_id,
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
        
        // decide on the stream type
        unsigned int stream_type = 0;
        unsigned int stream_id   = AP4_MPEG2_TS_DEFAULT_STREAM_ID_VIDEO;
        if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC1 ||
            sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC2 ||
            sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC3 ||
            sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC4 ||
            sample_description->GetFormat() == AP4_SAMPLE_FORMAT_DVAV ||
            sample_description->GetFormat() == AP4_SAMPLE_FORMAT_DVA1) {
            stream_type = AP4_MPEG2_STREAM_TYPE_AVC;
        } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_HEV1 ||
                   sample_description->GetFormat() == AP4_SAMPLE_FORMAT_HVC1 ||
                   sample_description->GetFormat() == AP4_SAMPLE_FORMAT_DVHE ||
                   sample_description->GetFormat() == AP4_SAMPLE_FORMAT_DVH1) {
            stream_type = AP4_MPEG2_STREAM_TYPE_HEVC;
        } else {
            fprintf(stderr, "ERROR: video codec not supported\n");
            return 1;
        }
        result = writer.SetVideoStream(video_track->GetMediaTimeScale(),
                                       stream_type,
                                       stream_id,
                                       video_stream,
                                       Options.video_pid);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "could not create video stream (%d)\n", result);
            goto end;
        }
    }
    
    result = WriteSamples(writer,
                          audio_track, audio_reader, audio_stream,
                          video_track, video_reader, video_stream,
                          Options.segment_duration_threshold);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to write samples (%d)\n", result);
    }

end:
    delete input_file;
    input->Release();
    delete linear_reader;
    delete audio_reader;
    delete video_reader;
    
    return result == AP4_SUCCESS?0:1;
}

