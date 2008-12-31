/*****************************************************************
|
|    AP4 - MP4 Encrypter
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
#define BANNER "MP4 Encrypter - Version 1.2\n"\
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
        "\n\n"
        "usage: mp4encrypt --method <method> [options] <input> <output>\n"
        "  --method: <method> is OMA-PDCF-CBC, OMA-PDCF-CTR, MARLIN-IPMP or ISMA-IAEC\n"
        "  Options:\n"
        "  --show-progress: show progress details\n"
        "  --key <n>:<k>:<iv>\n"   
        "      Specifies the key to use for a track.\n"
        "      <n> is a track ID, <k> a 128-bit key in hex (32 characters)\n"
        "      and <iv> a 64-bit IV or salting key in hex (16 characters)\n"
        "      (several --key options can be used, one for each track)\n"
        "  --property <n>:<name>:<value>\n"
        "      Specifies a named string property for a track\n"
        "      <n> is a track ID, <name> a property name, and <value> is the\n"
        "      property value\n"
        "      (several --property options can be used, one or more for each track)\n"
        "  --kms-uri <uri>\n"
        "      Specifies the KMS URI for the ISMA-IAEC method\n"
        "\n"
        "  Method Specifics:\n"
        "    OMA-PDCF-CBC, OMA-PDCF-CTR, MARLIN-IPMP, ISMA-IAEC: the <iv> must be a 64-bit\n"
        "    hex string.\n"
        );
    exit(1);
}

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
enum Method {
    METHOD_NONE,
    METHOD_OMA_PDCF_CBC,
    METHOD_OMA_PDCF_CTR,
    METHOD_MARLIN_IPMP,
    METHOD_ISMA_AES
}; 

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
    if (argc == 1) PrintUsageAndExit();

    // parse options
    const char* kms_uri = NULL;
    enum Method method  = METHOD_NONE;
    const char* input_filename = NULL;
    const char* output_filename = NULL;
    AP4_ProtectionKeyMap key_map;
    AP4_TrackPropertyMap property_map;
    bool                 show_progress = false;

    // parse the command line arguments
    char* arg;
    while ((arg = *++argv)) {
        if (!strcmp(arg, "--method")) {
            arg = *++argv;
            if (!strcmp(arg, "OMA-PDCF-CBC")) {
                method = METHOD_OMA_PDCF_CBC;
            } else if (!strcmp(arg, "OMA-PDCF-CTR")) {
                method = METHOD_OMA_PDCF_CTR;
            } else if (!strcmp(arg, "MARLIN-IPMP")) {
                method = METHOD_MARLIN_IPMP;
            } else if (!strcmp(arg, "ISMA-IAEC")) {
                method = METHOD_ISMA_AES;
            } else {
                fprintf(stderr, "ERROR: invalid value for --method argument\n");
                return 1;
            }
        } else if (!strcmp(arg, "--kms-uri")) {
            if (method != METHOD_ISMA_AES) {
                fprintf(stderr, "ERROR: --kms-uri only applies to method ISMA-IAEC\n");
                return 1;
            }
            kms_uri = *++argv;
        } else if (!strcmp(arg, "--show-progress")) {
            show_progress = true;
        } else if (!strcmp(arg, "--key")) {
            if (method == METHOD_NONE) {
                fprintf(stderr, "ERROR: --method argument must appear before --key\n");
                return 1;
            }
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --key option\n");
                return 1;
            }
            char* track_ascii = NULL;
            char* key_ascii = NULL;
            char* iv_ascii = NULL;
            if (AP4_FAILED(AP4_SplitArgs(arg, track_ascii, key_ascii, iv_ascii))) {
                fprintf(stderr, "ERROR: invalid argument for --key option\n");
                return 1;
            }
            unsigned char key[16];
            unsigned char iv[16];
            unsigned int track = strtoul(track_ascii, NULL, 10);
            AP4_SetMemory(key, 0, sizeof(key));
            AP4_SetMemory(iv, 0, sizeof(iv));
            if (AP4_ParseHex(key_ascii, key, 16)) {
                fprintf(stderr, "ERROR: invalid hex format for key\n");
            }
            if (AP4_ParseHex(iv_ascii, iv, 8)) {
                fprintf(stderr, "ERROR: invalid hex format for iv\n");
                return 1;
            }
            // check that the key is not already there
            if (key_map.GetKey(track)) {
                fprintf(stderr, "ERROR: key already set for track %d\n", track);
                return 1;
            }
            // set the key in the map
            key_map.SetKey(track, key, iv);
        } else if (!strcmp(arg, "--property")) {
            char* track_ascii = NULL;
            char* name = NULL;
            char* value = NULL;
            if (method != METHOD_OMA_PDCF_CBC && 
                method != METHOD_OMA_PDCF_CTR &&
                method != METHOD_MARLIN_IPMP) {
                fprintf(stderr, "ERROR: this method does not use properties\n");
                return 1;
            }
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --property option\n");
                return 1;
            }
            if (AP4_FAILED(AP4_SplitArgs(arg, track_ascii, name, value))) {
                fprintf(stderr, "ERROR: invalid argument for --property option\n");
                return 1;
            }
            unsigned int track = strtoul(track_ascii, NULL, 10);

            // check that the property is not already set
            if (property_map.GetProperty(track, name)) {
                fprintf(stderr, "ERROR: property %s already set for track %d\n",
                                name, track);
                return 1;
            }
            // set the property in the map
            property_map.SetProperty(track, name, value);
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
    if (method == METHOD_NONE) {
        fprintf(stderr, "ERROR: missing --method argument\n");
        return 1;
    }
    if (input_filename == NULL) {
        fprintf(stderr, "ERROR: missing input filename\n");
        return 1;
    }
    if (output_filename == NULL) {
        fprintf(stderr, "ERROR: missing output filename\n");
        return 1;
    }

    // create an encrypting processor
    AP4_Processor* processor;
    if (method == METHOD_ISMA_AES) {
        if (kms_uri == NULL) {
            fprintf(stderr, "ERROR: method ISMA-IAEC requires --kms-uri\n");
            return 1;
        }
        AP4_IsmaEncryptingProcessor* isma_processor = new AP4_IsmaEncryptingProcessor(kms_uri);
        isma_processor->GetKeyMap().SetKeys(key_map);
        processor = isma_processor;
    } else if (method == METHOD_MARLIN_IPMP) {
        AP4_MarlinIpmpEncryptingProcessor* marlin_processor = 
            new AP4_MarlinIpmpEncryptingProcessor();
        marlin_processor->GetKeyMap().SetKeys(key_map);
        marlin_processor->GetPropertyMap().SetProperties(property_map);
        processor = marlin_processor;
    } else {
        AP4_OmaDcfEncryptingProcessor* oma_processor = 
            new AP4_OmaDcfEncryptingProcessor(method == METHOD_OMA_PDCF_CTR?
                                              AP4_OMA_DCF_CIPHER_MODE_CTR :
                                              AP4_OMA_DCF_CIPHER_MODE_CBC);
        oma_processor->GetKeyMap().SetKeys(key_map);
        oma_processor->GetPropertyMap().SetProperties(property_map);
        processor = oma_processor;
    }

    // create the input stream
    AP4_ByteStream* input;
    try{
        input = new AP4_FileByteStream(input_filename,
            AP4_FileByteStream::STREAM_MODE_READ);
    } catch (AP4_Exception) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }

    // create the output stream
    AP4_ByteStream* output;
    try {
        output = new AP4_FileByteStream(output_filename,
            AP4_FileByteStream::STREAM_MODE_WRITE);
    } catch (AP4_Exception) {
        fprintf(stderr, "ERROR: cannot open output file (%s)\n", output_filename);
        return 1;
    }

    // process/decrypt the file
    ProgressListener listener;
    AP4_Result result = processor->Process(*input, *output, show_progress?&listener:NULL);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to process the file (%d)\n", result);
    }

    // cleanup
    delete processor;
    input->Release();
    output->Release();

    return 0;
}
