/*****************************************************************
|
|    AP4 - Avc Track Writer Test
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
#define BANNER "Avc Track Writer Test - Version 1.1\n"\
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
            "\n\nusage: avctrackwritertest [options] <input> <output>\n"
            "options:\n"
            "  --key <n>:<key>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   DecryptingByteStream
|
|   This class represents encrypted samples stored on disk and presents
|   them as a byte stream, decrypting the data on demand. This way,
|   we can add samples that point to these streams to the sample table
|   without having to keep the data in memory: the data is read and 
|   decrypted on demand when the samples are written to disk.
+---------------------------------------------------------------------*/
class DecryptingByteStream : public AP4_ByteStream 
{
public:
    DecryptingByteStream(AP4_SampleDecrypter* decrypter,
                         AP4_Sample&          sample) : 
        m_ReferenceCount(1), 
        m_Position(0),
        m_Decrypter(decrypter) {
        // keep a reference to the source stream, and the offset
        // and size of the sample data in that stream.
        m_SourceData       = sample.GetDataStream();
        m_SourceDataOffset = sample.GetOffset();
        m_SourceDataSize   = sample.GetSize();
        
        // compute the size of the sample data after decryption
        m_Size = decrypter->GetDecryptedSampleSize(sample);
    }
    ~DecryptingByteStream() {
        // release the reference to the source stream
        m_SourceData->Release();
    }
    
    // AP4_ByteStream methods
    virtual AP4_Result ReadPartial(void*     buffer, 
                                   AP4_Size  bytes_to_read, 
                                   AP4_Size& bytes_read) {
        // check the data bounds
        bytes_read = 0;
        if (m_Position+bytes_to_read > m_Size) {
            return AP4_ERROR_OUT_OF_RANGE;
        }
        
        // read the encrypted sample data
        AP4_DataBuffer encrypted_data;
        encrypted_data.SetBufferSize(m_SourceDataSize);
        encrypted_data.SetDataSize(m_SourceDataSize);
        AP4_Result result;
        result = m_SourceData->Seek(m_SourceDataOffset);
        if (AP4_FAILED(result)) return result;
        result = m_SourceData->Read(encrypted_data.UseData(), m_SourceDataSize);
        if (AP4_FAILED(result)) return result;
        
        // decrypt the sample data
        AP4_DataBuffer cleartext_data;
        result = m_Decrypter->DecryptSampleData(encrypted_data, cleartext_data);
        if (AP4_FAILED(result)) return result;
        
        // copy the data to the caller's buffer
        AP4_CopyMemory(buffer, cleartext_data.GetData()+m_Position, bytes_to_read);
        bytes_read = bytes_to_read;
        return AP4_SUCCESS;
    }
    virtual AP4_Result WritePartial(const void* /* buffer         */, 
                                    AP4_Size    /* bytes_to_write */, 
                                    AP4_Size&   /* bytes_written  */) {
        return AP4_ERROR_NOT_SUPPORTED;
    }
    virtual AP4_Result Seek(AP4_Position position) {
        m_Position = position;
        return AP4_SUCCESS;
    }
    virtual AP4_Result Tell(AP4_Position& position) {
        position = m_Position;
        return AP4_SUCCESS;
    }
    virtual AP4_Result GetSize(AP4_LargeSize& size) {
        size = m_Size;
        return AP4_SUCCESS;
    }
    
    // AP4_Referenceable methods
    void AddReference() { m_ReferenceCount++;                       }
    void Release()      { if (--m_ReferenceCount == 0) delete this; }

private:
    AP4_Cardinal         m_ReferenceCount;
    AP4_Position         m_Position;
    AP4_Size             m_Size;
    AP4_SampleDecrypter* m_Decrypter;
    AP4_ByteStream*      m_SourceData;
    AP4_Position         m_SourceDataOffset;
    AP4_Size             m_SourceDataSize;
};

