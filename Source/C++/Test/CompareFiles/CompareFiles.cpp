/*****************************************************************
|
|    AP4 - Basic Test
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
#define BANNER "File Comparison Test - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2009 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: comparefilestest <file1> <file2>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define CHECK(x)                                                          \
do  {                                                                     \
  if (!(x)) {                                                             \
    fprintf(stderr, "CHECK FAILED: %s, line %d\n", #x, __LINE__);   \
    return 1;                                                             \
  }                                                                       \
} while(0)

/*----------------------------------------------------------------------
|   CompareBuffers
+---------------------------------------------------------------------*/
static bool
CompareBuffers(AP4_DataBuffer& data1, AP4_DataBuffer& data2)
{
    const AP4_Byte* b1 = data1.GetData();
    const AP4_Byte* b2 = data2.GetData();
    for (unsigned int i=0; i<data1.GetDataSize(); i++) {
        if (b1[i] != b2[i]) return false;
    }
    
    return true;
}

/*----------------------------------------------------------------------
|   CompareTrackSamples
+---------------------------------------------------------------------*/
static int
CompareTrackSamples(AP4_Movie& movie1, AP4_ByteStream* input1, AP4_Movie& movie2, AP4_ByteStream* input2, unsigned int track_id)
{
    AP4_LinearReader reader1(movie1, input1);
    AP4_LinearReader reader2(movie2, input2);

    reader1.EnableTrack(track_id);
    reader2.EnableTrack(track_id);
    
    AP4_Sample sample1;
    AP4_Sample sample2;
    AP4_DataBuffer data1;
    AP4_DataBuffer data2;
    
    AP4_UI32 track_id_1;
    AP4_UI32 track_id_2;
    
    for (;;) {
        AP4_Result result1 = reader1.GetNextSample(sample1, track_id_1);
        AP4_Result result2 = reader2.GetNextSample(sample2, track_id_2);
        CHECK(result1 == result2);
        if (result1 == AP4_SUCCESS) {
            CHECK(sample1.GetCts()              == sample2.GetCts());
            CHECK(sample1.GetDts()              == sample2.GetDts());
            CHECK(sample1.GetSize()             == sample2.GetSize());
            CHECK(sample1.GetDescriptionIndex() == sample2.GetDescriptionIndex());
            CHECK(data1.GetDataSize()           == data2.GetDataSize());
            CHECK(CompareBuffers(data1, data2));
        } else {
            CHECK(result1 == AP4_ERROR_EOS);
            break;
        }
    }

    return 0;
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
    const char* filename1 = argv[1];
    const char* filename2 = argv[2];
    
    // open the first file
    AP4_ByteStream* input1 = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename1, AP4_FileByteStream::STREAM_MODE_READ, input1);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file 1 (%s)\n", filename1);
        return 1;
    }
    
    // open the second file
    AP4_ByteStream* input2 = NULL;
    result = AP4_FileByteStream::Create(filename2, AP4_FileByteStream::STREAM_MODE_READ, input2);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file 2 (%s)\n", filename2);
        return 1;
    }
        
    // parse the files
    AP4_File* mp4_file1 = new AP4_File(*input1);
    input1->Release();
    AP4_Movie* movie1 = mp4_file1->GetMovie();
    AP4_File* mp4_file2 = new AP4_File(*input2);
    input2->Release();
    AP4_Movie* movie2 = mp4_file2->GetMovie();
    
    AP4_FtypAtom* ftyp1 = mp4_file1->GetFileType();
    AP4_FtypAtom* ftyp2 = mp4_file2->GetFileType();
    CHECK(!(ftyp1 == NULL && ftyp2 != NULL));
    CHECK(!(ftyp2 == NULL && ftyp1 != NULL));
    CHECK(movie1->GetDuration()   == movie2->GetDuration());
    CHECK(movie1->GetDurationMs() == movie2->GetDurationMs());
    CHECK(movie1->HasFragments()  == movie2->HasFragments());
    CHECK(movie1->GetTracks().ItemCount() == movie2->GetTracks().ItemCount());
    for (unsigned int i=0; i<movie1->GetTracks().ItemCount(); i++) {
        AP4_Track* track1 = NULL;
        AP4_Track* track2 = NULL;
        CHECK(AP4_SUCCEEDED(movie1->GetTracks().Get(i, track1)));
        CHECK(AP4_SUCCEEDED(movie2->GetTracks().Get(i, track2)));
        CHECK(track1->GetId()          == track2->GetId());
        CHECK(track1->GetDuration()    == track2->GetDuration());
        CHECK(track1->GetDurationMs()  == track2->GetDurationMs());
        CHECK(track1->GetSampleCount() == track2->GetSampleCount());
        result = CompareTrackSamples(*movie1, input1, *movie2, input2, track1->GetId());
        if (result) return result;
    }
    
    // cleanup
    delete mp4_file1;
    delete mp4_file2;

    return 0;                                            
}

