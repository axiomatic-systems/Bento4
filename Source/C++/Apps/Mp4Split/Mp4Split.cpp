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
#define BANNER "MP4 Fragment Splitter - Version 1.2\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2016 Axiomatic Systems, LLC"
 
#define AP4_SPLIT_DEFAULT_INIT_SEGMENT_NAME  "init.mp4"
#define AP4_SPLIT_DEFAULT_MEDIA_SEGMENT_NAME "segment-%llu.%04llu.m4s"
#define AP4_SPLIT_DEFAULT_PATTERN_PARAMS     "IN"

const unsigned int AP4_SPLIT_MAX_TRACK_IDS = 32;

/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
struct Options {
    bool         verbose;
    const char*  input;
    const char*  init_segment_name;
    const char*  media_segment_name;
    const char*  pattern_params;
    unsigned int start_number;
    unsigned int track_ids[AP4_SPLIT_MAX_TRACK_IDS];
    unsigned int track_id_count;
    bool         audio_only;
    bool         video_only;
    bool         init_only;
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
            "  --verbose : print verbose information when running\n"
            "  --init-segment <filename> : name of init segment (default: init.mp4)\n"
            "  --init-only : only output the init segment (no media segments)\n"
            "  --media-segment <filename-pattern> (default: segment-%%llu.%%04llu.m4s)\n"
            "    NOTE: all parameters are 64-bit integers, use %%llu in the pattern\n"
            "  --start-number <n> : start numbering segments at <n> (default=1)\n"
            "  --pattern-parameters <params> : one or more selector letter (default: IN)\n"
            "     I: track ID\n"
            "     N: segment number\n"
            "  --track-id <track-id> : only output segments with this track ID\n"
            "     More than one track IDs can be specified if <track-id> is a comma-separated\n"
            "     list of track IDs\n"
            "  --audio : only output audio segments\n"
            "  --video : only output video segments\n");
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
        track_index = TrackCounters.ItemCount();
        TrackIds.Append(track_id);
        TrackCounters.Append(0);
    }
    return TrackCounters[track_index]++;
}

/*----------------------------------------------------------------------
|   ParseTrackIds
+---------------------------------------------------------------------*/
static bool
ParseTrackIds(char* ids)
{
    for (char* separator = ids; ; ++separator) {
        if (*separator == ',' || *separator == 0) {
            if (Options.track_id_count >= AP4_SPLIT_MAX_TRACK_IDS) {
                return false;
            }
            bool the_end = (*separator == 0);
            *separator = 0;
            Options.track_ids[Options.track_id_count++] = (unsigned int)strtoul(ids, NULL, 10);
            if (the_end) break;
            ids = separator+1;
        }
    }
    
    return true;
}

