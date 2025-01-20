/*****************************************************************
|
|    AP4 - Elementary Stream Muliplexer
|
|    Copyright 2002-2020 Axiomatic Systems, LLC
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
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "Ap4.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 Elementary Stream Multiplexer - Version 3.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-20020 Axiomatic Systems, LLC"

const unsigned int AP4_MUX_DEFAULT_VIDEO_FRAME_RATE = 24;
const unsigned int AP4_MUX_READ_BUFFER_SIZE         = 65536;

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
static struct {
    bool verbose;
} Options;

/*----------------------------------------------------------------------
|   SampleOrder
+---------------------------------------------------------------------*/
struct SampleOrder {
    SampleOrder(AP4_UI32 decode_order, AP4_UI32 display_order) :
        m_DecodeOrder(decode_order),
        m_DisplayOrder(display_order) {}
    AP4_UI32 m_DecodeOrder;
    AP4_UI32 m_DisplayOrder;
};

/*----------------------------------------------------------------------
|   Parameter
+---------------------------------------------------------------------*/
struct Parameter {
    Parameter(const char* name, const char* value) :
        m_Name(name),
        m_Value(value) {}
    AP4_String m_Name;
    AP4_String m_Value;
};

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4mux [options] --track [<type>:]<input>[#<params>] [--track [<type>:]<input>[#<params>] ...] <output>\n"
            "\n"
            "  <params>, when specified, are expressd as a comma-separated list of\n"
            "  one or more <name>=<value> parameters\n"
            "\n"
            "Supported types:\n"
            "  h264: H264/AVC NAL units\n"
            "    optional params:\n"
            "      dv_profile: integer number for Dolby vision profile ID (valid value: 9)\n"
            "      dv_bc: integer number for Dolby vision BL signal cross-compatibility ID (must be 2 if dv_profile is set to 9)\n"
            "      frame_rate: floating point number in frames per second (default=24.0)\n"
            "      format: avc1 (default) or avc3  for AVC tracks and Dolby Vision back-compatible tracks\n"
            "  h265: H265/HEVC NAL units\n"
            "    optional params:\n"
            "      dv_profile: integer number for Dolby vision profile ID (valid value: 5,8)\n"
            "      dv_bc: integer number for Dolby vision BL signal cross-compatibility ID (mandatory if dv_profile is set to 8)\n"
            "      frame_rate: floating point number in frames per second (default=24.0)\n"
            "      format: hev1 or hvc1 (default) for HEVC tracks and Dolby Vision backward-compatible tracks\n"
            "              dvhe or dvh1 (default) for Dolby vision tracks\n"
            "  aac:  AAC in ADTS format\n"
            "  ac3:  Dolby Digital\n"
            "  ec3:  Dolby Digital Plus\n"
            "  ac4:  Dolby AC-4\n"

            "  mp4:  MP4 track(s) from an MP4 file\n"
            "    optional params:\n"
            "      track: audio, video, or integer track ID (default=all tracks)\n"
            "\n"
            "Common optional parameters for all types:\n"
            "  language: language code (3-character ISO 639-2 Alpha-3 code)\n"
            "\n"
            "If no type is specified for an input, the type will be inferred from the file extension\n"
            "\n"
            "Options:\n"
            "  --verbose: show more details\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ParseParameters
+---------------------------------------------------------------------*/
static AP4_Result
ParseParameters(const char* params_str, AP4_Array<Parameter>& parameters)
{
    AP4_Array<AP4_String> params;
    const char* cursor = params_str;
    const char* start = params_str;
    do {
        if (*cursor == ',' || *cursor == '\0') {
            AP4_String param;
            param.Assign(start, (unsigned int)(cursor-start));
            params.Append(param);
            if (*cursor == ',') {
                start = cursor+1;
            }
        }
    } while (*cursor++);
    
    for (unsigned int i=0; i<params.ItemCount(); i++) {
        AP4_String& param = params[i];
        AP4_String name;
        AP4_String value;
        int equal = param.Find('=');
        if (equal >= 0) {
            name.Assign(param.GetChars(), equal);
            value = param.GetChars()+equal+1;
        } else {
            name = param;
        }
        parameters.Append(Parameter(name.GetChars(), value.GetChars()));
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   ComputeDoviLevel
+---------------------------------------------------------------------*/
static AP4_Result
ComputeDoviLevel(AP4_UI32 video_width, AP4_UI32 video_height, double frame_rate, AP4_UI32& dv_level)
{
    double level = video_width * video_height * frame_rate;

    if (level <= 1280*720*24) {
        dv_level = 1;
    } else if (level<= 1280*720*30) {
        dv_level = 2;
    } else if (level <= 1920*1080*24) {
        dv_level = 3;
    } else if (level <= 1920*1080*30) {
        dv_level = 4;
    } else if (level <= 1920*1080*60) {
        dv_level = 5;
    } else if (level <= 3840*2160*24) {
        dv_level = 6;
    } else if (level <= 3840*2160*30) {
        dv_level = 7;
    } else if (level <= 3840*2160*48) {
        dv_level = 8;
    } else if (level <= 3840*2160*60) {
        dv_level = 9;
    } else if (level <= 3840*2160*120) {
        if (video_width == 7680) {
            dv_level = 11;
        } else {
            dv_level = 10;
        }
    } else if (level <= 7680*4320*60) {
        dv_level = 12;
    } else if (level <= (double)7680*4320*120) {
        dv_level = 13;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   CheckDoviInputParameters
+---------------------------------------------------------------------*/
static AP4_Result
CheckDoviInputParameters(AP4_Array<Parameter>& parameters)
{
    AP4_UI32 profile = 0;
    AP4_UI32 bc = -1;
    AP4_UI32 format = 0;

    //check: profile set correctly
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "dv_profile") {
            profile = atoi(parameters[i].m_Value.GetChars());
            if ((profile != 5) && (profile != 8) && (profile != 9)) {
                fprintf(stderr, "ERROR: invalid dolby vision profile id\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    //check: bl signal compatibility id value range
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "dv_bc") {
            bc = atoi(parameters[i].m_Value.GetChars());
            if (bc > 7) {
                fprintf(stderr, "ERROR: invalid dolby vision bl signal compatibility id\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "format") {
            if (parameters[i].m_Value == "hvc1") {
                format = AP4_SAMPLE_FORMAT_HVC1;
            } else if (parameters[i].m_Value == "hev1") {
                format = AP4_SAMPLE_FORMAT_HEV1;
            } else if (parameters[i].m_Value == "dvh1") {
                format = AP4_SAMPLE_FORMAT_DVH1;
            } else if (parameters[i].m_Value == "dvhe") {
                format = AP4_SAMPLE_FORMAT_DVHE;
            } else if (parameters[i].m_Value == "avc1") {
                format = AP4_SAMPLE_FORMAT_AVC1;
            } else if (parameters[i].m_Value == "avc3") {
                format = AP4_SAMPLE_FORMAT_AVC3;
            } else if (parameters[i].m_Value == "dvav") {
                format = AP4_SAMPLE_FORMAT_DVAV;
            } else if (parameters[i].m_Value == "dva1") {
                format = AP4_SAMPLE_FORMAT_DVA1;
            } else {
                fprintf(stderr, "ERROR: format name is invalid\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    //check: sample entry box name is set correctly

    if (format) {
        if (profile == 5) {
            // profile 5 does not compatible with other profile, only use dvhe or dvh1
            if ((format != AP4_SAMPLE_FORMAT_DVHE) && (format != AP4_SAMPLE_FORMAT_DVH1)) {
                fprintf(stderr, "ERROR: sample entry name is not correct for profile 5\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        } else if (profile == 8) {
            // profile 8 has CCID with 1, 2, 4, may be compatible with HDR10, SDR or HLG, should
            // not use dvhe or dvh1
            if ((format != AP4_SAMPLE_FORMAT_HVC1) && (format != AP4_SAMPLE_FORMAT_HEV1)) {
                fprintf(stderr, "ERROR: sample entry name is not correct for profile 8\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        } else if (profile == 9) {
            // profile 9 only has CCID with 2, which is SDR compliant, should not use dvav or dva1
            if ((format != AP4_SAMPLE_FORMAT_AVC1) && (format != AP4_SAMPLE_FORMAT_AVC3)) {
                fprintf(stderr, "ERROR: sample entry name is not correct for profile 9\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    //check: for profile 8/9, bl signal compatibility id must be set
    if ((bc == (AP4_UI32)-1) && ((profile == 8) || (profile == 9))) {
        fprintf(stderr, "ERROR: dolby vision bl signal compatibility id must be set when profile is 8 or 9\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 8, bl signal compatibility id must be: 1,2,4
    if ((profile == 8) && (bc != 1) && (bc != 2) && (bc != 4)) {
        fprintf(stderr, "ERROR: for dolby vision profile 8, bl signal compatibility id must be: 1, 2 or 4\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 9, bl signal compatibility id must be: 2
    if ((profile == 9) && (bc != 2) ) {
        fprintf(stderr, "ERROR: for dolby vision profile 9, bl signal compatibility id must be 2\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 5, bl signal compatibility id must be 0
    if ((profile == 5) && (bc != 0) && (bc != (AP4_UI32)-1)) {
        fprintf(stderr, "ERROR: for dolby vision profile 5, bl signal compatibility id must be 0\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   SampleFileStorage
+---------------------------------------------------------------------*/
class SampleFileStorage
{
public:
    static AP4_Result Create(const char* basename, SampleFileStorage*& sample_file_storage);
    ~SampleFileStorage() {
        m_Stream->Release();
        remove(m_Filename.GetChars());
    }
    
    AP4_ByteStream* GetStream() { return m_Stream; }
    
private:
    SampleFileStorage(const char* basename) : m_Stream(NULL) {
        AP4_Size name_length = (AP4_Size)AP4_StringLength(basename);
        char* filename = new char[name_length+2];
        AP4_CopyMemory(filename, basename, name_length);
        filename[name_length]   = '_';
        filename[name_length+1] = '\0';
        m_Filename = filename;
        delete[] filename;
    }

    AP4_ByteStream* m_Stream;
    AP4_String      m_Filename;
};

/*----------------------------------------------------------------------
|   SampleFileStorage::Create
+---------------------------------------------------------------------*/
AP4_Result
SampleFileStorage::Create(const char* basename, SampleFileStorage*& sample_file_storage)
{
    sample_file_storage = NULL;
    SampleFileStorage* object = new SampleFileStorage(basename);
    AP4_Result result = AP4_FileByteStream::Create(object->m_Filename.GetChars(),
                                                   AP4_FileByteStream::STREAM_MODE_WRITE,
                                                   object->m_Stream);
    if (AP4_FAILED(result)) {
        return result;
    }
    sample_file_storage = object;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   SortSamples
+---------------------------------------------------------------------*/
static void
SortSamples(SampleOrder* array, unsigned int n)
{
    if (n < 2) {
        return;
    }
    SampleOrder pivot = array[n / 2];
    SampleOrder* left  = array;
    SampleOrder* right = array + n - 1;
    while (left <= right) {
        if (left->m_DisplayOrder < pivot.m_DisplayOrder) {
            ++left;
            continue;
        }
        if (right->m_DisplayOrder > pivot.m_DisplayOrder) {
            --right;
            continue;
        }
        SampleOrder temp = *left;
        *left++ = *right;
        *right-- = temp;
    }
    SortSamples(array, (unsigned int)(right - array + 1));
    SortSamples(left, (unsigned int)(array + n - left));
}

/*----------------------------------------------------------------------
|   GetLanguageFromParameters
+---------------------------------------------------------------------*/
static const char*
GetLanguageFromParameters(AP4_Array<Parameter>& parameters, const char* defaut_value)
{
    // check if we have a language parameter
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "language") {
            const char* language = parameters[i].m_Value.GetChars();

            // the language must be a 3-character ISO 639-2 Alpha-3 code
            if (strlen(language) != 3) {
                fprintf(stderr, "ERROR: language codes must be 3-character ISO 639-2 Alpha-3 codes\n");
                return NULL;
            }

            return language;
        }
    }

    return defaut_value;
}

/*----------------------------------------------------------------------
|   AddAacTrack
+---------------------------------------------------------------------*/
static void
AddAacTrack(AP4_Movie&            movie,
            const char*           input_name,
            AP4_Array<Parameter>& parameters,
            SampleFileStorage&    sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // create an ADTS parser
    AP4_AdtsParser parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get AAC frames
    AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;
    bool eos = false;
    for(;;) {
        // try to get a frame
        AP4_AacFrame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (Options.verbose) {
                printf("AAC frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameLength,
                       (int)frame.m_Info.m_SamplingFrequency,
                       frame.m_Info.m_ChannelConfiguration);
            }
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_DataBuffer dsi;
                unsigned char aac_dsi[2];

                unsigned int object_type = 2; // AAC LC by default
                aac_dsi[0] = (object_type<<3) | (frame.m_Info.m_SamplingFrequencyIndex>>1);
                aac_dsi[1] = ((frame.m_Info.m_SamplingFrequencyIndex&1)<<7) | (frame.m_Info.m_ChannelConfiguration<<3);

                dsi.SetData(aac_dsi, 2);
                AP4_MpegAudioSampleDescription* sample_description = 
                    new AP4_MpegAudioSampleDescription(
                    AP4_OTI_MPEG4_AUDIO,   // object type
                    (AP4_UI32)frame.m_Info.m_SamplingFrequency,
                    16,                    // sample size
                    frame.m_Info.m_ChannelConfiguration,
                    &dsi,                  // decoder info
                    6144,                  // buffer size
                    128000,                // max bitrate
                    128000);               // average bitrate
                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                sample_rate = (AP4_UI32)frame.m_Info.m_SamplingFrequency;
            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameLength);
            sample_data.SetDataSize(frame.m_Info.m_FrameLength);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameLength);
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameLength);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(), position, frame.m_Info.m_FrameLength, 1024, sample_description_index, 0, 0, true);
            sample_count++;
        } else {
            if (result == AP4_ERROR_CORRUPTED_BITSTREAM) {
                fprintf(stderr, "WARN: The stream in corrupted, muxing stopped. The muxed MP4 only contains the first %d samples.\n", sample_count);
                break;
            } else if ((result == AP4_ERROR_NOT_ENOUGH_DATA) && (parser.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE -1))){
                fprintf(stderr, "WARN: The frame %d size is larger than the max buffer size, muxing stopped. The muxed MP4 only contains the first %d samples.\n", sample_count + 1, sample_count);
                break;
            }
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            } else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }
    
    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                     sample_table,
                                     0,                 // track id
                                     sample_rate,       // movie time scale
                                     sample_count*1024, // track duration
                                     sample_rate,       // media time scale
                                     sample_count*1024, // media duration
                                     language,          // language
                                     0, 0);             // width, height

    // cleanup
    input->Release();

    movie.AddTrack(track);
}

/*----------------------------------------------------------------------
 |   AddAc3Track
 +---------------------------------------------------------------------*/
static void
AddAc3Track(AP4_Movie&             movie,
            const char*             input_name,
            AP4_Array<Parameter>&   parameters,
            SampleFileStorage&      sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable(); // chunk_size is used to control chunk size in 'stsc' box

    // create an AC-3 Sync Frame parser
    AP4_Ac3Parser parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get AC-3 frames
    AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;

    bool eos = false;
    for(;;) {
        // try to get a frame
        AP4_Ac3Frame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (Options.verbose) {
                printf("AC-3 frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameSize,
                       frame.m_Info.m_SampleRate,
                       frame.m_Info.m_ChannelCount);
            }
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_Dac3Atom::StreamInfo *ac3_stream_info = &frame.m_Info.m_Ac3StreamInfo;

                AP4_Ac3SampleDescription* sample_description =
                new AP4_Ac3SampleDescription(
                                             frame.m_Info.m_SampleRate,       // sample rate
                                             16,                              // sample size
                                             2,                               // channel count
                                             frame.m_Info.m_FrameSize,        // Access Unit size
                                             ac3_stream_info);            // AC-3 SubStream

                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                sample_rate      = frame.m_Info.m_SampleRate;
            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameSize);
            sample_data.SetDataSize(frame.m_Info.m_FrameSize);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameSize);
            if (frame.m_LittleEndian) {
                AP4_ByteSwap16(sample_data.UseData(), (unsigned int)frame.m_Info.m_FrameSize);
            }
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameSize);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(), position, frame.m_Info.m_FrameSize, 1536, sample_description_index, 0, 0, true);
            sample_count++;
        } else {
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            } else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }

    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                     sample_table,
                                     0,                   // track id
                                     sample_rate,         // movie time scale
                                     sample_count * 1536, // track duration
                                     sample_rate,         // media time scale
                                     sample_count * 1536, // media duration
                                     language,            // language
                                     0, 0);

    // add an edit list with MediaTime==0 to ac3 track defautly.
    if (1) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        if (!movie.GetTimeScale()) {
            duration = sample_count * 1536;
        } else {
            duration = AP4_ConvertTime(1536*sample_table->GetSampleCount(), sample_rate, movie.GetTimeScale());
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, 0, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->UseTrakAtom()->AddChild(new_edts, 1);
    }

    // cleanup
    input->Release();

    movie.AddTrack(track);
}

/*----------------------------------------------------------------------
|   AddEac3Track
+---------------------------------------------------------------------*/
static void
AddEac3Track(AP4_Movie&             movie,
            const char*             input_name,
            AP4_Array<Parameter>&   parameters,
            SampleFileStorage&      sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable(); // The parameter chunk_size is used to control chunk size in 'stsc' box

    // create an E-AC-3 Sync Frame parser
    AP4_Eac3Parser parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get E-AC-3 frames
    AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;

    bool eos = false;
    for(;;) {
        // try to get a frame
        AP4_Eac3Frame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (Options.verbose) {
                printf("E-AC-3 frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameSize,
                       frame.m_Info.m_SampleRate,
                       frame.m_Info.m_ChannelCount);
            }
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_Dec3Atom::SubStream *eac3_substream = &frame.m_Info.m_Eac3SubStream;
                const unsigned int obj_num = frame.m_Info.complexity_index_type_a;

                AP4_Eac3SampleDescription* sample_description =
                    new AP4_Eac3SampleDescription(
                    frame.m_Info.m_SampleRate,      // sample rate
                    16,                             // sample size
                    2,                              // channel count
                    frame.m_Info.m_FrameSize,       // Access Unit size
                    eac3_substream,                 // E-AC-3 SubStream
                    obj_num);                       // DD+JOC object numbers
                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                sample_rate      = frame.m_Info.m_SampleRate;
            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameSize);
            sample_data.SetDataSize(frame.m_Info.m_FrameSize);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameSize);
            if (frame.m_LittleEndian) {
                AP4_ByteSwap16(sample_data.UseData(), (unsigned int)frame.m_Info.m_FrameSize);
            }
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameSize);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(), position, frame.m_Info.m_FrameSize, 1536, sample_description_index, 0, 0, true);
            sample_count++;
        } else {
            if (result == AP4_ERROR_CORRUPTED_BITSTREAM) {
                fprintf(stderr, "WARN: The stream in corrupted, so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count);
                break;
            } else if ((result == AP4_ERROR_NOT_ENOUGH_DATA) && (parser.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE -1))){
                fprintf(stderr, "WARN: The frame %d size is larger than pre-defined buffer size (8191 bytes), so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count + 1, sample_count);
                break;
            }
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            } else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }

    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                     sample_table,
                                     0,                   // track id
                                     sample_rate,         // movie time scale
                                     sample_count * 1536, // track duration
                                     sample_rate,         // media time scale
                                     sample_count * 1536, // media duration
                                     language,            // language
                                     0, 0);               // width, height

    // add an edit list with MediaTime==0 to ec3 track defautly.
    if (1) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        if (!movie.GetTimeScale()) {
            duration = sample_count * 1536;
        } else {
            duration = AP4_ConvertTime(1536*sample_table->GetSampleCount(), sample_rate, movie.GetTimeScale());
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, 0, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->UseTrakAtom()->AddChild(new_edts, 1);
    }

    // cleanup
    input->Release();

    movie.AddTrack(track);
}

/*----------------------------------------------------------------------
|   AddAc4Track
+---------------------------------------------------------------------*/
static void
AddAc4Track(AP4_Movie&            movie,
            const char*           input_name,
            AP4_Array<Parameter>& parameters,
            SampleFileStorage&    sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable(); // The parameter chunk_size is used to control chunk size in 'stsc' box

    // create an AC-4 Sync Frame parser
    AP4_Ac4Parser parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get AC-4 frames
    // AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;
    AP4_Cardinal sample_duration = 0;
    AP4_Cardinal media_time_scale = 0;
    bool eos = false;
    for(;;) {
        // try to get a frame
        AP4_Ac4Frame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (Options.verbose) {
                printf("AC-4 frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameSize,
                       (int)frame.m_Info.m_Ac4Dsi.d.v1.fs,
                       frame.m_Info.m_ChannelCount);
            }
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_Dac4Atom::Ac4Dsi *ac4Dsi = &frame.m_Info.m_Ac4Dsi;

                AP4_Ac4SampleDescription* sample_description =
                    new AP4_Ac4SampleDescription(
                    frame.m_Info.m_Ac4Dsi.d.v1.fs,  // sample rate
                    16,                             // sample size
                    frame.m_Info.m_ChannelCount,    // channel count
                    frame.m_Info.m_FrameSize,       // DIS size, can't calcuate the DSI size in advance, so assume the maximum value is frame size.
                    ac4Dsi);                        // AC-4 DSI
                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                /* sample_rate = frame.m_Info.m_Ac4Dsi.d.v1.fs */;
                sample_duration  = frame.m_Info.m_SampleDuration;
                media_time_scale = frame.m_Info.m_MediaTimeScale;

            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameSize);
            sample_data.SetDataSize(frame.m_Info.m_FrameSize);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameSize);
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameSize);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(), position, frame.m_Info.m_FrameSize, sample_duration, sample_description_index, 0, 0, (frame.m_Info.m_Iframe == 1));
            sample_count++;
            // Skip the CRC word for 0xAC41 stream
            frame.m_Source->SkipBytes(frame.m_Info.m_CRCSize);
        } else {
            if (result == AP4_ERROR_CORRUPTED_BITSTREAM) {
                fprintf(stderr, "WARN: The stream in corrupted, so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count);
                break;
            } else if ((result == AP4_ERROR_NOT_ENOUGH_DATA) && (parser.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE -1))){
                fprintf(stderr, "WARN: The frame %d size is larger than pre-defined buffer size (8191 bytes), so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count + 1, sample_count);
                break;
            }
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            } else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }

    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                     sample_table,
                                     0,                                        // track id
                                     media_time_scale,                         // movie time scale
                                     AP4_UI64(sample_count) * sample_duration, // track duration
                                     media_time_scale,                         // media time scale
                                     AP4_UI64(sample_count) * sample_duration, // media duration
                                     language,                                 // language
                                     0, 0);                                    // width, height

    // add an edit list with MediaTime==0 to ac4 track defautly.
    if (1) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        if (!movie.GetTimeScale()) {
            duration = AP4_UI64(sample_count) * sample_duration;
        } else {
            duration = AP4_ConvertTime(AP4_UI64(sample_duration)*sample_table->GetSampleCount(), media_time_scale, movie.GetTimeScale());
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, 0, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->UseTrakAtom()->AddChild(new_edts, 1);
    }

    // cleanup
    input->Release();

    movie.AddTrack(track);
}

/*----------------------------------------------------------------------
|   AddH264Track
+---------------------------------------------------------------------*/
static void
AddH264Track(AP4_Movie&            movie,
             const char*           input_name,
             AP4_Array<Parameter>& parameters,
             AP4_Array<AP4_UI32>&  brands,
             SampleFileStorage&    sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // see if the frame rate is specified
    unsigned int video_frame_rate = AP4_MUX_DEFAULT_VIDEO_FRAME_RATE*1000;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "frame_rate") {
            double frame_rate = atof(parameters[i].m_Value.GetChars());
            if (frame_rate == 0.0) {
                fprintf(stderr, "ERROR: invalid video frame rate %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
            video_frame_rate = (unsigned int)(1000.0*frame_rate);
        }
    }
    
    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // allocate an array to keep track of sample order
    AP4_Array<SampleOrder> sample_orders;
    
    // parse the input
    AP4_AvcFrameParser parser;
    for (;;) {
        bool eos;
        unsigned char input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        bool     found_access_unit = false;
        do {
            AP4_AvcFrameParser::AccessUnitInfo access_unit_info;

            found_access_unit = false;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset],
                                 bytes_in_buffer,
                                 bytes_consumed,
                                 access_unit_info,
                                 eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
            if (access_unit_info.nal_units.ItemCount()) {
                // we got one access unit
                found_access_unit = true;
                if (Options.verbose) {
                    printf("H264 Access Unit, %d NAL units, decode_order=%d, display_order=%d\n",
                           access_unit_info.nal_units.ItemCount(),
                           access_unit_info.decode_order,
                           access_unit_info.display_order);
                }
                
                // compute the total size of the sample data
                unsigned int sample_data_size = 0;
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
                }

                // store the sample data
                AP4_Position position = 0;
                sample_storage.GetStream()->Tell(position);
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_storage.GetStream()->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
                    sample_storage.GetStream()->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
                }

                // add the sample to the track
                sample_table->AddSample(*sample_storage.GetStream(), position, sample_data_size, 1000, 0, 0, 0, access_unit_info.is_idr);
            
                // remember the sample order
                sample_orders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));
                
                // free the memory buffers
                access_unit_info.Reset();
            }

            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer || found_access_unit);
        if (eos) break;
    }

    // adjust the sample CTS/DTS offsets based on the sample orders
    if (sample_orders.ItemCount() > 1) {
        unsigned int start = 0;
        for (unsigned int i=1; i<=sample_orders.ItemCount(); i++) {
            if (i == sample_orders.ItemCount() || sample_orders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&sample_orders[start], i-start);
                start = i;
            }
        }
    }
    unsigned int max_delta = 0;
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        if (sample_orders[i].m_DecodeOrder > i) {
            unsigned int delta =sample_orders[i].m_DecodeOrder-i;
            if (delta > max_delta) {
                max_delta = delta;
            }
        }
    }
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        sample_table->UseSample(sample_orders[i].m_DecodeOrder).SetCts(1000ULL*(AP4_UI64)(i+max_delta));
    }

    // check the video parameters
    AP4_AvcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps = parser.GetSequenceParameterSets()[i];
            break;
        }
    }
    if (sps == NULL) {
        fprintf(stderr, "ERROR: no sequence parameter set found in video\n");
        input->Release();
        return;
    }
    unsigned int video_width = 0;
    unsigned int video_height = 0;
    sps->GetInfo(video_width, video_height);
    if (Options.verbose) {
        printf("VIDEO: %dx%d\n", video_width, video_height);
    }

    // collect the SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps_array.Append(parser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_AVC_PPS_MAX_ID; i++) {
        if (parser.GetPictureParameterSets()[i]) {
            pps_array.Append(parser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }
    
    // setup the video the sample descripton
    AP4_AvcSampleDescription* sample_description =
        new AP4_AvcSampleDescription(AP4_SAMPLE_FORMAT_AVC1,
                                     video_width,
                                     video_height,
                                     24,
                                     "AVC Coding",
                                     sps->profile_idc,
                                     sps->level_idc,
                                     sps->constraint_set0_flag<<7 |
                                     sps->constraint_set1_flag<<6 |
                                     sps->constraint_set2_flag<<5 |
                                     sps->constraint_set3_flag<<4,
                                     4,
                                     sps->chroma_format_idc,
                                     sps->bit_depth_luma_minus8,
                                     sps->bit_depth_chroma_minus8,
                                     sps_array,
                                     pps_array);
    sample_table->AddSampleDescription(sample_description);

    AP4_UI32 movie_timescale      = 1000;
    AP4_UI32 media_timescale      = video_frame_rate;
    AP4_UI64 video_track_duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie_timescale);
    AP4_UI64 video_media_duration = 1000*sample_table->GetSampleCount();

    // create a video track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                     sample_table,
                                     0,                    // auto-select track id
                                     movie_timescale,      // movie time scale
                                     video_track_duration, // track duration
                                     media_timescale,      // media time scale
                                     video_media_duration, // media duration
                                     language,             // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );
    // Use an edit list to compensate for the inital cts offset
    if (max_delta) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        if (!movie.GetTimeScale()) {
            duration = video_media_duration;
        } else {
            duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie.GetTimeScale());
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, max_delta*1000ULL, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->UseTrakAtom()->AddChild(new_edts, 1);
    }
    // update the brands list
    brands.Append(AP4_FILE_BRAND_AVC1);

    // cleanup
    input->Release();

    movie.AddTrack(track);
}

