/*****************************************************************
|
|    AP4 - Passthrough Writer Test
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
#define BANNER "Passthrough Writer Test - Version 1.0\n"\
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
            "\n\nusage: passthroughwritertest [options] <input> <output>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   CopyTrack
+---------------------------------------------------------------------*/
static void
CopyTrack(AP4_Movie* input_movie, AP4_Track* input_track, AP4_Movie* output_movie)
{
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();
    AP4_SampleDescription* sample_description = input_track->GetSampleDescription(0);
    
#if 0
    if (input_track->GetType() == AP4_Track::TYPE_VIDEO) {
        AP4_ContainerAtom* schi = new AP4_ContainerAtom(AP4_ATOM_TYPE_SCHI);
        sample_description = new AP4_ProtectedSampleDescription(
            AP4_ATOM_TYPE_ENCV,
            sample_description,
            sample_description->GetFormat(),
            AP4_PROTECTION_SCHEME_TYPE_OMA,
            AP4_PROTECTION_SCHEME_VERSION_OMA_20,
            "http://foo.bar/prot",
            schi,
            false);
        delete schi;
    } 
#endif
    
    // add the description to the table without transfering ownership
    sample_table->AddSampleDescription(sample_description, false);

    AP4_Sample  sample;
    AP4_Ordinal index = 0;
    printf("Copying track %d\n", input_track->GetId());
    while (AP4_SUCCEEDED(input_track->GetSample(index, sample))) {
        if (sample.GetDescriptionIndex() != 0) {
            // just for the test, fix that later... too lazy for now....
            fprintf(stderr, "ERROR: unable to write tracks with more than one sample desc");
        }
        
        AP4_ByteStream* data_stream;
        data_stream = sample.GetDataStream();
        sample_table->AddSample(*data_stream,
                                sample.GetOffset(),
                                sample.GetSize(),
                                sample.GetDuration(),
                                sample.GetDescriptionIndex(),
                                sample.GetDts(),
                                sample.GetCtsDelta(),
                                sample.IsSync());
        AP4_RELEASE(data_stream); // release our ref, the table has kept its own ref.
        index++;
    }    
    printf("Copied %d samples\n", index);
    
    // create an output track
    AP4_Track* output_track = new AP4_Track(input_track->GetType(),
                                            sample_table,
                                            input_track->GetId(),
                                            input_movie->GetTimeScale(),
                                            input_track->GetDuration(),
                                            input_track->GetMediaTimeScale(),
                                            input_track->GetMediaDuration(),
                                            input_track->GetTrackLanguage(),
                                            input_track->GetWidth(),
                                            input_track->GetHeight());
    
    // add the track to the output movie
    output_movie->AddTrack(output_track);
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
    const char* input_filename  = argv[1];
    const char* output_filename = argv[2];
    
    // open the input
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
    AP4_Movie* input_movie = input_file->GetMovie();
    if (input_movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        return 1;
    }

    AP4_Movie* output_movie = new AP4_Movie(input_movie->GetTimeScale());
    AP4_File* output_file = new AP4_File(output_movie);    
    output_file->SetFileType(input_file->GetFileType()->GetMajorBrand(),
                             input_file->GetFileType()->GetMinorVersion(),
                             &input_file->GetFileType()->GetCompatibleBrands()[0],
                             input_file->GetFileType()->GetCompatibleBrands().ItemCount());

    // copy all tracks
    AP4_List<AP4_Track>& tracks = input_movie->GetTracks();
    AP4_List<AP4_Track>::Item* track_item = tracks.FirstItem();
    while (track_item) {
        AP4_Track* track = track_item->GetData();
        CopyTrack(input_movie, track, output_movie);
        track_item = track_item->GetNext();
    }
    
    // write the file to the output
    AP4_FileWriter::Write(*output_file, *output);
    
    // cleanup
    delete input_file;
    delete output_file;
    input->Release();
    output->Release();

    return 0;                                            
}

