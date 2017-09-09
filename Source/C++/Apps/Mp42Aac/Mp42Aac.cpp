/*****************************************************************
|
|    AP4 - MP4 to AAC File Converter
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

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 To AAC File Converter - Version 1.0\n"\
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
            "\n\nusage: mp42aac [options] <input> <output>\n"
            "  Options:\n"
            "  --key <hex>: 128-bit decryption key (in hex: 32 chars)\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   GetSamplingFrequencyIndex
+---------------------------------------------------------------------*/
static unsigned int
GetSamplingFrequencyIndex(unsigned int sampling_frequency)
{
    switch (sampling_frequency) {
        case 96000: return 0;
        case 88200: return 1;
        case 64000: return 2;
        case 48000: return 3;
        case 44100: return 4;
        case 32000: return 5;
        case 24000: return 6;
        case 22050: return 7;
        case 16000: return 8;
        case 12000: return 9;
        case 11025: return 10;
        case 8000:  return 11;
        case 7350:  return 12;
        default:    return 0;
    }
}

/*----------------------------------------------------------------------
|   WriteAdtsHeader
+---------------------------------------------------------------------*/
static AP4_Result
WriteAdtsHeader(AP4_ByteStream* output, 
                unsigned int    frame_size,
                unsigned int    sampling_frequency_index,
                unsigned int    channel_configuration)
{
	unsigned char bits[7];

	bits[0] = 0xFF;
	bits[1] = 0xF1; // 0xF9 (MPEG2)
    bits[2] = 0x40 | (sampling_frequency_index << 2) | (channel_configuration >> 2);
    bits[3] = ((channel_configuration&0x3)<<6) | ((frame_size+7) >> 11);
    bits[4] = ((frame_size+7) >> 3)&0xFF;
	bits[5] = (((frame_size+7) << 5)&0xFF) | 0x1F;
	bits[6] = 0xFC;

	return output->Write(bits, 7);

	/*
        0:  syncword 12 always: '111111111111' 
        12: ID 1 0: MPEG-4, 1: MPEG-2 
        13: layer 2 always: '00' 
        15: protection_absent 1  
        16: profile 2  
        18: sampling_frequency_index 4  
        22: private_bit 1  
        23: channel_configuration 3  
        26: original/copy 1  
        27: home 1  
        28: emphasis 2 only if ID == 0 

        ADTS Variable header: these can change from frame to frame 
        28: copyright_identification_bit 1  
        29: copyright_identification_start 1  
        30: aac_frame_length 13 length of the frame including header (in bytes) 
        43: adts_buffer_fullness 11 0x7FF indicates VBR 
        54: no_raw_data_blocks_in_frame 2  
        ADTS Error check 
        crc_check 16 only if protection_absent == 0 
   */
}

/*----------------------------------------------------------------------
|   DecryptAndWriteSamples
+---------------------------------------------------------------------*/
static void
DecryptAndWriteSamples(AP4_Track*             track, 
                       AP4_SampleDescription* sdesc, 
                       AP4_Byte*              key, 
                       AP4_ByteStream*        output)
{
    AP4_ProtectedSampleDescription* pdesc = AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, sdesc);
    if (pdesc == NULL) {
        fprintf(stderr, "ERROR: unable to obtain cipher info\n");
        return;
    }
    
    AP4_AudioSampleDescription* audio_desc = AP4_DYNAMIC_CAST(AP4_AudioSampleDescription, pdesc->GetOriginalSampleDescription());
    if (audio_desc == NULL) {
        fprintf(stderr, "ERROR: sample description is not audio\n");
        return;
    }
    unsigned int sampling_frequency_index = GetSamplingFrequencyIndex(audio_desc->GetSampleRate());
    unsigned int channel_configuration    = audio_desc->GetChannelCount();
    
    // create the decrypter
    AP4_SampleDecrypter* decrypter = AP4_SampleDecrypter::Create(pdesc, key, 16);
    if (decrypter == NULL) {
        fprintf(stderr, "ERROR: unable to create decrypter\n");
        return;
    }

    AP4_Sample     sample;
    AP4_DataBuffer encrypted_data;
    AP4_DataBuffer decrypted_data;
    AP4_Ordinal    index = 0;
    while (AP4_SUCCEEDED(track->ReadSample(index, sample, encrypted_data))) {
        if (AP4_FAILED(decrypter->DecryptSampleData(encrypted_data, decrypted_data))) {
            fprintf(stderr, "ERROR: failed to decrypt sample\n");
            return;
        }

	    WriteAdtsHeader(output, decrypted_data.GetDataSize(), sampling_frequency_index, channel_configuration);
        output->Write(decrypted_data.GetData(), decrypted_data.GetDataSize());
	    index++;
    }
}

