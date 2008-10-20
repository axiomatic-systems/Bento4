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
#define BANNER "Avc Track Writer Test - Version 1.0\n"\
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
            "\n\nusage: avctrackwritertest <input> <output>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   CopySamplesToSyntheticTable
+---------------------------------------------------------------------*/
void
CopySamplesToSyntheticTable(AP4_Track* input_track, AP4_SyntheticSampleTable* table) {
    AP4_Sample sample;
    AP4_Ordinal index = 0;
    
    while (AP4_SUCCEEDED(input_track->GetSample(index, sample))) {
        if (sample.GetDescriptionIndex() != 0) {
            // just for the test, fix that later... too lazy for now....
            fprintf(stderr, "ERROR: unable to write tracks with more than one sample desc");
        }
        AP4_ByteStream* data_stream = sample.GetDataStream();
        table->AddSample(*data_stream,
                         sample.GetOffset(),
                         sample.GetSize(),
                         sample.GetDescriptionIndex(),
                         sample.GetCts(),
                         sample.GetDts(),
                         sample.IsSync());
        AP4_RELEASE(data_stream);
        index++;
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc != 3) {
        PrintUsageAndExit();
    }
    
    AP4_ByteStream* input = NULL;
    try {
        input = new AP4_FileByteStream(argv[1],
                               AP4_FileByteStream::STREAM_MODE_READ);
    } catch (AP4_Exception) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", argv[1]);
        return 1;
    }
    
    // open the output
    AP4_ByteStream* output = new AP4_FileByteStream(
        argv[2],
        AP4_FileByteStream::STREAM_MODE_WRITE);
    
    // get the movie
    AP4_File* input_file = new AP4_File(*input);
    AP4_SampleDescription* sample_description = NULL;
    AP4_Movie* movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        return 1;
    }

    // get the video track
    AP4_Track* video_track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    if (video_track == NULL) {
        fprintf(stderr, "ERROR: no video track found\n");
        return 1;
    }

    // check that the track is of the right type
    sample_description = video_track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        return 1;
    }
    
    // get the info from the input sample description and feed it to the synthetic one
    if (sample_description->GetType() != AP4_SampleDescription::TYPE_AVC) {
        fprintf(stderr, "ERROR: wrong type for sample description\n");
        return 1;
    }
    AP4_AvcSampleDescription* avc_desc = dynamic_cast<AP4_AvcSampleDescription*>(sample_description);
    AP4_SyntheticSampleTable* video_sample_table = new AP4_SyntheticSampleTable();
    video_sample_table->AddSampleDescription(new AP4_AvcSampleDescription(
        avc_desc->GetWidth(),
        avc_desc->GetHeight(),
        avc_desc->GetDepth(),
        avc_desc->GetCompressorName(),
        avc_desc->GetConfigurationVersion(),
        avc_desc->GetProfile(),
        avc_desc->GetLevel(),
        avc_desc->GetProfileCompatibility(),
        avc_desc->GetNaluLengthSize(),
        avc_desc->GetSequenceParameters(),
        avc_desc->GetPictureParameters()));
        
    // add the samples
    CopySamplesToSyntheticTable(video_track, video_sample_table);
    
    // create the output objects    
    AP4_Track* output_video_track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                                  video_sample_table,
                                                  1, // track id
                                                  movie->GetTimeScale(),
                                                  video_track->GetDuration(),
                                                  video_track->GetMediaTimeScale(),
                                                  video_track->GetMediaDuration(),
                                                  video_track->GetTrackLanguage(),
                                                  avc_desc->GetWidth(),
                                                  avc_desc->GetHeight());
                                               
    // get the audio track
    AP4_Track* audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    if (audio_track == NULL) {
        fprintf(stderr, "ERROR: no audio track found\n");
        return 1;
    }

    // check that the track is of the right type
    sample_description = audio_track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        return 1;
    }
    
    // get the info from the input sample description and feed it to the synthetic one
    if (sample_description->GetType() != AP4_SampleDescription::TYPE_MPEG) {
        fprintf(stderr, "ERROR: wrong type for sample description\n");
        return 1;
    }
    AP4_MpegAudioSampleDescription* mpg_desc = dynamic_cast<AP4_MpegAudioSampleDescription*>(sample_description);
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
    CopySamplesToSyntheticTable(audio_track, audio_sample_table);
    
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

    // create a writer to write the file
    AP4_FileWriter* writer = new AP4_FileWriter(*output_file);

    // write the file to the output
    writer->Write(*output);
    
    // cleanup
    delete input_file;
    delete output_file;
    input->Release();
    output->Release();
    delete writer;

    return 0;
                                            
}

