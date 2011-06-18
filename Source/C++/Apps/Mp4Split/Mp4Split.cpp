/*****************************************************************
|
|    AP4 - MP4 fragmented file splitter
|
|    Copyright 2002-2011 Axiomatic Systems, LLC
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
#define BANNER "MP4 Fragment Splitter - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2011 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
struct Options {
    bool        verbose;
    const char* input;
    const char* init_segment_name;
    const char* media_segment_pattern;
} Options;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4split [options] <input>\n"
            "Options:\n"
            "  --verbose\n");
    exit(1);
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
    
    // default options
    Options.verbose               = false;
    Options.input                 = NULL;
    Options.init_segment_name     = "init-segment.mp4";
    Options.media_segment_pattern = NULL;
    
    // parse command line
    AP4_Result result;
    char** args = argv+1;
    while (const char* arg = *args++) {
        if (!strcmp(arg, "--verbose")) {
            Options.verbose = true;
        } else if (!strcmp(arg, "--init-segment")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: missing argument after --init-segment option\n");
                return 1;
            }
            Options.init_segment_name = *args++;
        } else if (Options.input == NULL) {
            Options.input = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument\n");
            return 1;
        }
    }

    // check args
    if (Options.input == NULL) {
        fprintf(stderr, "ERROR: missing input file name\n");
        return 1;
    }
    
	// create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(Options.input, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
        return 1;
    }
    
    // get the movie
    AP4_File* file = new AP4_File(*input, AP4_DefaultAtomFactory::Instance, true);
    AP4_Movie* movie = file->GetMovie();
    
    // save the init segment
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(Options.init_segment_name, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%d)\n", result);
        return 1;
    }
    result = movie->GetMoovAtom()->Write(*output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot write init segment (%d)\n", result);
        return 1;
    }
    
#if 0
    AP4_Atom* atom = NULL;
    do {
        // process the next atom
        result = AP4_DefaultAtomFactory::Instance.CreateAtomFromStream(*input, atom);
        if (AP4_SUCCEEDED(result)) {
            printf("atom size=%lld\n", atom->GetSize());
            if (atom->GetType() == AP4_ATOM_TYPE_MOOF) {
                AP4_ContainerAtom* moof = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
                if (moof) {
                    // remember where we are in the stream
                    AP4_Position position = 0;
                    input->Tell(position);
        
                    // process the movie fragment
                    ProcessMoof(movie, moof, input, position-atom->GetSize(), position+8);

                    // go back to where we were before processing the fragment
                    input->Seek(position);
                }
            } else {
                delete atom;
            }            
        }
    } while (AP4_SUCCEEDED(result));
#endif

    // cleanup
    delete file;
    input->Release();
    
    return 0;
}