/*----------------------------------------------------------------------
|   WriteSamples
+---------------------------------------------------------------------*/
static void
WriteSamples(AP4_Track*             track, 
             AP4_SampleDescription* sdesc,
             AP4_ByteStream*        output)
{
    AP4_AudioSampleDescription* audio_desc = AP4_DYNAMIC_CAST(AP4_AudioSampleDescription, sdesc);
    if (audio_desc == NULL) {
        fprintf(stderr, "ERROR: sample description is not audio\n");
        return;
    }
    unsigned int sampling_frequency_index = GetSamplingFrequencyIndex(audio_desc->GetSampleRate());
    unsigned int channel_configuration    = audio_desc->GetChannelCount();

    AP4_Sample     sample;
    AP4_DataBuffer data;
    AP4_Ordinal    index = 0;
    while (AP4_SUCCEEDED(track->ReadSample(index, sample, data))) {
	    WriteAdtsHeader(output, sample.GetSize(), sampling_frequency_index, channel_configuration);
        output->Write(data.GetData(), data.GetDataSize());
	    index++;
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    int return_value = 1;
    
    if (argc < 3) {
        PrintUsageAndExit();
    }
    
    // parse command line
    AP4_Result result;
    char** args = argv+1;
    unsigned char key[16];
    bool          key_option = false;
    if (!strcmp(*args, "--key")) {
        if (argc != 5) {
            fprintf(stderr, "ERROR: invalid command line\n");
            return 1;
        }
        ++args;
        if (AP4_ParseHex(*args++, key, 16)) {
            fprintf(stderr, "ERROR: invalid hex format for key\n");
            return 1;
        }
        key_option = true;
    }

    AP4_ByteStream* input  = NULL;
    AP4_File* input_file   = NULL;
    AP4_ByteStream* output = NULL;
    AP4_Movie*      movie  = NULL;
    AP4_Track*      audio_track = NULL;

	// create the input stream
    result = AP4_FileByteStream::Create(*args++, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
        goto end;
    }
    
	// create the output stream
    result = AP4_FileByteStream::Create(*args++, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output (%d)\n", result);
        goto end;
    }

	// open the file
    input_file = new AP4_File(*input);

    // get the movie
    AP4_SampleDescription* sample_description;
    movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        goto end;
    }

    // get the audio track
    audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    if (audio_track == NULL) {
        fprintf(stderr, "ERROR: no audio track found\n");
        goto end;
    }

    // check that the track is of the right type
    sample_description = audio_track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        goto end;
    }

    // show info
    AP4_Debug("Audio Track:\n");
    AP4_Debug("  duration: %ld ms\n", audio_track->GetDurationMs());
    AP4_Debug("  sample count: %ld\n", audio_track->GetSampleCount());

    switch (sample_description->GetType()) {
        case AP4_SampleDescription::TYPE_MPEG: {
            WriteSamples(audio_track, sample_description, output);
            return_value = 0;
            break;
        }

        case AP4_SampleDescription::TYPE_PROTECTED: 
            if (!key_option) {
                fprintf(stderr, "ERROR: encrypted tracks require a key\n");
                return_value = 1;
                break;
            }
            DecryptAndWriteSamples(audio_track, sample_description, key, output);
            result = 0;
            break;

        default:
            fprintf(stderr, "ERROR: unsupported sample type\n");
            return_value = 1;
            break;
    }

end:
    delete input_file;
    if (input) input->Release();
    if (output) output->Release();

    return return_value;
}