/*----------------------------------------------------------------------
|   CopySamplesToSyntheticTable
+---------------------------------------------------------------------*/
void
CopySamplesToSyntheticTable(AP4_Track*                track, 
                            AP4_SampleDecrypter*      track_decrypter,
                            AP4_SyntheticSampleTable* table) {
    AP4_Sample  sample;
    AP4_Ordinal index = 0;
    
    // check if the samples are encrypted
    AP4_SampleDescription* sdesc = track->GetSampleDescription(0);
    if (sdesc == NULL) {
        fprintf(stderr, "ERROR: failed to get sample description");
    }
    
    while (AP4_SUCCEEDED(track->GetSample(index, sample))) {
        if (sample.GetDescriptionIndex() != 0) {
            // just for the test, fix that later... too lazy for now....
            fprintf(stderr, "ERROR: unable to write tracks with more than one sample desc");
        }
        
        AP4_ByteStream* data_stream;
        if (track_decrypter) {
            data_stream = new DecryptingByteStream(track_decrypter, sample);
            AP4_LargeSize sample_size;
            data_stream->GetSize(sample_size);
            table->AddSample(*data_stream,
                             0,
                             (AP4_Size)sample_size,
                             sample.GetDescriptionIndex(),
                             sample.GetDuration(),
                             sample.GetDts(),
                             sample.GetCtsDelta(),
                             sample.IsSync());
        } else {
            data_stream = sample.GetDataStream();
            table->AddSample(*data_stream,
                             sample.GetOffset(),
                             sample.GetSize(),
                             sample.GetDuration(),
                             sample.GetDescriptionIndex(),
                             sample.GetDts(),
                             sample.GetCtsDelta(),
                             sample.IsSync());
        }
        AP4_RELEASE(data_stream); // release our ref, the table has kept its own ref.
        index++;
    }
}