/*----------------------------------------------------------------------
|   TrackIdMatches
+---------------------------------------------------------------------*/
static bool
TrackIdMatches(unsigned int track_id)
{
    if (Options.track_id_count == 0) return true;
    for (unsigned int i=0; i<Options.track_id_count; i++) {
        if (Options.track_ids[i] == track_id) return true;
    }
    
    return false;
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
    Options.verbose                = false;
    Options.input                  = NULL;
    Options.init_segment_name      = AP4_SPLIT_DEFAULT_INIT_SEGMENT_NAME;
    Options.media_segment_name     = AP4_SPLIT_DEFAULT_MEDIA_SEGMENT_NAME;
    Options.pattern_params         = AP4_SPLIT_DEFAULT_PATTERN_PARAMS;
    Options.start_number           = 1;
    Options.track_id_count         = 0;
    Options.audio_only             = false;
    Options.video_only             = false;
    Options.init_only              = false;
    
    // parse command line
    AP4_Result result;
    char** args = argv+1;
    while (char* arg = *args++) {
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
        } else if (!strcmp(arg, "--pattern-parameters")) {
            if (*args == NULL) {
                fprintf(stderr, "ERROR: missing argument after --pattern-params option\n");
                return 1;
            }
            Options.pattern_params = *args++;
        } else if (!strcmp(arg, "--track-id")) {
            if (!ParseTrackIds(*args++)) {
                fprintf(stderr, "ERROR: invalid argument for --track-id\n");
                return 1;
            }
        } else if (!strcmp(arg, "--start-number")) {
            Options.start_number = (unsigned int)strtoul(*args++, NULL, 10);
        } else if (!strcmp(arg, "--init-only")) {
            Options.init_only = true;
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
    if ((Options.audio_only     && (Options.video_only || Options.track_id_count)) ||
        (Options.video_only     && (Options.audio_only || Options.track_id_count)) ||
        (Options.track_id_count && (Options.audio_only || Options.video_only    ))) {
        fprintf(stderr, "ERROR: --audio, --video and --track-id options are mutualy exclusive\n");
        return 1;
    }
    if (strlen(Options.pattern_params) < 1) {
        fprintf(stderr, "ERROR: --pattern-params argument is too short\n");
        return 1;
    }
    if (strlen(Options.pattern_params) > 2) {
        fprintf(stderr, "ERROR: --pattern-params argument is too long\n");
        return 1;
    }
    const char* cursor = Options.pattern_params;
    while (*cursor) {
        if (*cursor != 'I' && *cursor != 'N') {
            fprintf(stderr, "ERROR: invalid pattern parameter '%c'\n", *cursor);
            return 1;
        }
        ++cursor;
    }
    
	// create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(Options.input, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input (%d)\n", result);
        return 1;
    }
    
    // get the movie
    AP4_File* file = new AP4_File(*input, true);
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
        Options.track_ids[0]   = track->GetId();
        Options.track_id_count = 1;
    } else if (Options.video_only) {
        AP4_Track* track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
        if (track == NULL) {
            fprintf(stderr, "--video option specified, but no video track found\n");
            return 1;
        }
        Options.track_ids[0]   = track->GetId();
        Options.track_id_count = 1;
    } else if (Options.track_id_count) {
        for (unsigned int i=0; i<Options.track_id_count; i++) {
            AP4_Track* track = movie->GetTrack(Options.track_ids[i]);
            if (track == NULL) {
                fprintf(stderr, "--track-id option specified, but no such track found\n");
                return 1;
            }
        }
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
            fprintf(stderr, "ERROR: cannot write ftyp segment (%d)\n", result);
            return 1;
        }
    }
    if (Options.track_id_count) {
        AP4_MoovAtom* moov = movie->GetMoovAtom();
        
        // only keep the 'trak' atom that we need
        AP4_List<AP4_Atom>::Item* child = moov->GetChildren().FirstItem();
        while (child) {
            AP4_Atom* atom = child->GetData();
            child = child->GetNext();
            if (atom->GetType() == AP4_ATOM_TYPE_TRAK) {
                AP4_TrakAtom* trak = (AP4_TrakAtom*)atom;
                AP4_TkhdAtom* tkhd = (AP4_TkhdAtom*)trak->GetChild(AP4_ATOM_TYPE_TKHD);
                if (tkhd && !TrackIdMatches(tkhd->GetTrackId())) {
                    atom->Detach();
                    delete atom;
                }
            }
        }

        // only keep the 'trex' atom that we need
        AP4_ContainerAtom* mvex = AP4_DYNAMIC_CAST(AP4_ContainerAtom, moov->GetChild(AP4_ATOM_TYPE_MVEX));
        if (mvex) {
            child = mvex->GetChildren().FirstItem();
            while (child) {
                AP4_Atom* atom = child->GetData();
                child = child->GetNext();
                if (atom->GetType() == AP4_ATOM_TYPE_TREX) {
                    AP4_TrexAtom* trex = AP4_DYNAMIC_CAST(AP4_TrexAtom, atom);
                    if (trex && !TrackIdMatches(trex->GetTrackId())) {
                        atom->Detach();
                        delete atom;
                    }
                }
            }
        }
    }
    result = movie->GetMoovAtom()->Write(*output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot write init segment (%d)\n", result);
        return 1;
    }
        
    AP4_Atom* atom = NULL;
    unsigned int track_id = 0;
    AP4_DefaultAtomFactory atom_factory;
    for (;!Options.init_only;) {
        // process the next atom
        result = atom_factory.CreateAtomFromStream(*input, atom);
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
            
            // open a new file for this fragment if this moof is a segment start
            char segment_name[4096];
            if (Options.track_id_count == 0 || track_id == Options.track_ids[0]) {
                if (output) {
                    output->Release();
                    output = NULL;
                }

                AP4_UI64 p[2] = {0,0};
                unsigned int params_len = (unsigned int)strlen(Options.pattern_params);
                for (unsigned int i=0; i<params_len; i++) {
                    if (Options.pattern_params[i] == 'I') {
                        p[i] = track_id;
                    } else if (Options.pattern_params[i] == 'N') {
                        p[i] = NextFragmentIndex(track_id)+Options.start_number;
                    }
                }
                switch (params_len) {
                    case 1:
                        sprintf(segment_name, Options.media_segment_name, p[0]);
                        break;
                    case 2:
                        sprintf(segment_name, Options.media_segment_name, p[0], p[1]);
                        break;
                    default:
                        segment_name[0] = 0;
                        break;
                }
                result = AP4_FileByteStream::Create(segment_name, AP4_FileByteStream::STREAM_MODE_WRITE, output);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: cannot open output file (%d)\n", result);
                    return 1;
                }
            }
        }
        
        // write the atom
        if (output && atom->GetType() != AP4_ATOM_TYPE_MFRA && TrackIdMatches(track_id)) {
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