/*----------------------------------------------------------------------
|   AddH264DoviTrack
+---------------------------------------------------------------------*/
static void
AddH264DoviTrack(AP4_Movie&            movie,
                 const char*           input_name,
                 AP4_Array<Parameter>& parameters,
                 AP4_Array<AP4_UI32>&  brands,
                 SampleFileStorage&    sample_storage)
{
    double frame_rate = 0.0;
    //based on the Dovi iso spec, set the following values to const 
    const AP4_UI32 dv_major_version = 1;
    const AP4_UI32 dv_minor_version = 0;
    const bool     dv_rpu_flag = 1;
    const bool     dv_el_flag = 0;
    const bool     dv_bl_flag = 1;

    AP4_UI32 dv_profile = 0;
    AP4_UI32 dv_bl_signal_comp_id = 0;
    AP4_UI32 dv_level = 0;

    AP4_UI32 format = 0;

    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // see if the frame rate is specified
    AP4_UI32 video_frame_rate = AP4_MUX_DEFAULT_VIDEO_FRAME_RATE*1000;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "frame_rate") {
            frame_rate = atof(parameters[i].m_Value.GetChars());
            if (frame_rate == 0.0) {
                fprintf(stderr, "ERROR: invalid video frame rate %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
            video_frame_rate = (unsigned int)(1000.0*frame_rate);
        } else if (parameters[i].m_Name == "format") {
            if (parameters[i].m_Value == "avc1") {
                format = AP4_SAMPLE_FORMAT_AVC1;
            } else if (parameters[i].m_Value == "avc3") {
                format = AP4_SAMPLE_FORMAT_AVC3;
            } else if (parameters[i].m_Value == "dva1") {
                format = AP4_SAMPLE_FORMAT_DVA1;
            } else if (parameters[i].m_Value == "dvav") {
                format = AP4_SAMPLE_FORMAT_DVAV;
            }
        } else if (parameters[i].m_Name == "dv_profile") {
            dv_profile = atoi(parameters[i].m_Value.GetChars());
        } else if (parameters[i].m_Name == "dv_bc") {
            dv_bl_signal_comp_id = atoi(parameters[i].m_Value.GetChars());
        }
    }

    //set default frame rate value
    if (frame_rate == 0.0) {
        frame_rate = 24.0;
    }

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // allocate an array to keep track of sample order
    AP4_Array<SampleOrder> sample_orders;

    // parse the input
    AP4_AvcFrameParser parser;

    if (format == AP4_SAMPLE_FORMAT_AVC3 || format == AP4_SAMPLE_FORMAT_DVAV) {
        parser.SetParameterControl(true);
    } else if (format == AP4_SAMPLE_FORMAT_AVC1 || format == AP4_SAMPLE_FORMAT_DVA1) {
        parser.SetParameterControl(false);
    }

    for (;;) {
        bool eos;
        unsigned char input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        bool     found_access_unit = false;
        do {
            AP4_AvcFrameParser::AccessUnitInfo access_unit_info;
    
            found_access_unit = false;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset],
                                 bytes_in_buffer,
                                 bytes_consumed,
                                 access_unit_info,
                                 eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
            if (access_unit_info.nal_units.ItemCount()) {
                // we got one access unit
                found_access_unit = true;
                if (Options.verbose) {
                    printf("H264 Access Unit, %d NAL units, decode_order=%d, display_order=%d\n",
                           access_unit_info.nal_units.ItemCount(),
                           access_unit_info.decode_order,
                           access_unit_info.display_order);
                }
                
                // compute the total size of the sample data
                unsigned int sample_data_size = 0;
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
                }

                // store the sample data
                AP4_Position position = 0;
                sample_storage.GetStream()->Tell(position);
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_storage.GetStream()->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
                    sample_storage.GetStream()->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
                }

                // add the sample to the track
                sample_table->AddSample(*sample_storage.GetStream(), position, sample_data_size, 1000, 0, 0, 0, access_unit_info.is_idr);

                // remember the sample order
                sample_orders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));

                // free the memory buffers
                access_unit_info.Reset();
            }

            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer || found_access_unit);
        if (eos) break;
    }

    // adjust the sample CTS/DTS offsets based on the sample orders
    if (sample_orders.ItemCount() > 1) {
        unsigned int start = 0;
        for (unsigned int i=1; i<=sample_orders.ItemCount(); i++) {
            if (i == sample_orders.ItemCount() || sample_orders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&sample_orders[start], i-start);
                start = i;
            }
        }
    }
    unsigned int max_delta = 0;
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        if (sample_orders[i].m_DecodeOrder > i) {
            unsigned int delta =sample_orders[i].m_DecodeOrder-i;
            if (delta > max_delta) {
                max_delta = delta;
            }
        }
    }
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        sample_table->UseSample(sample_orders[i].m_DecodeOrder).SetCts(1000ULL*(AP4_UI64)(i+max_delta));
    }

    // check the video parameters
    AP4_AvcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps = parser.GetSequenceParameterSets()[i];
            break;
        }
    }
    if (sps == NULL) {
        fprintf(stderr, "ERROR: no sequence parameter set found in video\n");
        input->Release();
        return;
    }
    unsigned int video_width = 0;
    unsigned int video_height = 0;
    sps->GetInfo(video_width, video_height);
    if (Options.verbose) {
        printf("VIDEO: %dx%d\n", video_width, video_height);
    }

    // collect the SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps_array.Append(parser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_AVC_PPS_MAX_ID; i++) {
        if (parser.GetPictureParameterSets()[i]) {
            pps_array.Append(parser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }

    // compute the dolby vision level
    ComputeDoviLevel(video_width, video_height, frame_rate, dv_level);

    // setup the video the sample descripton
    AP4_AvcDoviSampleDescription* sample_description =
        new AP4_AvcDoviSampleDescription(format,
                                         video_width,
                                         video_height,
                                         24,
                                         "DOVI Coding",
                                         sps->profile_idc,
                                         sps->level_idc,
                                         sps->constraint_set0_flag<<7 |
                                         sps->constraint_set1_flag<<6 |
                                         sps->constraint_set2_flag<<5 |
                                         sps->constraint_set3_flag<<4,
                                         4,
                                         sps_array,
                                         pps_array,
                                         sps->chroma_format_idc,
                                         sps->bit_depth_luma_minus8,
                                         sps->bit_depth_chroma_minus8,
                                         dv_major_version,
                                         dv_minor_version,
                                         dv_profile,
                                         dv_level,
                                         dv_rpu_flag,
                                         dv_el_flag,
                                         dv_bl_flag,
                                         dv_bl_signal_comp_id);
    sample_table->AddSampleDescription(sample_description);

    AP4_UI32 movie_timescale      = 1000;
    AP4_UI32 media_timescale      = video_frame_rate;
    AP4_UI64 video_track_duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie_timescale);
    AP4_UI64 video_media_duration = 1000*sample_table->GetSampleCount();

    // create a video track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                     sample_table,
                                     0,                    // auto-select track id
                                     movie_timescale,      // movie time scale
                                     video_track_duration, // track duration
                                     media_timescale,      // media time scale
                                     video_media_duration, // media duration
                                     language,             // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );
    // Using edit list to compensate the inital cts offset
    if (max_delta) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        if (!movie.GetTimeScale()) {
            duration = video_media_duration;
        } else {
            duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie.GetTimeScale());
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, max_delta*1000ULL, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->UseTrakAtom()->AddChild(new_edts, 1);
    }
    // update the brands list
    brands.Append(format);

    // cleanup
    input->Release();

    movie.AddTrack(track);
}

