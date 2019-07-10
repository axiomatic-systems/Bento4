/*****************************************************************
|
|    AP4 - MP4 Decrypter
|
|    Copyright 2002-2014 Axiomatic Systems, LLC
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
#define BANNER "MP4 Decrypter - Version 1.4\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2015 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\n"
            "usage: mp4decrypt [options] <input> <output>\n"
            "Options are:\n"
            "  --show-progress : show progress details\n"
            "  --key <id>:<k>\n"
            "      <id> is either a track ID in decimal or a 128-bit KID in hex,\n"
            "      <k> is a 128-bit key in hex\n"
            "      (several --key options can be used, one for each track or KID)\n"
            "      note: for dcf files, use 1 as the track index\n"
            "      note: for Marlin IPMP/ACGK, use 0 as the track ID\n"
            "      note: KIDs are only applicable to some encryption methods like MPEG-CENC\n"
            "  --fragments-info <filename>\n"
            "      Decrypt the fragments read from <input>, with track info read\n"
            "      from <filename>.\n"
            );
    exit(1);
}

/*----------------------------------------------------------------------
|   ProgressListener
+---------------------------------------------------------------------*/
class ProgressListener : public AP4_Processor::ProgressListener
{
public:
    AP4_Result OnProgress(unsigned int step, unsigned int total);
};

AP4_Result
ProgressListener::OnProgress(unsigned int step, unsigned int total)
{
    printf("\r%d/%d", step, total);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc == 1) {
        PrintUsageAndExit();
    }

    // create a key map object to hold keys
    AP4_ProtectionKeyMap key_map;
    
    // parse options
    const char* input_filename = NULL;
    const char* output_filename = NULL;
    const char* fragments_info_filename = NULL;
    bool        show_progress = false;

    char* arg;
    while ((arg = *++argv)) {
        if (!strcmp(arg, "--key")) {
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument after --key option\n");
                return 1;
            }
            char* keyid_text = NULL;
            char* key_text = NULL;
            if (AP4_SplitArgs(arg, keyid_text, key_text)) {
                fprintf(stderr, "ERROR: invalid argument for --key option\n");
                return 1;
            }
            unsigned char key[16];
            unsigned int  track_id = 0;
            unsigned char kid[16];
            if (strlen(keyid_text) == 32) {
                if (AP4_ParseHex(keyid_text, kid, 16)) {
                    fprintf(stderr, "ERROR: invalid hex format for key id\n");
                    return 1;
                }
            } else {
                track_id = (unsigned int)strtoul(keyid_text, NULL, 10);
                if (track_id == 0) {
                    fprintf(stderr, "ERROR: invalid key id\n");
                    return 1;
                }
            }
            if (AP4_ParseHex(key_text, key, 16)) {
                fprintf(stderr, "ERROR: invalid hex format for key\n");
                return 1;
            }
            // set the key in the map
            if (track_id) {
                key_map.SetKey(track_id, key, 16);
            } else {
                key_map.SetKeyForKid(kid, key, 16);
            }
        } else if (!strcmp(arg, "--fragments-info")) {
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --fragments-info option\n");
                return 1;
            }
            fragments_info_filename = arg;
        } else if (!strcmp(arg, "--show-progress")) {
            show_progress = true;
        } else if (input_filename == NULL) {
            input_filename = arg;
        } else if (output_filename == NULL) {
            output_filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument (%s)\n", arg);
            return 1;
        }
    }

    // check the arguments
    if (input_filename == NULL) {
        fprintf(stderr, "ERROR: missing input filename\n");
        return 1;
    }
    if (output_filename == NULL) {
        fprintf(stderr, "ERROR: missing output filename\n");
        return 1;
    }

    // create the input stream
    AP4_Result result;
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s) %d\n", input_filename, result);
        return 1;
    }

    // create the output stream
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%s) %d\n", output_filename, result);
        return 1;
    }

    // create the fragments stream if needed
    AP4_ByteStream* fragments_info = NULL;
    if (fragments_info_filename) {
        result = AP4_FileByteStream::Create(fragments_info_filename, AP4_FileByteStream::STREAM_MODE_READ, fragments_info);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot open fragments info file (%s)\n", fragments_info_filename);
            return 1;
        }
    }

    // create the decrypting processor
    AP4_Processor* processor = NULL;
    AP4_File* input_file = new AP4_File(fragments_info?*fragments_info:*input);
    AP4_FtypAtom* ftyp = input_file->GetFileType();
    if (ftyp) {
        if (ftyp->GetMajorBrand() == AP4_OMA_DCF_BRAND_ODCF || ftyp->HasCompatibleBrand(AP4_OMA_DCF_BRAND_ODCF)) {
            processor = new AP4_OmaDcfDecryptingProcessor(&key_map);
        } else if (ftyp->GetMajorBrand() == AP4_MARLIN_BRAND_MGSV || ftyp->HasCompatibleBrand(AP4_MARLIN_BRAND_MGSV)) {
            processor = new AP4_MarlinIpmpDecryptingProcessor(&key_map);
        } else if (ftyp->GetMajorBrand() == AP4_PIFF_BRAND || ftyp->HasCompatibleBrand(AP4_PIFF_BRAND)) {
            processor = new AP4_CencDecryptingProcessor(&key_map);
        }
    }
    if (processor == NULL) {
        // no ftyp, look at the sample description of the tracks first
        AP4_Movie* movie = input_file->GetMovie();
        if (movie) {
            AP4_List<AP4_Track>& tracks = movie->GetTracks();
            for (unsigned int i=0; i<tracks.ItemCount(); i++) {
                AP4_Track* track = NULL;
                tracks.Get(i, track);
                if (track) {
                    AP4_SampleDescription* sdesc = track->GetSampleDescription(0);
                    if (sdesc && sdesc->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
                        AP4_ProtectedSampleDescription* psdesc = AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, sdesc);
                        if (psdesc) {
                            if (psdesc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CENC ||
                                psdesc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CBC1 ||
                                psdesc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CENS ||
                                psdesc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CBCS) {
                                processor = new AP4_CencDecryptingProcessor(&key_map);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
        
    // by default, try a standard decrypting processor
    if (processor == NULL) {
        processor = new AP4_StandardDecryptingProcessor(&key_map);
    }
    
    delete input_file;
    input_file = NULL;
    if (fragments_info) {
        fragments_info->Seek(0);
    } else {
        input->Seek(0);
    }
    
    // process/decrypt the file
    ProgressListener listener;
    if (fragments_info) {
        result = processor->Process(*input, *output, *fragments_info, show_progress?&listener:NULL);
    } else {
        result = processor->Process(*input, *output, show_progress?&listener:NULL);
    }
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to process the file (%d)\n", result);
    }

    // cleanup
    delete processor;
    input->Release();
    output->Release();

    return 0;
}
