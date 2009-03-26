/*****************************************************************
|
|    AP4 - MP4 Rtp Hint Info
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
#define BANNER "MP4 Rtp Hint Info - Version 1.0\n"\
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
        "\n\nusage: mp4rtphintinfo [--trackid <hinttrackid>] <input>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   DumpRtpPackets
+---------------------------------------------------------------------*/
static AP4_Result
DumpRtpPackets(AP4_HintTrackReader& reader, const char* file_name)
{
    // create the output stream
    AP4_Result result;
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(file_name, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%s)\n", file_name);
        return AP4_FAILURE;
    }

    // read the packets from the reader and write them in the output stream
    AP4_DataBuffer data(1500);
    AP4_UI32 ts_ms;
    while(AP4_SUCCEEDED(reader.GetNextPacket(data, ts_ms))) {
        output->Write(data.GetData(), data.GetDataSize());
        AP4_Debug("#########\n\tpacket contains %d bytes\n", data.GetDataSize());
        AP4_Debug("\tsent at time stamp %d\n\n", ts_ms);
    }
    
    output->Release();

    return AP4_SUCCESS;
}


/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    AP4_Result result = AP4_SUCCESS;

    // parse the command line
    if (argc != 4) PrintUsageAndExit();

    // parse options
    const char* input_filename = NULL;
    AP4_UI32 hint_track_id = 0;

    char* arg;
    while ((arg = *++argv)) {
        if (!strcmp(arg, "--trackid")) {
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --trackid option\n");
                PrintUsageAndExit();
            }
            hint_track_id = atoi(arg);
        } else if (input_filename == NULL) {
            input_filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument (%s)\n", arg);
            return 1;
        }
    }
    // create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", argv[1]);
        PrintUsageAndExit();
    }
    
    AP4_File file(*input);
    AP4_Movie* movie = file.GetMovie();
    if (movie != NULL) {
        // get a hint track reader
        AP4_Track* hint_track = movie->GetTrack(hint_track_id);
        if (hint_track == NULL) {
            fprintf(stderr, "ERROR: No track for track id (%d)\n", hint_track_id);
            return AP4_FAILURE;
        }
        if (hint_track->GetType() != AP4_Track::TYPE_HINT) {
            fprintf(stderr, "track of id (%d) is not of type 'hint'\n", hint_track_id);
            return AP4_FAILURE;
        }        	
        AP4_HintTrackReader* reader = NULL;
        result = AP4_HintTrackReader::Create(*hint_track, *movie, 0x01020304, reader);
        if (AP4_FAILED(result)) return result;
        
        char rtp_file_name[256];
        AP4_FormatString(rtp_file_name, 256, "%s-track-%d.rtp", input_filename, hint_track_id);

        // display the sdp
        AP4_String sdp;
        reader->GetSdpText(sdp);
        AP4_Debug("sdp:\n%s\n\n", sdp.GetChars());

        // dump the packet
        result = DumpRtpPackets(*reader, rtp_file_name);
        
        delete reader;
    } else {
        AP4_Debug("No movie found in the file\n");
        return AP4_FAILURE;
    }

    input->Release();

    return result;
}