/*----------------------------------------------------------------------
|   CheckProtection
+---------------------------------------------------------------------*/
static bool
CheckProtection(AP4_SampleDescription*& sample_description,
                AP4_UI32                track_id,
                AP4_ProtectionKeyMap&   key_map,
                AP4_SampleDecrypter*&   decrypter)
{
    // check if the sample description indicates an encrypted track
    if (sample_description->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
        AP4_ProtectedSampleDescription* prot_desc = 
            AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, sample_description);
            
        printf("INFO: track %d is encrypted\n", track_id);
        // check that we have a key
        const AP4_DataBuffer* key = key_map.GetKey(track_id);
        if (key == NULL) {
            fprintf(stderr, "ERROR: no key given for track %d\n", track_id);
            return false;
        }
        
        // create the decrypter
        decrypter = AP4_SampleDecrypter::Create(prot_desc, key->GetData(), key->GetDataSize());
        if (decrypter == NULL) {
            fprintf(stderr, "ERROR: unable to create decrypter\n");
            return false;
        }
        
        // use the sample description wrapped by the encrypted description
        sample_description = prot_desc->GetOriginalSampleDescription();
    }
    
    return true;
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
    const char* input_filename = NULL;
    const char* output_filename = NULL;
    AP4_ProtectionKeyMap key_map;

    // parse the command line
    ++argv;
    char* arg;
    while ((arg = *argv++)) {
        if (!strcmp(arg, "--key")) {
            arg = *argv++;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --key option\n");
                return 1;
            }
            char* track_ascii = arg;
            char* key_ascii = NULL;
            char* delimiter = strchr(arg, ':');
            if (delimiter != NULL) {
                *delimiter = '\0';
                key_ascii = delimiter+1;  
            }

            // get the track id
            AP4_Ordinal track_id = (AP4_Ordinal) strtoul(track_ascii, NULL, 10); 

            // see if we have a key for this track
            if (key_ascii != NULL) {
                unsigned char key[16];
                if (AP4_ParseHex(key_ascii, key, 16)) {
                    fprintf(stderr, "ERROR: invalid hex format for key\n");
                    return 1;
                }
                // set the key in the map
                key_map.SetKey(track_id, key, 16);
            }
        } else if (input_filename == NULL) {
            input_filename = arg;
        } else if (output_filename == NULL) {
            output_filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument (%s)\n", arg);
        }
    }
    
    // check args
    if (input_filename == NULL) {
        fprintf(stderr, "ERROR: no input filename specified\n");
        return 1;
    }
    if (output_filename == NULL) {
        fprintf(stderr, "ERROR: no output filename specified\n");
        return 1;
    }
    
    AP4_Result result;
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }
    
    // open the output
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%s)\n", output_filename);
        return 1;
    }
    
    // get the movie
    AP4_File* input_file = new AP4_File(*input);
    AP4_SampleDescription* sample_description = NULL;
    AP4_Movie* movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        return 1;
    }

    // get the video track
    AP4_Track*           video_track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    AP4_SampleDecrypter* video_decrypter = NULL;
    if (video_track == NULL) {
        fprintf(stderr, "ERROR: no video track found\n");
        return 1;
    }

    // get the sample description (only one sample desc per track supported)
    sample_description = video_track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        return 1;
    }
        
    // check if the track is encrypted
    if (!CheckProtection(sample_description, video_track->GetId(), key_map, video_decrypter)) {
        return 1;
    }
    
    // check the sample description
    if (sample_description->GetType() != AP4_SampleDescription::TYPE_AVC) {
        fprintf(stderr, "ERROR: wrong type for sample description\n");
        return 1;
    }
    AP4_AvcSampleDescription* avc_desc = AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, sample_description);
    AP4_SyntheticSampleTable* video_sample_table = new AP4_SyntheticSampleTable();
    video_sample_table->AddSampleDescription(new AP4_AvcSampleDescription(
        AP4_SAMPLE_FORMAT_AVC1,
        avc_desc->GetWidth(),
        avc_desc->GetHeight(),
        avc_desc->GetDepth(),
        avc_desc->GetCompressorName(),
        avc_desc->GetProfile(),
        avc_desc->GetLevel(),
        avc_desc->GetProfileCompatibility(),
        avc_desc->GetNaluLengthSize(),
        avc_desc->GetSequenceParameters(),
        avc_desc->GetPictureParameters()));
        
    // add the samples
    CopySamplesToSyntheticTable(video_track, video_decrypter, video_sample_table);
    
    // create the output objects    
    AP4_Track* output_video_track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                                  video_sample_table,
                                                  1, // track id
                                                  movie->GetTimeScale(),
                                                  video_track->GetDuration(),
                                                  video_track->GetMediaTimeScale(),
                                                  video_track->GetMediaDuration(),
                                                  video_track->GetTrackLanguage(),
                                                  video_track->GetWidth(),
                                                  video_track->GetHeight());
                                               
    // get the audio track
    AP4_Track*           audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    AP4_SampleDecrypter* audio_decrypter = NULL;
    if (audio_track == NULL) {
        fprintf(stderr, "ERROR: no audio track found\n");
        return 1;
    }

    // get the sample description (only one sample desc per track supported)
    sample_description = audio_track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        return 1;
    }
    
    // check if the track is encrypted
    if (!CheckProtection(sample_description, audio_track->GetId(), key_map, audio_decrypter)) {
        return 1;
    }
    
    // check the sample description
    if (sample_description->GetType() != AP4_SampleDescription::TYPE_MPEG) {
        fprintf(stderr, "ERROR: wrong type for sample description\n");
        return 1;
    }
    AP4_MpegAudioSampleDescription* mpg_desc = AP4_DYNAMIC_CAST(AP4_MpegAudioSampleDescription, sample_description);
    AP4_SyntheticSampleTable* audio_sample_table = new AP4_SyntheticSampleTable();
    audio_sample_table->AddSampleDescription(new AP4_MpegAudioSampleDescription(
        mpg_desc->GetObjectTypeId(),
        mpg_desc->GetSampleRate(),
        mpg_desc->GetSampleSize(),
        mpg_desc->GetChannelCount(),
        &mpg_desc->GetDecoderInfo(),
        mpg_desc->GetBufferSize(),
        mpg_desc->GetMaxBitrate(),
        mpg_desc->GetAvgBitrate()));
        
    // add the samples
    CopySamplesToSyntheticTable(audio_track, audio_decrypter, audio_sample_table);
    
    // create the output objects    
    AP4_Track* output_audio_track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                                  audio_sample_table,
                                                  2, // track id
                                                  movie->GetTimeScale(),
                                                  audio_track->GetDuration(),
                                                  audio_track->GetMediaTimeScale(),
                                                  audio_track->GetMediaDuration(),
                                                  audio_track->GetTrackLanguage(),
                                                  0,
                                                  0);

    // put the movie together
    AP4_Movie* output_movie = new AP4_Movie(movie->GetTimeScale());
    output_movie->AddTrack(output_video_track);
    output_movie->AddTrack(output_audio_track);
    AP4_File* output_file = new AP4_File(output_movie);    
    output_file->SetFileType(input_file->GetFileType()->GetMajorBrand(),
                             input_file->GetFileType()->GetMinorVersion(),
                             &input_file->GetFileType()->GetCompatibleBrands()[0],
                             input_file->GetFileType()->GetCompatibleBrands().ItemCount());

    // write the file to the output
    AP4_FileWriter::Write(*output_file, *output);
    
    // cleanup
    delete input_file;
    delete output_file;
    input->Release();
    output->Release();

    return 0;
                                            
}

