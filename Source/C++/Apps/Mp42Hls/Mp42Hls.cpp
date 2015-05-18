/*****************************************************************
|
|    AP4 - MP4 to HLS File Converter
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
#include "Ap4StreamCipher.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 To HLS File Converter - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2015 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
typedef enum {
    ENCRYPTION_MODE_NONE,
    ENCRYPTION_MODE_AES_128,
    ENCRYPTION_MODE_AES_SAMPLE
} EncryptionMode;

typedef enum {
    ENCRYPTION_IV_MODE_SEQUENCE,
    ENCRYPTION_IV_MODE_RANDOM
} EncryptionIvMode;

struct Options {
    const char*      input;
    bool             verbose;
    unsigned int     hls_version;
    unsigned int     pmt_pid;
    unsigned int     audio_pid;
    unsigned int     video_pid;
    const char*      index_filename;
    const char*      segment_filename_template;
    const char*      segment_url_template;
    unsigned int     segment_duration;
    unsigned int     segment_duration_threshold;
    const char*      encryption_key_hex;
    AP4_UI08         encryption_key[16];
    EncryptionMode   encryption_mode;
    EncryptionIvMode encryption_iv_mode;
    const char*      encryption_key_uri;
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
            "\n\nusage: mp42hls [options] <input>\n"
            "Options:\n"
            "  --verbose\n"
            "  --hls-version <n> (default: 3)\n"
            "  --pmt-pid <pid>\n"
            "    PID to use for the PMT (default: 0x100)\n"
            "  --audio-pid <pid>\n"
            "    PID to use for audio packets (default: 0x101)\n"
            "  --video-pid <pid>\n"
            "    PID to use for video packets (default: 0x102)\n"
            "  --segment-duration <n>\n"
            "    Target segment duration in seconds (default: 10)\n"
            "  --segment-duration-threshold <t>\n"
            "    Segment duration threshold in milliseconds (default: 50)\n"
            "  --index-filename <filename>\n"
            "    Filename to use for the playlist/index (default: stream.m3u8)\n"
            "  --segment-filename-template <pattern>\n"
            "    Filename pattern to use for the segments. Use a printf-style pattern with\n"
            "    one number field for the segment number (default: segment-%%d.ts)\n"
            "  --segment-url-template <pattern>\n"
            "    URL pattern to use for the segments. Use a printf-style pattern with\n"
            "    one number field for the segment number. (may be a realtive or absolute URI).\n"
            "    (default: segment-%%d.ts)\n"
            "  --encryption-mode <mode>\n"
            "    Encryption mode (only used when --encryption-key is specified). (default: AES-128)\n"
            "  --encryption-key <key>\n"
            "    Encryption key in hexadecimal (default: no encryption)\n"
            "  --encryption-iv-mode <mode>\n"
            "    Encryption IV mode: 'sequence' or 'random' (default: sequence)\n"
            "  --encryption-key-uri <uri>\n"
            "    Encryption key URI (may be a realtive or absolute URI). (default: key.bin)\n"
            );
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
    char filename[4096];
    sprintf(filename, filename_pattern, segment_number);
    AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output (%d)\n", result);
        return NULL;
    }
    
    return output;
}

/*----------------------------------------------------------------------
|   EncryptingStream
+---------------------------------------------------------------------*/
class EncryptingStream: public AP4_ByteStream {
public:
    static AP4_Result Create(const AP4_UI08* key, const AP4_UI08* iv, AP4_ByteStream* output, EncryptingStream*& stream);
    virtual AP4_Result ReadPartial(void* , AP4_Size, AP4_Size&) {
        return AP4_ERROR_NOT_SUPPORTED;
    }
    virtual AP4_Result WritePartial(const void* buffer,
                                    AP4_Size    bytes_to_write, 
                                    AP4_Size&   bytes_written) {
        AP4_UI08* out = new AP4_UI08[bytes_to_write+16];
        AP4_Size  out_size = bytes_to_write+16;
        AP4_Result result = m_StreamCipher->ProcessBuffer((const AP4_UI08*)buffer,
                                                          bytes_to_write,
                                                          out,
                                                          &out_size);
        if (AP4_SUCCEEDED(result)) {
            result = m_Output->Write(out, out_size);
            bytes_written = bytes_to_write;
        } else {
            bytes_written = 0;
        }
        delete[] out;
        return result;
    }
    virtual AP4_Result Flush() {
        AP4_UI08 trailer[16];
        AP4_Size trailer_size = sizeof(trailer);
        AP4_Result result = m_StreamCipher->ProcessBuffer(NULL, 0, trailer, &trailer_size, true);
        if (AP4_SUCCEEDED(result) && trailer_size) {
            m_Output->Write(trailer, trailer_size);
        }
        
        return AP4_SUCCESS;
    }
    virtual AP4_Result Seek(AP4_Position) { return AP4_ERROR_NOT_SUPPORTED; }
    virtual AP4_Result Tell(AP4_Position& position) { position = 0; return AP4_ERROR_NOT_SUPPORTED; }
    virtual AP4_Result GetSize(AP4_LargeSize& size) { size = 0; return AP4_ERROR_NOT_SUPPORTED; }
    
