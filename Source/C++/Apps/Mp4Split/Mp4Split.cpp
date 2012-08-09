/*****************************************************************
|
|    AP4 - MP4 fragmented file splitter
|
|    Copyright 2002-2012 Axiomatic Systems, LLC
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
               "(c) 2002-2012 Axiomatic Systems, LLC"
 
#define AP4_SPLIT_DEFAULT_INIT_SEGMENT_NAME              "init.mp4"
#define AP4_SPLIT_DEFAULT_MEDIA_SEGMENT_NAME             "segment-%d.%04d.m4f"
#define AP4_SPLIT_DEFAULT_MEDIA_SEGMENT_NAME_NO_TRACK_ID "segment-%04d.m4f"

/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
struct Options {
    bool         verbose;
    const char*  input;
    const char*  init_segment_name;
    const char*  media_segment_name;
    bool         no_track_id;
    bool         audio_only;
    bool         video_only;
    unsigned int track_filter;
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
            "  --verbose\n"
            "  --init-segment <filename> (default: init.mp4)\n"
            "  --media-segment <filename-pattern> (default: segment-%%d.%%04d.m4f\n"
            "    or segment-%%04d.m4f if the --no-track-id option is used)\n"
            "  --no-track-id\n"
            "  --audio\n"
            "  --video\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   NextFragmentIndex
+---------------------------------------------------------------------*/
AP4_Array<unsigned int> TrackIds;
AP4_Array<unsigned int> TrackCounters;

static unsigned int
NextFragmentIndex(unsigned int track_id)
{
    int track_index = -1;
    for (unsigned int i=0; i<TrackIds.ItemCount(); i++) {
        if (TrackIds[i] == track_id) {
            track_index = i;
            break;
        }
    }
    if (track_index == -1) {
        track_index = 0;
        TrackIds.Append(track_id);
        TrackCounters.Append(0);
    }
    return TrackCounters[track_index]++;
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
    Options.verbose            = false;
    Options.input              = NULL;
    Options.init_segment_name  = AP4_SPLIT_DEFAULT_INIT_SEGMENT_NAME;
    Options.media_segment_name = NULL;
    Options.no_track_id        = false;
    Options.audio_only         = false;
    Options.video_only         = false;
    Options.track_filter       = 0;
    
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
        } else if (!strcmp(arg, "--media-segment")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: missing argument after --media-segment option\n");
                return 1;
            }
            Options.media_segment_name = *args++;
        } else if (!strcmp(arg, "--no-track-id")) {
            Options.no_track_id = true;
        } else if (!strcmp(arg, "--audio")) {
            Options.audio_only = true;
        } else if (!strcmp(arg, "--video")) {
            Options.video_only = true;
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
    if (Options.audio_only && Options.video_only) {
        fprintf(stderr, "ERROR: --audio and --video options are mutualy exclusive\n");
        return 1;
    }
    if (Options.media_segment_name == NULL) {
        if (Options.no_track_id) {
            Options.media_segment_name = AP4_SPLIT_DEFAULT_MEDIA_SEGMENT_NAME_NO_TRACK_ID;
        } else {
            Options.media_segment_name = AP4_SPLIT_DEFAULT_MEDIA_SEGMENT_NAME;
        }
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
    if (movie == NULL) {
        fprintf(stderr, "no movie found in file\n");
        return 1;
    }
    
    // filter tracks if required
    if (Options.audio_only) {
        AP4_Track* track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
        if (track == NULL) {
            fprintf(stderr, "--audio option specified, but no audio track found\n");
            return 1;
        }
        Options.track_filter = track->GetId();
    } else if (Options.video_only) {
        AP4_Track* track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
        if (track == NULL) {
            fprintf(stderr, "--video option specified, but no video track found\n");
            return 1;
        }
        Options.track_filter = track->GetId();
    }
    
    // save the init segment
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(Options.init_segment_name, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%d)\n", result);
        return 1;
    }
    AP4_FtypAtom* ftyp = file->GetFileType(); 
    if (ftyp) {
        result = ftyp->Write(*output);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot write init segment (%d)\n", result);
            return 1;
        }
    }
    result = movie->GetMoovAtom()->Write(*output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot write init segment (%d)\n", result);
        return 1;
    }
    
    AP4_Atom* atom = NULL;
    unsigned int track_id = 0;
    for (;;) {
        // process the next atom
        result = AP4_DefaultAtomFactory::Instance.CreateAtomFromStream(*input, atom);
        if (AP4_FAILED(result)) break;
        
        if (atom->GetType() == AP4_ATOM_TYPE_MOOF) {
            AP4_ContainerAtom* moof = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);

            unsigned int traf_count = 0;
            AP4_ContainerAtom* traf = NULL;
            do {
                traf = AP4_DYNAMIC_CAST(AP4_ContainerAtom, moof->GetChild(AP4_ATOM_TYPE_TRAF, traf_count));
                if (traf == NULL) break;
                AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
                if (tfhd == NULL) {
                    fprintf(stderr, "ERROR: invalid media format\n");
                    return 1;                    
                }
                track_id = tfhd->GetTrackId();
                traf_count++;
            } while (traf);
    
            // check if this fragment has more than one traf
            if (traf_count > 1) {
                if (Options.audio_only) {
                    fprintf(stderr, "ERROR: --audio option incompatible with multi-track fragments");
                    return 1;
                }
                if (Options.video_only) {
                    fprintf(stderr, "ERROR: --video option incompatible with multi-track fragments");
                    return 1;
                }
                track_id = 0;
            }
            
            // open a new file for this fragment
            if (output) {
                output->Release();
                output = NULL;
            }
            char segment_name[4096];
            if (Options.track_filter == 0 || Options.track_filter == track_id) {
                if (Options.no_track_id) {
                    sprintf(segment_name, Options.media_segment_name, NextFragmentIndex(track_id));
                } else {
                    sprintf(segment_name, Options.media_segment_name, track_id, NextFragmentIndex(track_id));
                }
                result = AP4_FileByteStream::Create(segment_name, AP4_FileByteStream::STREAM_MODE_WRITE, output);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: cannot open output file (%d)\n", result);
                    return 1;
                }
            }
        }
        
        // write the atom
        if (output && atom->GetType() != AP4_ATOM_TYPE_MFRA) {
            atom->Write(*output);
        }
        delete atom;
    }

    // cleanup
    delete file;
    if (input) input->Release();
    if (output) output->Release();
    
    return 0;
}

