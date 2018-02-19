/*****************************************************************
|
|    AP4 - Elementary Stream Muliplexer
|
|    Copyright 2002-2016 Axiomatic Systems, LLC
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

#include "Ap4.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 Elementary Stream Multiplexer - Version 1.1\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-20016 Axiomatic Systems, LLC"

const unsigned int AP4_MUX_DEFAULT_VIDEO_FRAME_RATE = 24;

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
            "  h265: H265/HEVC NAL units\n"
            "    optional params:\n"
            "      frame_rate: floating point number in frames per second (default=24.0)\n"
            "      format: hev1 or hvc1 (default) for HEVC tracks\n"
            "  aac:  AAC in ADTS format\n"
            "  mp4:  MP4 track(s) from an MP4 file\n"
            "    optional params:\n"
            "      track: audio, video, or integer track ID (default=all tracks)\n"
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
|   AddAacTrack
+---------------------------------------------------------------------*/
static void
AddAacTrack(AP4_Movie&            movie,
            const char*           input_name,
            AP4_Array<Parameter>& /*parameters*/,
            SampleFileStorage&    sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

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
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[4096];
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
                                     "und",             // language
                                     0, 0);             // width, height
    
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
        unsigned char input_buffer[4096];
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
                                     video_frame_rate,     // media time scale
                                     video_media_duration, // media duration
                                     "und",                // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );

    // update the brands list
    brands.Append(AP4_FILE_BRAND_AVC1);

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
        unsigned char input_buffer[4096];
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
    AP4_UI08 luma_bit_depth =                      8; // FIXME: hardcoded temporarily, should be read from the bitstream
    AP4_UI08 chroma_bit_depth =                    8; // FIXME: hardcoded temporarily, should be read from the bitstream
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
                                     video_frame_rate,     // media time scale
                                     video_media_duration, // media duration
                                     "und",                // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );

    // update the brands list
    brands.Append(AP4_FILE_BRAND_HVC1);

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
    AP4_Movie* movie = new AP4_Movie();

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
        
        if (!strcmp(input_type, "h264")) {
            AddH264Track(*movie, input_name, parameters, brands, *sample_storage);
        } else if (!strcmp(input_type, "h265")) {
            AddH265Track(*movie, input_name, parameters, brands, *sample_storage);
        } else if (!strcmp(input_type, "aac")) {
            AddAacTrack(*movie, input_name, parameters, *sample_storage);
        } else if (!strcmp(input_type, "mp4")) {
            AddMp4Tracks(*movie, input_name, parameters, brands);
        } else {
            fprintf(stderr, "ERROR: unsupported input type '%s'\n", input_type);
            delete sample_storage;
            return 1;
        }
    }

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
