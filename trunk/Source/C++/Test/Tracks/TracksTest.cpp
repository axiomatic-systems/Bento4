/*****************************************************************
|
|    AP4 - Tracks Test
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
|   macros
+---------------------------------------------------------------------*/
#define CHECK(x) do { \
    if (!(x)) { fprintf(stderr, "ERROR line %d", __LINE__); return -1; }\
} while (0)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "Tracks Test - Version 1.0\n"\
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
            "\n\nusage: trackstest <path-to-file-test-002.mp4>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc != 2) {
        PrintUsageAndExit();
    }
    const char* input_filename  = argv[1];
    
    // open the input
    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }
        
    // get the movie
    AP4_File* file = new AP4_File(*input);
    AP4_Movie* movie = file->GetMovie();
    CHECK(movie != NULL);
    
    AP4_Track* video_track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    CHECK(video_track != NULL);
    AP4_Ordinal index;
    index = video_track->GetNearestSyncSampleIndex(0, true);
    CHECK(index == 0);
    index = video_track->GetNearestSyncSampleIndex(0, false);
    CHECK(index == 0);
    index = video_track->GetNearestSyncSampleIndex(1, true);
    CHECK(index == 0);
    index = video_track->GetNearestSyncSampleIndex(1, false);
    CHECK(index == 12);
    index = video_track->GetNearestSyncSampleIndex(52, true);
    CHECK(index == 48);
    index = video_track->GetNearestSyncSampleIndex(52, false);
    CHECK(index == video_track->GetSampleCount());
    
    // cleanup
    delete file;
    input->Release();

    return 0;                                            
}

