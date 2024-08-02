/*****************************************************************
|
|    AP4 - MP4 File Comparator
|
|    Copyright 2002-2017 Axiomatic Systems, LLC
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
#include "Ap4BitStream.h"
#include "Ap4Mp4AudioInfo.h"
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 Diff - Version 1.0.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2017 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4diff [options] <input1> <input2>\n"
            );
    exit(1);
}

/*----------------------------------------------------------------------
|   DiffSamples
+---------------------------------------------------------------------*/
static void
DiffSamples(unsigned int index, AP4_DataBuffer& sample_data1, AP4_DataBuffer& sample_data2)
{
    if (sample_data1.GetDataSize() != sample_data2.GetDataSize()) {
        printf("!!! sample %d: sizes not equal: %d, %d\n", index, sample_data1.GetDataSize(), sample_data2.GetDataSize());
        return;
    }
    
    const AP4_UI08* data1 = (const AP4_UI08*)sample_data1.GetData();
    const AP4_UI08* data2 = (const AP4_UI08*)sample_data2.GetData();
    bool         in_diff = false;
    unsigned int diff_start = 0;
    for (unsigned int i=0; i<sample_data1.GetDataSize(); i++) {
        if (in_diff) {
            if (data1[i] == data2[i]) {
                // end of diff
                printf("!!! sample %d, %d differences at %d / %d\n", index, i-diff_start, diff_start, sample_data1.GetDataSize());
                in_diff = false;
            }
        } else {
            if (data1[i] != data2[i]) {
                diff_start = i;
                in_diff = true;
            }
        }
    }
    if (in_diff) {
        printf("!!! sample %d, %d differences at %d / %d\n", index, sample_data1.GetDataSize()-diff_start, diff_start, sample_data1.GetDataSize());
    }
}

/*----------------------------------------------------------------------
|   DiffFragments
+---------------------------------------------------------------------*/
static void
DiffFragments(AP4_Movie& movie1, AP4_ByteStream* stream1, AP4_Movie& movie2, AP4_ByteStream* stream2)
{
    stream1->Seek(0);
    stream2->Seek(0);
    AP4_LinearReader reader1(movie1, stream1);
    AP4_LinearReader reader2(movie2, stream2);
    AP4_List<AP4_Track>::Item* track_item1 = movie1.GetTracks().FirstItem();
    AP4_List<AP4_Track>::Item* track_item2 = movie2.GetTracks().FirstItem();
    while (track_item1 && track_item2) {
        reader1.EnableTrack(track_item1->GetData()->GetId());
        reader2.EnableTrack(track_item1->GetData()->GetId());
        track_item1 = track_item1->GetNext();
        track_item2 = track_item2->GetNext();
    }
    
    AP4_Sample     sample1;
    AP4_Sample     sample2;
    AP4_DataBuffer sample_data1;
    AP4_DataBuffer sample_data2;
    AP4_UI32       prev_track_id = 0;
    for(unsigned int i=0; ; i++) {
        AP4_UI32 track_id1 = 0;
        AP4_UI32 track_id2 = 0;
        AP4_Result result1 = reader1.ReadNextSample(sample1, sample_data1, track_id1);
        AP4_Result result2 = reader2.ReadNextSample(sample2, sample_data2, track_id2);
        if (AP4_SUCCEEDED(result1) && AP4_SUCCEEDED(result2)) {
            if (track_id1 != track_id2) {
                fprintf(stderr, "!!! sample %d, track ID 1 = %d, track ID 2 = %d\n", i, track_id1, track_id2);
                return;
            }

            if (track_id1 != prev_track_id) {
                printf("Track %d:\n", track_id1);
                prev_track_id = track_id1;
            }
            
            DiffSamples(i, sample_data1, sample_data2);
        } else {
            printf("### processed %d samples\n", i+1);
            break;
        }
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
    const char* filename1 = NULL;
    const char* filename2 = NULL;
    
    while (char* arg = *++argv) {
        if (filename1 == NULL) {
            filename1 = arg;
        } else if (filename2 == NULL) {
            filename2 = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
            return 1;
        }
    }
    if (filename1 == NULL || filename2 == NULL) {
        fprintf(stderr, "ERROR: filename missing\n");
        return 1;
    }

    AP4_ByteStream* input1 = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename1,
                                                   AP4_FileByteStream::STREAM_MODE_READ, 
                                                   input1);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file %s (%d)\n", filename1, result);
        return 1;
    }

    AP4_ByteStream* input2 = NULL;
    result = AP4_FileByteStream::Create(filename2,
                                        AP4_FileByteStream::STREAM_MODE_READ,
                                        input2);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file %s (%d)\n", filename2, result);
        return 1;
    }
    
    AP4_File* file1 = new AP4_File(*input1, true);
    AP4_File* file2 = new AP4_File(*input2, true);
    
    AP4_Movie* movie1 = file1->GetMovie();
    AP4_Movie* movie2 = file2->GetMovie();

    if (movie1 && movie2) {
        AP4_List<AP4_Track>& tracks1 = movie1->GetTracks();
        AP4_List<AP4_Track>& tracks2 = movie1->GetTracks();

        if (tracks1.ItemCount() != tracks2.ItemCount()) {
            fprintf(stderr, "### file 1 has %d tracks, file 2 has %d tracks\n", tracks1.ItemCount(), tracks2.ItemCount());
        } else {
            for (unsigned int i=0; i<tracks1.ItemCount(); i++) {
                AP4_Track* track1;
                AP4_Track* track2;
                tracks1.Get(i, track1);
                tracks2.Get(i, track2);
                printf("--- comparing track ID %d\n", track1->GetId());
            }
        }
        
        if (movie1->HasFragments() != movie2->HasFragments()) {
            fprintf(stderr, "### file 1 fragmented=%s, file2 fragmented=%s\n",
                    movie1->HasFragments() ? "true" : "false",
                    movie2->HasFragments() ? "true" : "false");
        }
        if (movie1->HasFragments() && movie2->HasFragments()) {
            DiffFragments(*movie1, input1, *movie2, input2);
        }
    }

    delete file1;
    delete file2;
    input1->Release();
    input2->Release();

    return 0;
}
