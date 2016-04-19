/*****************************************************************
|
|    AP4 - MP4 to HEVC File Converter
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
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 To HEVC File Converter - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2014 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp42hevc [options] <input> <output>\n"
            "  Options:\n"
            "  --key <hex>: 128-bit decryption key (in hex: 32 chars)\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   WriteSample
+---------------------------------------------------------------------*/
static void
WriteSample(const AP4_DataBuffer& sample_data, 
            AP4_DataBuffer&       prefix, 
            unsigned int          nalu_length_size, 
            AP4_ByteStream*       output)
{
    const unsigned char* data      = sample_data.GetData();
    unsigned int         data_size = sample_data.GetDataSize();

    // detect if we have VPS/SPS/PPS and/or AUD NAL units already
    bool have_param_sets = false;
    bool have_access_unit_delimiter = false;
    while (data_size) {
        // sanity check
        if (data_size < nalu_length_size) break;
        
        // get the next NAL unit
        AP4_UI32 nalu_size;
        if (nalu_length_size == 1) {
            nalu_size = *data++;
            data_size--;
        } else if (nalu_length_size == 2) {
            nalu_size = AP4_BytesToInt16BE(data);
            data      += 2;
            data_size -= 2;
        } else if (nalu_length_size == 4) {
            nalu_size = AP4_BytesToInt32BE(data);
            data      += 4;
            data_size -= 4;
        } else {
            break;
        }
        if (nalu_size > data_size) break;
        
        unsigned int nal_unit_type = (data[0]>>1)&0x3F;
        if (nal_unit_type == AP4_HEVC_NALU_TYPE_AUD_NUT) {
            have_access_unit_delimiter = true;
        }
        if (nal_unit_type == AP4_HEVC_NALU_TYPE_VPS_NUT ||
            nal_unit_type == AP4_HEVC_NALU_TYPE_SPS_NUT ||
            nal_unit_type == AP4_HEVC_NALU_TYPE_PPS_NUT) {
            have_param_sets = true;
            break;
        }
        
        // move to the next NAL unit
        data      += nalu_size;
        data_size -= nalu_size;
    } 
    data      = sample_data.GetData();
    data_size = sample_data.GetDataSize();

    // allocate a buffer for the frame data
    AP4_DataBuffer frame_data;
    unsigned char* frame_buffer = NULL;
    
    // add a delimiter if we don't already have one
    if (data_size && !have_access_unit_delimiter) {
        AP4_Size frame_data_size = frame_data.GetDataSize();
        frame_data.SetDataSize(frame_data_size+7);
        frame_buffer = frame_data.UseData()+frame_data_size;
    
        // start of access unit
        frame_buffer[0] = 0;
        frame_buffer[1] = 0;
        frame_buffer[2] = 0;
        frame_buffer[3] = 1;
        frame_buffer[4] = AP4_HEVC_NALU_TYPE_AUD_NUT<<1;
        frame_buffer[5] = 1;
        frame_buffer[6] = 0x40; // pic_type = 2 (B,P,I)
    }
    
    // write the NAL units
    bool prefix_added = false;
    while (data_size) {
        // sanity check
        if (data_size < nalu_length_size) break;
        
        // get the next NAL unit
        AP4_UI32 nalu_size;
        if (nalu_length_size == 1) {
            nalu_size = *data++;
            data_size--;
        } else if (nalu_length_size == 2) {
            nalu_size = AP4_BytesToInt16BE(data);
            data      += 2;
            data_size -= 2;
        } else if (nalu_length_size == 4) {
            nalu_size = AP4_BytesToInt32BE(data);
            data      += 4;
            data_size -= 4;
        } else {
            break;
        }
        if (nalu_size > data_size) break;
        
        // add the prefix if needed
        if (!have_param_sets && !prefix_added && !have_access_unit_delimiter) {
            AP4_Size frame_data_size = frame_data.GetDataSize();
            frame_data.SetDataSize(frame_data_size+prefix.GetDataSize());
            frame_buffer = frame_data.UseData()+frame_data_size;
            AP4_CopyMemory(frame_buffer, prefix.GetData(), prefix.GetDataSize());
            prefix_added = true;
        }
        
        // add a start code before the NAL unit
        AP4_Size frame_data_size = frame_data.GetDataSize();
        frame_data.SetDataSize(frame_data_size+3+nalu_size);
        frame_buffer = frame_data.UseData()+frame_data_size;
        frame_buffer[0] = 0;
        frame_buffer[1] = 0;
        frame_buffer[2] = 1;
        AP4_CopyMemory(frame_buffer+3, data, nalu_size);
        
        // add the prefix if needed
        if (!have_param_sets && !prefix_added) {
            frame_data_size = frame_data.GetDataSize();
            frame_data.SetDataSize(frame_data_size+prefix.GetDataSize());
            frame_buffer = frame_data.UseData()+frame_data_size;
            AP4_CopyMemory(frame_buffer, prefix.GetData(), prefix.GetDataSize());
            prefix_added = true;
        }

        // move to the next NAL unit
        data      += nalu_size;
        data_size -= nalu_size;
    } 
    
    output->Write(frame_data.GetData(), frame_data.GetDataSize());
}

