/*****************************************************************
|
|    AP4 - AAC to MP4 Converter
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
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
#include "Ap4AdtsParser.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "AAC to MP4 Converter - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2008 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: aac2mp4 <input> <output>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   MakeDsi
+---------------------------------------------------------------------*/
static void
MakeDsi(unsigned int sampling_frequency_index, unsigned int channel_configuration, unsigned char* dsi)
{
    unsigned int object_type = 2; // AAC LC by default
    dsi[0] = (object_type<<3) | (sampling_frequency_index>>1);
    dsi[1] = ((sampling_frequency_index&1)<<7) | (channel_configuration<<3);
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    AP4_Result result;

    if (argc < 3) {
        PrintUsageAndExit();
    }
    
    // open the input
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(argv[1], AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        AP4_Debug("ERROR: cannot open input (%s) %d\n", argv[1], result);
        return 1;
    }

    // open the output
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(argv[2], AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        AP4_Debug("ERROR: cannot open output (%s) %d\n", argv[2], result);
        return 1;
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
            AP4_Debug("AAC frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameLength,
                       frame.m_Info.m_SamplingFrequency,
                       frame.m_Info.m_ChannelConfiguration);
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_DataBuffer dsi;
                unsigned char aac_dsi[2];
                MakeDsi(frame.m_Info.m_SamplingFrequencyIndex, frame.m_Info.m_ChannelConfiguration, aac_dsi);
                dsi.SetData(aac_dsi, 2);
                AP4_MpegAudioSampleDescription* sample_description = 
                    new AP4_MpegAudioSampleDescription(
                    AP4_OTI_MPEG4_AUDIO,   // object type
                    frame.m_Info.m_SamplingFrequency,
                    16,                    // sample size
                    frame.m_Info.m_ChannelConfiguration,
                    &dsi,                  // decoder info
                    6144,                  // buffer size
                    128000,                // max bitrate
                    128000);               // average bitrate
                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                sample_rate = frame.m_Info.m_SamplingFrequency;
            }

            AP4_MemoryByteStream* sample_data = new AP4_MemoryByteStream(frame.m_Info.m_FrameLength);
            frame.m_Source->ReadBytes(sample_data->UseData(), frame.m_Info.m_FrameLength);
            sample_table->AddSample(*sample_data, 0, frame.m_Info.m_FrameLength, 1024, sample_description_index, 0, 0, true);
            sample_data->Release();
            sample_count++;
        } else if (!eos) {
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
                        return 1;
                    }
                } else {
                    if (result == AP4_ERROR_EOS) {
                        eos = true;
                        parser.Feed(NULL, 0, true);
                    }
                }
            }
        } else {
            break;
        }
    }

    // create a movie
    AP4_Movie* movie = new AP4_Movie();

    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO, 
                                     sample_table, 
                                     0,     // track id
                                     sample_rate,       // movie time scale
                                     sample_count*1024, // track duration              
                                     sample_rate,       // media time scale
                                     sample_count*1024, // media duration
                                     "eng", // language
                                     0, 0); // width, height

    // add the track to the movie
    movie->AddTrack(track);

    // create a multimedia file
    AP4_File* file = new AP4_File(movie);

    // set the file type
    AP4_UI32 compatible_brands[2] = {
        AP4_FILE_BRAND_ISOM,
        AP4_FILE_BRAND_MP42
    };
    file->SetFileType(AP4_FILE_BRAND_M4A_, 0, compatible_brands, 2);

    // write the file to the output
    AP4_FileWriter::Write(*file, *output);

    delete file;
    input->Release();
    output->Release();
    
    return 0;
}
