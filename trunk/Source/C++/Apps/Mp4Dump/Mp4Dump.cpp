/*****************************************************************
|
|    AP4 - MP4 File Dumper
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf
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
#define BANNER "MP4 File Dumper - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2006 Gilles Boccon-Gibod & Julien Boeuf"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4dump [options] <input>\n"
            "options are:\n"
            "  -track <track_id> <data_file>: writes the track data in data_file\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   DumpTrackData
+---------------------------------------------------------------------*/
void
DumpTrackData(AP4_File* mp4_file, AP4_UI32 track_id, AP4_ByteStream* track_data)
{
    // get the track
    AP4_Track* track = mp4_file->GetMovie()->GetTrack(track_id);
    if (track == NULL) {
        fprintf(stderr, "track not found (id = %d)", track_id);
        return;
    }

    // write the data
    AP4_Sample sample;
    AP4_DataBuffer sample_data;
    AP4_Ordinal index = 0;
        
    while (AP4_SUCCEEDED(track->ReadSample(index, sample, sample_data))) {
        track_data->Write(sample_data.GetData(), sample_data.GetDataSize());
        index++;
    }
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

    // init the variables
    AP4_UI32        track_id = 0;
    AP4_ByteStream* input = NULL;
    AP4_ByteStream* track_data = NULL;

    // parse the command line
    argv++;
    char* arg;
    while ((arg = *argv++)) {
        if (!strcmp(arg, "-track")) {
            if (argv[0] && argv[1] && argv[2]) {
                track_id = atoi(argv[0]);
                if (track_id == 0) PrintUsageAndExit();
                track_data = 
                    new AP4_FileByteStream(argv[1], 
                                           AP4_FileByteStream::STREAM_MODE_WRITE);
                argv += 2;
            } else {
                PrintUsageAndExit();
            }
        } else {
            try {
                input = 
                    new AP4_FileByteStream(arg,
                    AP4_FileByteStream::STREAM_MODE_READ);
            } catch(AP4_Exception e) {
                AP4_Debug("ERROR: cannot open input (%d)\n", e.m_Error);
                return 1;
            }
        }
    }

    // open the output
    AP4_ByteStream* output =
        new AP4_FileByteStream("-stdout",
                               AP4_FileByteStream::STREAM_MODE_WRITE);
    
    // create an inspector
    AP4_PrintInspector inspector(*output);

    // inspect the atoms one by one
    AP4_Atom* atom;
    AP4_AtomFactory& atom_factory = AP4_DefaultAtomFactory::Instance;
    while (atom_factory.CreateAtomFromStream(*input, atom) == AP4_SUCCESS) {
        // remember the current stream position because the Inspect method
        // may read from the stream (there may be stream references in some
        // of the atoms
        AP4_Position position;
        input->Tell(position);

        // inspect the atom
        atom->Inspect(inspector);

        // restore the previous stream position
        input->Seek(position);

        // destroy the atom
        delete atom;
    }  

    // inspect the track data if needed
    if ((track_id != 0) && (track_data != NULL)) {
    	// rewind
    	input->Seek(0);
    	
    	// dump the track data
    	AP4_File* file = new AP4_File(*input);
        DumpTrackData(file, track_id, track_data);
        delete file;
    }

    if (input) input->Release();
    if (output) output->Release();
    if (track_data) track_data->Release();

    return 0;
}