/*----------------------------------------------------------------------
|   MakeFramePrefix
+---------------------------------------------------------------------*/
static AP4_Result
MakeFramePrefix(AP4_SampleDescription* sdesc, AP4_DataBuffer& prefix, unsigned int& nalu_length_size)
{
    AP4_HevcSampleDescription* hevc_desc = AP4_DYNAMIC_CAST(AP4_HevcSampleDescription, sdesc);
    if (hevc_desc == NULL) {
        fprintf(stderr, "ERROR: track does not contain an HEVC stream\n");
        return AP4_FAILURE;
    }
    
    // extract the nalu length size
    nalu_length_size = hevc_desc->GetNaluLengthSize();

    // make the VPS/SPS/PPS prefix
    for (unsigned int i=0; i<hevc_desc->GetSequences().ItemCount(); i++) {
        const AP4_HvccAtom::Sequence& seq = hevc_desc->GetSequences()[i];
        if (seq.m_NaluType == AP4_HEVC_NALU_TYPE_VPS_NUT) {
            for (unsigned int j=0; j<seq.m_Nalus.ItemCount(); j++) {
                const AP4_DataBuffer& buffer = seq.m_Nalus[j];
                unsigned int prefix_size = prefix.GetDataSize();
                prefix.SetDataSize(prefix_size+4+buffer.GetDataSize());
                unsigned char* p = prefix.UseData()+prefix_size;
                *p++ = 0;
                *p++ = 0;
                *p++ = 0;
                *p++ = 1;
                AP4_CopyMemory(p, buffer.GetData(), buffer.GetDataSize());
            }
        }
    }

    for (unsigned int i=0; i<hevc_desc->GetSequences().ItemCount(); i++) {
        const AP4_HvccAtom::Sequence& seq = hevc_desc->GetSequences()[i];
        if (seq.m_NaluType == AP4_HEVC_NALU_TYPE_SPS_NUT) {
            for (unsigned int j=0; j<seq.m_Nalus.ItemCount(); j++) {
                const AP4_DataBuffer& buffer = seq.m_Nalus[j];
                unsigned int prefix_size = prefix.GetDataSize();
                prefix.SetDataSize(prefix_size+4+buffer.GetDataSize());
                unsigned char* p = prefix.UseData()+prefix_size;
                *p++ = 0;
                *p++ = 0;
                *p++ = 0;
                *p++ = 1;
                AP4_CopyMemory(p, buffer.GetData(), buffer.GetDataSize());
            }
        }
    }

    for (unsigned int i=0; i<hevc_desc->GetSequences().ItemCount(); i++) {
        const AP4_HvccAtom::Sequence& seq = hevc_desc->GetSequences()[i];
        if (seq.m_NaluType == AP4_HEVC_NALU_TYPE_PPS_NUT) {
            for (unsigned int j=0; j<seq.m_Nalus.ItemCount(); j++) {
                const AP4_DataBuffer& buffer = seq.m_Nalus[j];
                unsigned int prefix_size = prefix.GetDataSize();
                prefix.SetDataSize(prefix_size+4+buffer.GetDataSize());
                unsigned char* p = prefix.UseData()+prefix_size;
                *p++ = 0;
                *p++ = 0;
                *p++ = 0;
                *p++ = 1;
                AP4_CopyMemory(p, buffer.GetData(), buffer.GetDataSize());
            }
        }
    }
    
    return AP4_SUCCESS;
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
    
    // get the original sample description and make the prefix
    AP4_SampleDescription* orig_sdesc = pdesc->GetOriginalSampleDescription();
    unsigned int   nalu_length_size = 0;
    AP4_DataBuffer prefix;
    if (AP4_FAILED(MakeFramePrefix(orig_sdesc, prefix, nalu_length_size))) {
        return;
    }
    
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
        WriteSample(decrypted_data, prefix, nalu_length_size, output);
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
    // make the frame prefix
    unsigned int   nalu_length_size = 0;
    AP4_DataBuffer prefix;
    if (AP4_FAILED(MakeFramePrefix(sdesc, prefix, nalu_length_size))) {
        return;
    }

    AP4_Sample     sample;
    AP4_DataBuffer data;
    AP4_Ordinal    index = 0;
    while (AP4_SUCCEEDED(track->ReadSample(index, sample, data))) {
        WriteSample(data, prefix, nalu_length_size, output);
	    index++;
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
    AP4_Track* video_track;
    AP4_Movie* movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        goto end;
    }

    // get the video track
    video_track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    if (video_track == NULL) {
        fprintf(stderr, "ERROR: no video track found\n");
        goto end;
    }

    // check that the track is of the right type
    sample_description = video_track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        goto end;
    }

    // show info
    AP4_Debug("Video Track:\n");
    AP4_Debug("  duration: %ld ms\n", video_track->GetDurationMs());
    AP4_Debug("  sample count: %ld\n", video_track->GetSampleCount());

    switch (sample_description->GetType()) {
        case AP4_SampleDescription::TYPE_HEVC:
            WriteSamples(video_track, sample_description, output);
            break;

        case AP4_SampleDescription::TYPE_PROTECTED: 
            if (!key_option) {
                fprintf(stderr, "ERROR: encrypted tracks require a key\n");
                goto end;
            }
            DecryptAndWriteSamples(video_track, sample_description, key, output);
            break;

        default:
            fprintf(stderr, "ERROR: unsupported sample type\n");
            break;
    }

end:
    delete input_file;
    input->Release();
    output->Release();

    return 0;
}