    void AddReference() {
        ++m_ReferenceCount;
    }
    void Release() {
        if (--m_ReferenceCount == 0) {
            delete this;
        }
    }

private:
    EncryptingStream(AP4_CbcStreamCipher* stream_cipher, AP4_ByteStream* output):
        m_ReferenceCount(1),
        m_StreamCipher(stream_cipher),
        m_Output(output) {
        output->AddReference();
    }
    ~EncryptingStream() {
        m_Output->Release();
        delete m_StreamCipher;
    }
    unsigned int         m_ReferenceCount;
    AP4_CbcStreamCipher* m_StreamCipher;
    AP4_ByteStream*      m_Output;
};

/*----------------------------------------------------------------------
|   EncryptingStream::Create
+---------------------------------------------------------------------*/
AP4_Result
EncryptingStream::Create(const AP4_UI08* key, const AP4_UI08* iv, AP4_ByteStream* output, EncryptingStream*& stream) {
    stream = NULL;
    AP4_BlockCipher* block_cipher = NULL;
    AP4_Result result = AP4_DefaultBlockCipherFactory::Instance.CreateCipher(AP4_BlockCipher::AES_128,
                                                                             AP4_BlockCipher::ENCRYPT,
                                                                             AP4_BlockCipher::CBC,
                                                                             NULL,
                                                                             key,
                                                                             16,
                                                                             block_cipher);
    if (AP4_FAILED(result)) return result;
    AP4_CbcStreamCipher* stream_cipher = new AP4_CbcStreamCipher(block_cipher);
    stream_cipher->SetIV(iv);
    stream = new EncryptingStream(stream_cipher, output);
    
    return AP4_SUCCESS;
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
    char              string_buffer[4096];
    AP4_Result        result = AP4_SUCCESS;
    AP4_Array<double> segment_durations;
    
    // prime the samples
    if (audio_reader) {
        result = ReadSample(*audio_reader, *audio_track, audio_sample, audio_sample_data, audio_ts, audio_eos);
        if (AP4_FAILED(result)) return result;
    }
    if (video_reader) {
        result = ReadSample(*video_reader, *video_track, video_sample, video_sample_data, video_ts, video_eos);
        if (AP4_FAILED(result)) return result;
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
                    output->Flush();
                    output->Release();
                    output = NULL;
                    ++segment_number;
                    audio_sample_count = 0;
                    video_sample_count = 0;
                }
            }
        }
        if (output == NULL) {
            output = OpenOutput(Options.segment_filename_template, segment_number);
            if (output == NULL) return AP4_ERROR_CANNOT_OPEN_FILE;
            if (Options.encryption_mode == ENCRYPTION_MODE_AES_128) {
                EncryptingStream* encrypting_stream = NULL;
                AP4_UI08 iv[16];
                AP4_SetMemory(iv, 0, sizeof(iv));
                if (Options.encryption_iv_mode == ENCRYPTION_IV_MODE_SEQUENCE) {
                    AP4_BytesFromUInt32BE(&iv[12], segment_number);
                }
                result = EncryptingStream::Create(Options.encryption_key, iv, output, encrypting_stream);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: failed to create encrypting stream (%d)\n", result);
                    return 1;
                }
                output->Release();
                output = encrypting_stream;
            }
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
        output->Flush();
        output->Release();
        output = NULL;
        ++segment_number;
        audio_sample_count = 0;
        video_sample_count = 0;
    }

    // create the playlist/index file
    playlist = OpenOutput(Options.index_filename, 0);
    if (playlist == NULL) return AP4_ERROR_CANNOT_OPEN_FILE;

    unsigned int target_duration = 0;
    for (unsigned int i=0; i<segment_durations.ItemCount(); i++) {
        if ((unsigned int)(segment_durations[i]+0.5) > target_duration) {
            target_duration = segment_durations[i];
        }
    }

    playlist->WriteString("#EXTM3U\r\n");
    if (Options.hls_version > 1) {
        sprintf(string_buffer, "#EXT-X-VERSION:%d\r\n", Options.hls_version);
        playlist->WriteString(string_buffer);
    }
    playlist->WriteString("#EXT-X-PLAYLIST-TYPE:VOD\r\n");
    playlist->WriteString("#EXT-X-INDEPENDENT-SEGMENTS\r\n");
    playlist->WriteString("#EXT-X-TARGETDURATION:");
    sprintf(string_buffer, "%d\r\n", target_duration);
    playlist->WriteString(string_buffer);
    playlist->WriteString("#EXT-X-MEDIA-SEQUENCE:0\r\n");

    if (Options.encryption_mode == ENCRYPTION_MODE_AES_128) {
        playlist->WriteString("#EXT-X-KEY:METHOD=AES-128,URI=\"");
        playlist->WriteString(Options.encryption_key_uri);
        playlist->WriteString("\"\r\n");
    }
    for (unsigned int i=0; i<segment_durations.ItemCount(); i++) {
        if (Options.hls_version >= 3) {
            sprintf(string_buffer, "#EXTINF:%f,\r\n", segment_durations[i]);
        } else {
            sprintf(string_buffer, "#EXTINF:%u,\r\n", (unsigned int)(segment_durations[i]+0.5));
        }
        playlist->WriteString(string_buffer);
        sprintf(string_buffer, Options.segment_filename_template, i);
        playlist->WriteString(string_buffer);
        playlist->WriteString("\r\n");
    }
                    
    playlist->WriteString("#EXT-X-ENDLIST\r\n");
    playlist->Release();

    if (Options.verbose) {
        if (video_track) {
            segment_duration = video_ts - last_ts;
        } else {
            segment_duration = audio_ts - last_ts;
        }
        printf("Conversion complete, duration=%.2f secs\n", segment_duration);
    }
    
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
    Options.input                      = NULL;
    Options.verbose                    = false;
    Options.hls_version                = 3;
    Options.pmt_pid                    = 0x100;
    Options.audio_pid                  = 0x101;
    Options.video_pid                  = 0x102;
    Options.index_filename             = "stream.m3u8";
    Options.segment_filename_template  = "segment-%d.ts";
    Options.segment_url_template       = "segment-%d.ts";
    Options.segment_duration           = 10;
    Options.segment_duration_threshold = DefaultSegmentDurationThreshold;
    Options.encryption_key_hex         = NULL;
    Options.encryption_mode            = ENCRYPTION_MODE_NONE;
    Options.encryption_iv_mode         = ENCRYPTION_IV_MODE_SEQUENCE;
    Options.encryption_key_uri         = "key.bin";
    
    // parse command line
    AP4_Result result;
    char** args = argv+1;
    while (const char* arg = *args++) {
        if (!strcmp(arg, "--verbose")) {
            Options.verbose = true;
        } else if (!strcmp(arg, "--hls-version")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --hls-version requires a number\n");
                return 1;
            }
            Options.hls_version = strtoul(*args++, NULL, 10);
            if (Options.hls_version ==0) {
                fprintf(stderr, "ERROR: --hls-version requires number > 0\n");
                return 1;
            }
        } else if (!strcmp(arg, "--segment-duration")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-duration requires a number\n");
                return 1;
            }
            Options.segment_duration = strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--segment-duration-threshold")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-duration-threshold requires a number\n");
                return 1;
            }
            Options.segment_duration_threshold = strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--segment-filename-template")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-filename-template requires an argument\n");
                return 1;
            }
            Options.segment_filename_template = *args++;
        } else if (!strcmp(arg, "--segment-url-template")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --segment-url-template requires an argument\n");
                return 1;
            }
            Options.segment_url_template = *args++;
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
        } else if (!strcmp(arg, "--index-filename")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --index-filename requires a filename\n");
                return 1;
            }
            Options.index_filename = *args++;
        } else if (!strcmp(arg, "--encryption-key")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-key requires an argument\n");
                return 1;
            }
            Options.encryption_key_hex = *args++;
            result = AP4_ParseHex(Options.encryption_key_hex, Options.encryption_key, 16);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: invalid hex key\n");
                return 1;
            }
            if (Options.encryption_mode == ENCRYPTION_MODE_NONE) {
                Options.encryption_mode = ENCRYPTION_MODE_AES_128;
            }
        } else if (!strcmp(arg, "--encryption-mode")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-mode requires an argument\n");
                return 1;
            }
            if (strncmp(*args, "aes-128", 7) == 0) {
                Options.encryption_mode = ENCRYPTION_MODE_AES_128;
            } else {
                fprintf(stderr, "ERROR: unknown encryption mode\n");
                return 1;
            }
            ++args;
        } else if (!strcmp(arg, "--encryption-iv")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-iv requires an argument\n");
                return 1;
            }
            if (strncmp(*args, "sequence", 8) == 0) {
                Options.encryption_iv_mode = ENCRYPTION_IV_MODE_SEQUENCE;
            } else if (strncmp(*args, "random", 6) == 0) {
                Options.encryption_iv_mode = ENCRYPTION_IV_MODE_RANDOM;
            } else {
                fprintf(stderr, "ERROR: unknown encryption IV mode\n");
                return 1;
            }
            ++args;
        } else if (!strcmp(arg, "--encryption-key-uri")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: --encryption-key-uri requires an argument\n");
                return 1;
            }
            Options.encryption_key_uri = *args++;
        } else if (Options.input == NULL) {
            Options.input = arg;
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
    if (Options.encryption_mode != ENCRYPTION_MODE_NONE && Options.encryption_key_hex == NULL) {
        fprintf(stderr, "ERROR: no encryption key specified\n");
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
    AP4_File* input_file = new AP4_File(*input, AP4_DefaultAtomFactory::Instance, true);   

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
            sample_description->GetFormat() == AP4_SAMPLE_FORMAT_AVC4) {
            stream_type = AP4_MPEG2_STREAM_TYPE_AVC;
        } else if (sample_description->GetFormat() == AP4_SAMPLE_FORMAT_HEV1 ||
                   sample_description->GetFormat() == AP4_SAMPLE_FORMAT_HVC1) {
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