/*----------------------------------------------------------------------
|   AddH265Track
+---------------------------------------------------------------------*/
static void
AddH265Track(AP4_Movie&            movie,
             const char*           input_name,
             AP4_Array<Parameter>& parameters,
             AP4_Array<AP4_UI32>&  brands,
             SampleFileStorage&    sample_storage)
{
    unsigned int video_width = 0;
    unsigned int video_height = 0;
    AP4_UI32     format = AP4_SAMPLE_FORMAT_HVC1;
    
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // see if the frame rate is specified
    unsigned int video_frame_rate = AP4_MUX_DEFAULT_VIDEO_FRAME_RATE*1000;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "frame_rate") {
            double frame_rate = atof(parameters[i].m_Value.GetChars());
            if (frame_rate == 0.0) {
                fprintf(stderr, "ERROR: invalid video frame rate %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
            video_frame_rate = (unsigned int)(1000.0*frame_rate);
        } else if (parameters[i].m_Name == "format") {
            if (parameters[i].m_Value == "hev1") {
                format = AP4_SAMPLE_FORMAT_HEV1;
            } else if (parameters[i].m_Value == "hvc1") {
                format = AP4_SAMPLE_FORMAT_HVC1;
            }
        }
    }
    
    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // allocate an array to keep track of sample order
    AP4_Array<SampleOrder> sample_orders;
    
    // parse the input
    AP4_HevcFrameParser parser;
    for (;;) {
        bool eos;
        unsigned char input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        bool     found_access_unit = false;
        do {
            AP4_HevcFrameParser::AccessUnitInfo access_unit_info;
            
            found_access_unit = false;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset],
                                 bytes_in_buffer,
                                 bytes_consumed,
                                 access_unit_info,
                                 eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
            if (access_unit_info.nal_units.ItemCount()) {
                // we got one access unit
                found_access_unit = true;
                if (Options.verbose) {
                    printf("H265 Access Unit, %d NAL units, decode_order=%d, display_order=%d\n",
                           access_unit_info.nal_units.ItemCount(),
                           access_unit_info.decode_order,
                           access_unit_info.display_order);
                }
                
                // compute the total size of the sample data
                unsigned int sample_data_size = 0;
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
                }
                
                // store the sample data
                AP4_Position position = 0;
                sample_storage.GetStream()->Tell(position);
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_storage.GetStream()->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
                    sample_storage.GetStream()->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
                }
                
                // add the sample to the track
                sample_table->AddSample(*sample_storage.GetStream(), position, sample_data_size, 1000, 0, 0, 0, access_unit_info.is_random_access);
            
                // remember the sample order
                sample_orders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));
                
                // free the memory buffers
                access_unit_info.Reset();
            }
        
            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer || found_access_unit);
        if (eos) break;
    }
    
    // adjust the sample CTS/DTS offsets based on the sample orders
    if (sample_orders.ItemCount() > 1) {
        unsigned int start = 0;
        for (unsigned int i=1; i<=sample_orders.ItemCount(); i++) {
            if (i == sample_orders.ItemCount() || sample_orders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&sample_orders[start], i-start);
                start = i;
            }
        }
    }
    unsigned int max_delta = 0;
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        if (sample_orders[i].m_DecodeOrder > i) {
            unsigned int delta =sample_orders[i].m_DecodeOrder-i;
            if (delta > max_delta) {
                max_delta = delta;
            }
        }
    }
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        sample_table->UseSample(sample_orders[i].m_DecodeOrder).SetCts(1000ULL*(AP4_UI64)(i+max_delta));
    }
    
    // check that we have at least one SPS
    AP4_HevcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        sps = parser.GetSequenceParameterSets()[i];
        if (sps) break;
    }
    if (sps == NULL) {
        fprintf(stderr, "ERROR: no sequence parameter set found in video\n");
        input->Release();
        return;
    }
    
    // collect parameters from the first SPS entry
    // TODO: synthesize from multiple SPS entries if we have more than one
    AP4_UI08 general_profile_space =               sps->profile_tier_level.general_profile_space;
    AP4_UI08 general_tier_flag =                   sps->profile_tier_level.general_tier_flag;
    AP4_UI08 general_profile =                     sps->profile_tier_level.general_profile_idc;
    AP4_UI32 general_profile_compatibility_flags = sps->profile_tier_level.general_profile_compatibility_flags;
    AP4_UI64 general_constraint_indicator_flags =  sps->profile_tier_level.general_constraint_indicator_flags;
    AP4_UI08 general_level =                       sps->profile_tier_level.general_level_idc;
    AP4_UI32 min_spatial_segmentation =            0; // TBD (should read from VUI if present)
    AP4_UI08 parallelism_type =                    0; // unknown
    AP4_UI08 chroma_format =                       sps->chroma_format_idc;
    AP4_UI08 luma_bit_depth =                      8; // hardcoded temporarily, should be read from the bitstream
    AP4_UI08 chroma_bit_depth =                    8; // hardcoded temporarily, should be read from the bitstream
    AP4_UI16 average_frame_rate =                  0; // unknown
    AP4_UI08 constant_frame_rate =                 0; // unknown
    AP4_UI08 num_temporal_layers =                 0; // unknown
    AP4_UI08 temporal_id_nested =                  0; // unknown
    AP4_UI08 nalu_length_size =                    4;

    sps->GetInfo(video_width, video_height);
    if (Options.verbose) {
        printf("VIDEO: %dx%d\n", video_width, video_height);
    }
    
    // collect the VPS, SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> vps_array;
    for (unsigned int i=0; i<=AP4_HEVC_VPS_MAX_ID; i++) {
        if (parser.GetVideoParameterSets()[i]) {
            vps_array.Append(parser.GetVideoParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps_array.Append(parser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_HEVC_PPS_MAX_ID; i++) {
        if (parser.GetPictureParameterSets()[i]) {
            pps_array.Append(parser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }
    
    // setup the video the sample descripton
    AP4_UI08 parameters_completeness = (format == AP4_SAMPLE_FORMAT_HVC1 ? 1 : 0);
    AP4_HevcSampleDescription* sample_description =
        new AP4_HevcSampleDescription(format,
                                      video_width,
                                      video_height,
                                      24,
                                      "HEVC Coding",
                                      general_profile_space,
                                      general_tier_flag,
                                      general_profile,
                                      general_profile_compatibility_flags,
                                      general_constraint_indicator_flags,
                                      general_level,
                                      min_spatial_segmentation,
                                      parallelism_type,
                                      chroma_format,
                                      luma_bit_depth,
                                      chroma_bit_depth,
                                      average_frame_rate,
                                      constant_frame_rate,
                                      num_temporal_layers,
                                      temporal_id_nested,
                                      nalu_length_size,
                                      vps_array,
                                      parameters_completeness,
                                      sps_array,
                                      parameters_completeness,
                                      pps_array,
                                      parameters_completeness);
    
    sample_table->AddSampleDescription(sample_description);
    
    AP4_UI32 movie_timescale      = 1000;
    AP4_UI32 media_timescale      = video_frame_rate;
    AP4_UI64 video_track_duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie_timescale);
    AP4_UI64 video_media_duration = 1000*sample_table->GetSampleCount();

    // create a video track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                     sample_table,
                                     0,                    // auto-select track id
                                     movie_timescale,      // movie time scale
                                     video_track_duration, // track duration
                                     media_timescale,      // media time scale
                                     video_media_duration, // media duration
                                     language,             // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );

    // Use an edit list to compensate for the inital cts offset
    if (max_delta) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        if (!movie.GetTimeScale()) {
            duration = video_media_duration;
        } else {
            duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie.GetTimeScale());
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, max_delta*1000ULL, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->UseTrakAtom()->AddChild(new_edts, 1);
    }
    // update the brands list
    brands.Append(AP4_FILE_BRAND_HVC1);

    // cleanup
    input->Release();

    movie.AddTrack(track);
}

/*----------------------------------------------------------------------
|   AddH265DoviTrack
+---------------------------------------------------------------------*/
static void
AddH265DoviTrack(AP4_Movie&           movie,
                const char*           input_name,
                AP4_Array<Parameter>& parameters,
                AP4_Array<AP4_UI32>&  brands,
                SampleFileStorage&    sample_storage)
{
    AP4_UI32 video_width = 0;
    AP4_UI32 video_height = 0;
    double   frame_rate = 0.0;

    //based on the Dovi iso spec, set the following values to const 
    const AP4_UI32 dv_major_version = 1;
    const AP4_UI32 dv_minor_version = 0;
    const bool     dv_rpu_flag = 1;
    const bool     dv_el_flag = 0;
    const bool     dv_bl_flag = 1;

    AP4_UI32 dv_profile = 0;
    AP4_UI32 dv_bl_signal_comp_id = 0;
    AP4_UI32 dv_level = 0;

    AP4_UI32 format = 0;
    
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // see if the frame rate/format/dv_profile/dv_bc is specified
    unsigned int video_frame_rate = AP4_MUX_DEFAULT_VIDEO_FRAME_RATE*1000;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "frame_rate") {
            frame_rate = atof(parameters[i].m_Value.GetChars());
            if (frame_rate == 0.0) {
                fprintf(stderr, "ERROR: invalid video frame rate %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
            video_frame_rate = (unsigned int)(1000.0*frame_rate);
        } else if (parameters[i].m_Name == "format") {
            if (parameters[i].m_Value == "hvc1") {
                format = AP4_SAMPLE_FORMAT_HVC1;
            } else if (parameters[i].m_Value == "hev1") {
                format = AP4_SAMPLE_FORMAT_HEV1;
            } else if (parameters[i].m_Value == "dvh1") {
                format = AP4_SAMPLE_FORMAT_DVH1;
            } else if (parameters[i].m_Value == "dvhe") {
                format = AP4_SAMPLE_FORMAT_DVHE;
            }
        } else if (parameters[i].m_Name == "dv_profile") {
            dv_profile = atoi(parameters[i].m_Value.GetChars());
        } else if (parameters[i].m_Name == "dv_bc") {
            dv_bl_signal_comp_id = atoi(parameters[i].m_Value.GetChars());
        }
    }
    
    //if not set format, set the default one based on profile 
    if (!format) {
        if (dv_profile == 5) {
            format = AP4_SAMPLE_FORMAT_DVH1;
        } else if (dv_profile == 8) {
            if (dv_bl_signal_comp_id) {
                format = AP4_SAMPLE_FORMAT_HVC1;
            } else {
                format = AP4_SAMPLE_FORMAT_DVH1;
            }
        }
    }

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // allocate an array to keep track of sample order
    AP4_Array<SampleOrder> sample_orders;
    
    // parse the input
    AP4_HevcFrameParser parser;
    if (format == AP4_SAMPLE_FORMAT_HEV1 || format == AP4_SAMPLE_FORMAT_DVHE) {
        parser.SetParameterControl(true);
    } else if (format == AP4_SAMPLE_FORMAT_HVC1 || format == AP4_SAMPLE_FORMAT_DVH1) {
        parser.SetParameterControl(false);
    }
    for (;;) {
        bool eos;
        unsigned char input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        bool     found_access_unit = false;
        do {
            AP4_HevcFrameParser::AccessUnitInfo access_unit_info;
            
            found_access_unit = false;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset],
                                 bytes_in_buffer,
                                 bytes_consumed,
                                 access_unit_info,
                                 eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
            if (access_unit_info.nal_units.ItemCount()) {
                // we got one access unit
                found_access_unit = true;
                if (Options.verbose) {
                    printf("H265 Access Unit, %d NAL units, decode_order=%d, display_order=%d\n",
                           access_unit_info.nal_units.ItemCount(),
                           access_unit_info.decode_order,
                           access_unit_info.display_order);
                }
                
                // compute the total size of the sample data
                unsigned int sample_data_size = 0;
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
                }
                
                // store the sample data
                AP4_Position position = 0;
                sample_storage.GetStream()->Tell(position);
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_storage.GetStream()->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
                    sample_storage.GetStream()->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
                }
                
                // add the sample to the track
                sample_table->AddSample(*sample_storage.GetStream(), position, sample_data_size, 1000, 0, 0, 0, access_unit_info.is_random_access);
            
                // remember the sample order
                sample_orders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));
                
                // free the memory buffers
                access_unit_info.Reset();
            }
        
            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer || found_access_unit);
        if (eos) break;
    }
    
    // adjust the sample CTS/DTS offsets based on the sample orders
    if (sample_orders.ItemCount() > 1) {
        unsigned int start = 0;
        for (unsigned int i=1; i<=sample_orders.ItemCount(); i++) {
            if (i == sample_orders.ItemCount() || sample_orders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&sample_orders[start], i-start);
                start = i;
            }
        }
    }
    unsigned int max_delta = 0;
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        if (sample_orders[i].m_DecodeOrder > i) {
            unsigned int delta =sample_orders[i].m_DecodeOrder-i;
            if (delta > max_delta) {
                max_delta = delta;
            }
        }
    }
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        sample_table->UseSample(sample_orders[i].m_DecodeOrder).SetCts(1000ULL*(AP4_UI64)(i+max_delta));
    }
    
    // check that we have at least one SPS
    AP4_HevcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        sps = parser.GetSequenceParameterSets()[i];
        if (sps) break;
    }
    if (sps == NULL) {
        fprintf(stderr, "ERROR: no sequence parameter set found in video\n");
        input->Release();
        return;
    }
    
    // collect parameters from the first SPS entry
    // TODO: synthesize from multiple SPS entries if we have more than one
    AP4_UI08 general_profile_space =               sps->profile_tier_level.general_profile_space;
    AP4_UI08 general_tier_flag =                   sps->profile_tier_level.general_tier_flag;
    AP4_UI08 general_profile =                     sps->profile_tier_level.general_profile_idc;
    AP4_UI32 general_profile_compatibility_flags = sps->profile_tier_level.general_profile_compatibility_flags;
    AP4_UI64 general_constraint_indicator_flags =  sps->profile_tier_level.general_constraint_indicator_flags;
    AP4_UI08 general_level =                       sps->profile_tier_level.general_level_idc;
    AP4_UI32 min_spatial_segmentation =            0; // TBD (should read from VUI if present)
    AP4_UI08 parallelism_type =                    0; // unknown
    AP4_UI08 chroma_format =                       sps->chroma_format_idc;
    AP4_UI08 luma_bit_depth =                      8; // hardcoded temporarily, should be read from the bitstream
    AP4_UI08 chroma_bit_depth =                    8; // hardcoded temporarily, should be read from the bitstream
    AP4_UI16 average_frame_rate =                  0; // unknown
    AP4_UI08 constant_frame_rate =                 0; // unknown
    AP4_UI08 num_temporal_layers =                 0; // unknown
    AP4_UI08 temporal_id_nested =                  0; // unknown
    AP4_UI08 nalu_length_size =                    4;
    AP4_UI08 transfer_characteristics =            sps->vui_parameters.transfer_characteristics;

    sps->GetInfo(video_width, video_height);
    if (Options.verbose) {
        printf("VIDEO: %dx%d\n", video_width, video_height);
    }
    
    // collect the VPS, SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> vps_array;
    for (unsigned int i=0; i<=AP4_HEVC_VPS_MAX_ID; i++) {
        if (parser.GetVideoParameterSets()[i]) {
            vps_array.Append(parser.GetVideoParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps_array.Append(parser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_HEVC_PPS_MAX_ID; i++) {
        if (parser.GetPictureParameterSets()[i]) {
            pps_array.Append(parser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }

    // get the frame rate from the VPS
    AP4_HevcVideoParameterSet* vps = parser.GetVideoParameterSets()[0];
    unsigned int vps_time_scale = 0;
    unsigned int vps_num_units = 0;
    vps->GetInfo(vps_time_scale, vps_num_units);
    if (vps_num_units) {
        frame_rate = vps_time_scale/vps_num_units;
    }

    // set dolby vision level
    ComputeDoviLevel(video_width, video_height, frame_rate, dv_level);

    // setup the video the sample descripton
    AP4_UI08 parameters_completeness = ((format == AP4_SAMPLE_FORMAT_HVC1 || format == AP4_SAMPLE_FORMAT_DVH1) ? 1 : 0);
    AP4_HevcDoviSampleDescription* sample_description =
        new AP4_HevcDoviSampleDescription(format,
                                          video_width,
                                          video_height,
                                          24,
                                          "DOVI Coding",
                                          general_profile_space,
                                          general_tier_flag,
                                          general_profile,
                                          general_profile_compatibility_flags,
                                          general_constraint_indicator_flags,
                                          general_level,
                                          min_spatial_segmentation,
                                          parallelism_type,
                                          chroma_format,
                                          luma_bit_depth,
                                          chroma_bit_depth,
                                          average_frame_rate,
                                          constant_frame_rate,
                                          num_temporal_layers,
                                          temporal_id_nested,
                                          nalu_length_size,
                                          vps_array,
                                          parameters_completeness,
                                          sps_array,
                                          parameters_completeness,
                                          pps_array,
                                          parameters_completeness,
                                          dv_major_version,
                                          dv_minor_version,
                                          dv_profile,
                                          dv_level,
                                          dv_rpu_flag,
                                          dv_el_flag,
                                          dv_bl_flag,
                                          dv_bl_signal_comp_id);
    
    sample_table->AddSampleDescription(sample_description);

    AP4_UI32 movie_timescale      = 1000;
    AP4_UI32 media_timescale      = video_frame_rate;
    AP4_UI64 video_track_duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie_timescale);
    AP4_UI64 video_media_duration = 1000*sample_table->GetSampleCount();

    // create a video track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                     sample_table,
                                     0,                    // auto-select track id
                                     movie_timescale,      // movie time scale
                                     video_track_duration, // track duration
                                     media_timescale,      // media time scale
                                     video_media_duration, // media duration
                                     language,             // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );
    // Using edit list to compensate the inital cts offset
    if (max_delta) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        if (!movie.GetTimeScale()) {
            duration = video_media_duration;
        } else {
            duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie.GetTimeScale());
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, max_delta*1000ULL, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->UseTrakAtom()->AddChild(new_edts, 1);
    }
    // update the brands list
    brands.Append(format);
    if (dv_profile == 8 && dv_bl_signal_comp_id == 4) {
        if (transfer_characteristics == 18) {
            brands.Append(AP4_FILE_BRAND_DB4H);
        } else if (transfer_characteristics == 14) {
            brands.Append(AP4_FILE_BRAND_DB4G);
        }
    }

    // cleanup
    input->Release();

    movie.AddTrack(track);
}

/*----------------------------------------------------------------------
|   AddMp4Tracks
+---------------------------------------------------------------------*/
static void
AddMp4Tracks(AP4_Movie&            movie,
             const char*           input_name,
             AP4_Array<Parameter>& parameters,
             AP4_Array<AP4_UI32>&  /*brands*/)
{
    // open the input
    AP4_ByteStream* input_stream = NULL;
    AP4_Result result = AP4_FileByteStream::Create(input_name,
                                                   AP4_FileByteStream::STREAM_MODE_READ, 
                                                   input_stream);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file %s (%d)\n", input_name, result);
        return;
    }
    
    AP4_File file(*input_stream, true);
    input_stream->Release();
    AP4_Movie* input_movie = file.GetMovie();
    if (input_movie == NULL) {
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, NULL);

    // check the parameters to decide which track(s) to import
    unsigned int track_id = 0;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "track") {
            if (parameters[i].m_Value == "audio") {
                AP4_Track* track = input_movie->GetTrack(AP4_Track::TYPE_AUDIO);
                if (track == NULL) {
                    fprintf(stderr, "ERROR: no audio track found in %s\n", input_name);
                    return;
                } else {
                    track_id = track->GetId();
                }
            } else if (parameters[i].m_Value == "video") {
                AP4_Track* track = input_movie->GetTrack(AP4_Track::TYPE_VIDEO);
                if (track == NULL) {
                    fprintf(stderr, "ERROR: no video track found in %s\n", input_name);
                    return;
                } else {
                    track_id = track->GetId();
                }
            } else {
                track_id = (unsigned int)strtoul(parameters[i].m_Value.GetChars(), NULL, 10);
                if (track_id == 0) {
                    fprintf(stderr, "ERROR: invalid track ID specified");
                    return;
                }
            }
        }
    }
    
    if (Options.verbose) {
        if (track_id == 0) {
            printf("MP4 Import: importing all tracks from %s\n", input_name);
        } else {
            printf("MP4 Import: importing track %d from %s\n", track_id, input_name);
        }
    }
    
    AP4_List<AP4_Track>::Item* track_item = input_movie->GetTracks().FirstItem();
    while (track_item) {
        AP4_Track* track = track_item->GetData();
        if (track_id == 0 || track->GetId() == track_id) {
            track = track->Clone();

            // reset the track ID so that it can be re-assigned
            track->SetId(0);

            // override the language if specified in the parameters
            if (language) {
                track->SetTrackLanguage(language);
            }

            movie.AddTrack(track);
        }
        track_item = track_item->GetNext();
    }
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
    Options.verbose = false;
    
    const char* output_filename = NULL;
    AP4_Array<char*> input_names;
    
    while (char* arg = *++argv) {
        if (!strcmp(arg, "--verbose")) {
            Options.verbose = true;
        } else if (!strcmp(arg, "--track")) {
            input_names.Append(*++argv);
        } else if (output_filename == NULL) {
            output_filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
            return 1;
        }
    }

    if (input_names.ItemCount() == 0) {
        fprintf(stderr, "ERROR: no input\n");
        return 1;
    }
    if (output_filename == NULL) {
        fprintf(stderr, "ERROR: output filename missing\n");
        return 1;
    }

    // create the movie object to hold the tracks
    AP4_UI64 creation_time = 0;
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        // adjust the time based on the MPEG time origin
        creation_time = (AP4_UI64)now + 0x7C25B080;
    }
    AP4_Movie* movie = new AP4_Movie(0, 0, creation_time, creation_time);

    // setup the brands
    AP4_Array<AP4_UI32> brands;
    brands.Append(AP4_FILE_BRAND_ISOM);
    brands.Append(AP4_FILE_BRAND_MP42);

    // create a temp file to store the sample data
    SampleFileStorage* sample_storage = NULL;
    AP4_Result result = SampleFileStorage::Create(output_filename, sample_storage);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to create temporary sample data storage (%d)\n", result);
        return 1;
    }
    
    // add all the tracks
    bool hasDovi = false;
    AP4_UI08 dolby_vision_ccid = 0;
    for (unsigned int i=0; i<input_names.ItemCount(); i++) {
        char*       input_name = input_names[i];
        const char* input_type = NULL;
        char*       input_params = NULL;

        char* separator = strchr(input_name, ':');
        if (separator) {
            input_type = input_name;
            input_name = separator+1;
            *separator = '\0';
        }
        separator = strchr(input_name, '#');
        if (separator) {
            input_params = separator+1;
            *separator = '\0';
        }
        
        if (input_type == NULL) {
            // no type, try to guess
            separator = strrchr(input_name, '.');
            if (separator) {
                input_type = separator+1;
                for (unsigned int j=1; separator[j]; j++) {
                    separator[j] = (char)tolower(separator[j]);
                }
                if (!strcmp("264", input_type)) {
                    input_type = "h264";
                } else if (!strcmp("avc", input_type)) {
                    input_type = "h264";
                } else if (!strcmp("265", input_type)) {
                    input_type = "h265";
                } else if (!strcmp("hevc", input_type)) {
                    input_type = "h265";
                } else if (!strcmp("adts", input_type)) {
                    input_type = "aac";
                } else if (!strcmp("m4a", input_type) ||
                           !strcmp("m4v", input_type) ||
                           !strcmp("mov", input_type)) {
                    input_type = "aac";
                }
            } else {
                fprintf(stderr, "ERROR: unable to determine type for input '%s'\n", input_name);
                delete sample_storage;
                return 1;
            }
        }
        
        // parse parameters
        AP4_Array<Parameter> parameters;
        if (input_params) {
            ParseParameters(input_params, parameters);
        }

        bool isDovi = false;
        for (unsigned int i=0; i<parameters.ItemCount(); i++) {
            if (parameters[i].m_Name == "dv_profile") {
                isDovi = true;
            }
            if (parameters[i].m_Name == "dv_bc") {
                if (parameters[i].m_Value == "1") {
                    dolby_vision_ccid |= 0x1;
                } else if (parameters[i].m_Value == "2") {
                    dolby_vision_ccid |= 0x2;
                } else if (parameters[i].m_Value == "4") {
                    dolby_vision_ccid |= 0x4;
                }
            }
        }

        if (!strcmp(input_type, "h264")) {
            if (isDovi) {
                if (CheckDoviInputParameters(parameters) != AP4_SUCCESS) {
                    fprintf(stderr, "ERROR: dolby vision input parameter error\n");
                    delete sample_storage;
                    return 1;
                } else {
                    AddH264DoviTrack(*movie, input_name, parameters, brands, *sample_storage);
                    hasDovi = true;
                }
            } else {
                AddH264Track(*movie, input_name, parameters, brands, *sample_storage);
            }
        } else if (!strcmp(input_type, "h265")) {
            if (isDovi) {
                if (CheckDoviInputParameters(parameters) != AP4_SUCCESS) {
                    fprintf(stderr, "ERROR: dolby vision input parameter error\n");
                    delete sample_storage;
                    return 1;
                } else {
                    AddH265DoviTrack(*movie, input_name, parameters, brands, *sample_storage);
                    hasDovi = true;
                }
            } else {
                AddH265Track(*movie, input_name, parameters, brands, *sample_storage);
            }
        } else if (!strcmp(input_type, "aac")) {
            AddAacTrack(*movie, input_name, parameters, *sample_storage);
        } else if (!strcmp(input_type, "ac3")) {
            AddAc3Track(*movie, input_name, parameters, *sample_storage);
        } else if (!strcmp(input_type, "ec3")) {
            AddEac3Track(*movie, input_name, parameters, *sample_storage);
        } else if (!strcmp(input_type, "ac4")) {
            AddAc4Track(*movie, input_name, parameters, *sample_storage);
        } else if (!strcmp(input_type, "mp4")) {
            AddMp4Tracks(*movie, input_name, parameters, brands);
        } else {
            fprintf(stderr, "ERROR: unsupported input type '%s'\n", input_type);
            delete sample_storage;
            return 1;
        }
    }

    // for Dolby Vision, add the 'dby1' brand
    if (hasDovi) {
        brands.Append(AP4_FILE_BRAND_DBY1);
    }
    // brand of Dolby Vision with CCID == 4 needs transfer characteristic to be decided later
    if (dolby_vision_ccid & 0x1) {
        brands.Append(AP4_FILE_BRAND_DB1P);
    }
    if (dolby_vision_ccid & 0x2) {
        brands.Append(AP4_FILE_BRAND_DB2G);
    }

    movie->GetMvhdAtom()->SetNextTrackId(movie->GetTracks().ItemCount() + 1);

    // open the output
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        AP4_Debug("ERROR: cannot open output '%s' (%d)\n", output_filename, result);
        delete sample_storage;
        return 1;
    }
    
    {
        // create a multimedia file
        AP4_File file(movie);

        // set the file type
        file.SetFileType(AP4_FILE_BRAND_MP42, 1, &brands[0], brands.ItemCount());

        // write the file to the output
        AP4_FileWriter::Write(file, *output);
    }
    
    // cleanup
    delete sample_storage;
    output->Release();
    
    return 0;
}
