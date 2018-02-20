/*****************************************************************
|
|    AP4 - MP4 Encrypter
|
|    Copyright 2002-2009 Axiomatic Systems, LLC
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
#define BANNER "MP4 Encrypter - Version 1.6\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2016 Axiomatic Systems, LLC"

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
        "     <method> is OMA-PDCF-CBC, OMA-PDCF-CTR, MARLIN-IPMP-ACBC,\n"
        "     MARLIN-IPMP-ACGK, ISMA-IAEC, PIFF-CBC, PIFF-CTR, ,MPEG-CENC,\n"
        "     MPEG-CBC1, MPEG-CENS, MPEG-CBCS\n"
        "  Options:\n"
        "  --show-progress\n"
        "      Show progress details\n"
        "  --fragments-info <filename>\n"
        "      Encrypt the fragments read from <input>, with track info read\n"
        "      from <filename>\n"
        "  --key <n>:<k>:<iv>\n"   
        "      Specifies the key to use for a track (or group key).\n"
        "      <n> is a track ID, <k> a 128-bit key in hex (32 characters)\n"
        "      and <iv> a 64-bit or 128-bit IV or salting key in hex\n"
        "      (16 or 32 characters) depending on the cipher mode\n"
        "      The key and IV values can also be specified with the keyword 'random'\n"
        "      instead of a hex-encoded value, in which case a randomly generated value\n"
        "      will be used.\n"
        "      (several --key options can be used, one for each track)\n"
        "  --strict\n"
        "      Fail if there is a warning (ex: one or more tracks would be left unencrypted)\n"
        "  --property <n>:<name>:<value>\n"
        "      Specifies a named string property for a track\n"
        "      <n> is a track ID, <name> a property name, and <value> is the\n"
        "      property value\n"
        "      (several --property options can be used, one or more for each track)\n"
        "  --global-option <name>:<value>\n"
        "      Sets the global option <name> to be equal to <value>\n"
        "  --pssh <system-id>:<filename>\n"
        "      Add a 'pssh' atom for this system ID, with the payload\n"
        "      loaded from <filename>.\n"
        "      (several --pssh options can be used, with a different system ID for each)\n"
        "      (the filename can be left empty after the ':' if no payload is needed)\n"
        "  --pssh-v1 <system-id>:<filename>\n"
        "      Same as --pssh but generates a version=1 'pssh' atom\n"
        "      (this option must appear *after* the --property options on the command line)\n"
        "  --kms-uri <uri>\n"
        "      Specifies the KMS URI for the ISMA-IAEC method\n"
        "\n"
        "  Method Specifics:\n"
        "    OMA-PDCF-CBC, MARLIN-IPMP-ACBC, MARLIN-IPMP-ACGK, PIFF-CBC, MPEG-CBC1, MPEG-CBCS: \n"
        "    the <iv> can be 64-bit or 128-bit\n"
        "    If the IV is specified as a 64-bit value, it will be padded with zeros.\n"
        "\n"
        "    OMA-PDCF-CTR, ISMA-IAEC, PIFF-CTR, MPEG-CENC, MPEG-CENS:\n"
        "    the <iv> should be a 64-bit hex string.\n"
        "    If a 128-bit value is supplied, it will be truncated to 64-bit.\n"
        "\n"
        "    OMA-PDCF-CBC, OMA-PDCF-CTR: The following properties are defined,\n"
        "    and all other properties are stored in the textual headers:\n"
        "      ContentId       -> the content ID for the track\n"
        "      RightsIssuerUrl -> the Rights Issuer URL\n"
        "\n"
        "    MARLIN-IPMP-ACBC, MARLIN-IPMP-ACGK: The following properties are defined:\n"
        "      ContentId -> the content ID for the track\n"
        "\n"
        "    MARLIN-IPMP-ACGK: The group key is specified with --key where <n>\n"
        "    is 0. The <iv> part of the key must be present, but will be ignored;\n"
        "    It should therefore be set to 0000000000000000\n"
        "\n"
        "    MPEG-CENC, MPEG-CBC1, MPEG-CENS, MPEG-CBCS, PIFF-CTR, PIFF-CBC:\n"
        "    The following properties are defined:\n"
        "      KID -> the value of KID, 16 bytes, in hexadecimal (32 characters)\n"
        "      ContentId -> Content ID mapping for KID (Marlin option)\n"
        "      PsshPadding -> pad the 'pssh' container to this size\n"
        "                    (only when using ContentId).\n"
        "                    This property should be set for track ID 0 only\n"
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
    METHOD_MARLIN_IPMP_ACBC,
    METHOD_MARLIN_IPMP_ACGK,
    METHOD_PIFF_CBC,
    METHOD_PIFF_CTR,
    METHOD_MPEG_CENC,
    METHOD_MPEG_CBC1,
    METHOD_MPEG_CENS,
    METHOD_MPEG_CBCS,
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
|   CheckWarning
+---------------------------------------------------------------------*/
static bool
CheckWarning(AP4_ByteStream& stream, AP4_ProtectionKeyMap& key_map, Method method)
{
    AP4_File file(stream, true);
    AP4_Movie* movie = file.GetMovie();
    if (!movie) {
        if (method != METHOD_MPEG_CENC &&
            method != METHOD_MPEG_CBC1 &&
            method != METHOD_MPEG_CENS &&
            method != METHOD_MPEG_CBCS &&
            method != METHOD_PIFF_CBC  &&
            method != METHOD_PIFF_CTR) {
            fprintf(stderr, "WARNING: no movie atom found in input file\n");
            return false;
        }
    }

    bool warning = false;
    switch (method) {
        case METHOD_MPEG_CENC:
        case METHOD_MPEG_CBC1:
        case METHOD_MPEG_CENS:
        case METHOD_MPEG_CBCS:
        case METHOD_PIFF_CBC:
        case METHOD_PIFF_CTR:
            if (movie && !movie->HasFragments()) {
                fprintf(stderr, "WARNING: this encryption method only applies to fragmented files\n");
                warning = true;
            }
            break;
        default:
            break;
    }

    if (movie) {
        for (unsigned int i=0; i<movie->GetTracks().ItemCount(); i++) {
            AP4_Track* track;
            AP4_Result result = movie->GetTracks().Get(i, track);
            if (AP4_FAILED(result)) return false;
            const AP4_DataBuffer* key = key_map.GetKey(track->GetId());
            if (key == NULL) {
                fprintf(stderr, "WARNING: track ID %d will not be encrypted\n", track->GetId());
                warning = true;
            }
        }
    }
    
    stream.Seek(0);
    return warning;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc == 1) PrintUsageAndExit();

    // parse options
    const char*              kms_uri = NULL;
    enum Method              method  = METHOD_NONE;
    const char*              input_filename = NULL;
    const char*              output_filename = NULL;
    const char*              fragments_info_filename = NULL;
    AP4_ProtectionKeyMap     key_map;
    AP4_TrackPropertyMap     property_map;
    bool                     show_progress = false;
    bool                     strict = false;
    AP4_Array<AP4_PsshAtom*> pssh_atoms;
    AP4_DataBuffer           kids;
    unsigned int             kid_count = 0;
    AP4_Result               result;
    
    // parse the command line arguments
    char* arg;
    while ((arg = *++argv)) {
        if (!strcmp(arg, "--method")) {
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --method option\n");
                return 1;
            }
            if (!strcmp(arg, "OMA-PDCF-CBC")) {
                method = METHOD_OMA_PDCF_CBC;
            } else if (!strcmp(arg, "OMA-PDCF-CTR")) {
                method = METHOD_OMA_PDCF_CTR;
            } else if (!strcmp(arg, "MARLIN-IPMP-ACBC")) {
                method = METHOD_MARLIN_IPMP_ACBC;
            } else if (!strcmp(arg, "MARLIN-IPMP-ACGK")) {
                method = METHOD_MARLIN_IPMP_ACGK;
            } else if (!strcmp(arg, "PIFF-CBC")) {
                method = METHOD_PIFF_CBC;
            } else if (!strcmp(arg, "PIFF-CTR")) {
                method = METHOD_PIFF_CTR;
            } else if (!strcmp(arg, "MPEG-CENC")) {
                method = METHOD_MPEG_CENC;
            } else if (!strcmp(arg, "MPEG-CBC1")) {
                method = METHOD_MPEG_CBC1;
            } else if (!strcmp(arg, "MPEG-CENS")) {
                method = METHOD_MPEG_CENS;
            } else if (!strcmp(arg, "MPEG-CBCS")) {
                method = METHOD_MPEG_CBCS;
            } else if (!strcmp(arg, "ISMA-IAEC")) {
                method = METHOD_ISMA_AES;
            } else {
                fprintf(stderr, "ERROR: invalid value for --method argument\n");
                return 1;
            }
        } else if (!strcmp(arg, "--fragments-info")) {
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --fragments-info option\n");
                return 1;
            }
            fragments_info_filename = arg;
        } else if (!strcmp(arg, "--pssh") || !strcmp(arg, "--pssh-v1")) {
            bool v1 = (strcmp(arg, "--pssh-v1") == 0);
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --pssh\n");
                return 1;
            }
            if (AP4_StringLength(arg) < 32+1 || arg[32] != ':') {
                fprintf(stderr, "ERROR: invalid argument syntax for --pssh\n");
                return 1;
            }
            unsigned char system_id[16];
            arg[32] = '\0';
            result = AP4_ParseHex(arg, system_id, 16);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: invalid argument syntax for --pssh\n");
                return 1;
            }
            const char* pssh_filename = arg+33;
            
            // load the pssh payload
            AP4_DataBuffer pssh_payload;
            if (pssh_filename[0]) {
                AP4_ByteStream* pssh_input = NULL;
                result = AP4_FileByteStream::Create(pssh_filename, AP4_FileByteStream::STREAM_MODE_READ, pssh_input);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: cannot open pssh payload file (%d)\n", result);
                    return 1;
                }
                AP4_LargeSize pssh_payload_size = 0;
                pssh_input->GetSize(pssh_payload_size);
                pssh_payload.SetDataSize((AP4_Size)pssh_payload_size);
                result = pssh_input->Read(pssh_payload.UseData(), (AP4_Size)pssh_payload_size);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: cannot read pssh payload from file (%d)\n", result);
                    return 1;
                }
            }
            AP4_PsshAtom* pssh;
            if (v1) {
                if (kid_count) {
                    pssh = new AP4_PsshAtom(system_id, kids.GetData(), kid_count);
                } else {
                    pssh = new AP4_PsshAtom(system_id);
                }
            } else {
                pssh = new AP4_PsshAtom(system_id);
            }
            if (pssh_payload.GetDataSize()) {
                pssh->SetData(pssh_payload.GetData(), pssh_payload.GetDataSize());
            }
            pssh_atoms.Append(pssh);
        } else if (!strcmp(arg, "--kms-uri")) {
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --kms-uri option\n");
                return 1;
            }
            if (method != METHOD_ISMA_AES) {
                fprintf(stderr, "ERROR: --kms-uri only applies to method ISMA-IAEC\n");
                return 1;
            }
            kms_uri = arg;
        } else if (!strcmp(arg, "--show-progress")) {
            show_progress = true;
        } else if (!strcmp(arg, "--strict")) {
            strict = true;
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
            unsigned int track = (unsigned int)strtoul(track_ascii, NULL, 10);

            
            // parse the key value
            unsigned char key[16];
            AP4_SetMemory(key, 0, sizeof(key));
            if (AP4_CompareStrings(key_ascii, "random") == 0) {
                result = AP4_System_GenerateRandomBytes(key, 16);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: failed to generate random key (%d)\n", result);
                    return 1;
                }
                char key_hex[32+1];
                key_hex[32] = '\0';
                AP4_FormatHex(key, 16, key_hex);
                printf("KEY.%d=%s\n", track, key_hex);
            } else {
                if (AP4_ParseHex(key_ascii, key, 16)) {
                    fprintf(stderr, "ERROR: invalid hex format for key\n");
                    return 1;
                }
            }
            
            // parse the iv
            unsigned char iv[16];
            AP4_SetMemory(iv, 0, sizeof(iv));
            if (AP4_CompareStrings(iv_ascii, "random") == 0) {
                result = AP4_System_GenerateRandomBytes(iv, 16);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: failed to generate random key (%d)\n", result);
                    return 1;
                }
                iv[0] &= 0x7F; // always set the MSB to 0 so we don't have wraparounds
            } else {
                unsigned int iv_size = (unsigned int)AP4_StringLength(iv_ascii)/2;
                if (AP4_ParseHex(iv_ascii, iv, iv_size)) {
                    fprintf(stderr, "ERROR: invalid hex format for iv\n");
                    return 1;
                }
            }
            switch (method) {
                case METHOD_OMA_PDCF_CTR:
                case METHOD_ISMA_AES:
                case METHOD_PIFF_CTR:
                case METHOD_MPEG_CENC:
                case METHOD_MPEG_CENS:
                    // truncate the IV
                    AP4_SetMemory(&iv[8], 0, 8);
                    break;
                    
                default:
                    break;
            }
            
            // check that the key is not already there
            if (key_map.GetKey(track)) {
                fprintf(stderr, "ERROR: key already set for track %d\n", track);
                return 1;
            }
            
            // set the key in the map
            key_map.SetKey(track, key, 16, iv, 16);
        } else if (!strcmp(arg, "--property")) {
            char* track_ascii = NULL;
            char* name = NULL;
            char* value = NULL;
            if (method != METHOD_OMA_PDCF_CBC     && 
                method != METHOD_OMA_PDCF_CTR     &&
                method != METHOD_MARLIN_IPMP_ACBC &&
                method != METHOD_MARLIN_IPMP_ACGK &&
                method != METHOD_PIFF_CBC         &&
                method != METHOD_PIFF_CTR         &&
                method != METHOD_MPEG_CENC        &&
                method != METHOD_MPEG_CBC1        &&
                method != METHOD_MPEG_CENS        &&
                method != METHOD_MPEG_CBCS) {
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
            unsigned int track = (unsigned int)strtoul(track_ascii, NULL, 10);

            // check that the property is not already set
            if (property_map.GetProperty(track, name)) {
                fprintf(stderr, "ERROR: property %s already set for track %d\n",
                                name, track);
                return 1;
            }
            // set the property in the map
            property_map.SetProperty(track, name, value);
            
            // special treatment for KID properties
            if (!strcmp(name, "KID")) {
                if (AP4_StringLength(value) != 32) {
                    fprintf(stderr, "ERROR: invalid size for KID property (must be 32 hex chars)\n");
                    return 1;
                }
                AP4_UI08 kid[16];
                if (AP4_FAILED(AP4_ParseHex(value, kid, 16))) {
                    fprintf(stderr, "ERROR: invalid syntax for KID property (must be 32 hex chars)\n");
                    return 1;
                }
                
                // check if we already have this KID
                bool kid_already_present = false;
                for (unsigned int i=0; i<kid_count; i++) {
                    if (AP4_CompareMemory(kids.GetData()+(i*16), kid, 16) == 0) {
                        kid_already_present = true;
                        break;
                    }
                }
                if (!kid_already_present) {
                    ++kid_count;
                    kids.AppendData(kid, 16);
                }
            }
        } else if (!strcmp(arg, "--global-option")) {
            arg = *++argv;
            char* name = NULL;
            char* value = NULL;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --global-option option\n");
                return 1;
            }
            if (AP4_FAILED(AP4_SplitArgs(arg, name, value))) {
                fprintf(stderr, "ERROR: invalid argument for --global-option option\n");
                return 1;
            }
            AP4_GlobalOptions::SetString(name, value);
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
    AP4_Processor* processor = NULL;
    if (method == METHOD_ISMA_AES) {
        if (kms_uri == NULL) {
            fprintf(stderr, "ERROR: method ISMA-IAEC requires --kms-uri\n");
            return 1;
        }
        AP4_IsmaEncryptingProcessor* isma_processor = new AP4_IsmaEncryptingProcessor(kms_uri);
        isma_processor->GetKeyMap().SetKeys(key_map);
        processor = isma_processor;
    } else if (method == METHOD_MARLIN_IPMP_ACBC ||
               method == METHOD_MARLIN_IPMP_ACGK) {
        bool use_group_key = (method == METHOD_MARLIN_IPMP_ACGK);
        if (use_group_key) {
            // check that the group key is set
            if (key_map.GetKey(0) == NULL) {
                fprintf(stderr, "ERROR: method MARLIN-IPMP-ACGK requires a group key\n");
                return 1;
            }
        }
        AP4_MarlinIpmpEncryptingProcessor* marlin_processor = 
            new AP4_MarlinIpmpEncryptingProcessor(use_group_key);
        marlin_processor->GetKeyMap().SetKeys(key_map);
        marlin_processor->GetPropertyMap().SetProperties(property_map);
        processor = marlin_processor;
    } else if (method == METHOD_OMA_PDCF_CTR ||
               method == METHOD_OMA_PDCF_CBC) {
        AP4_OmaDcfEncryptingProcessor* oma_processor = 
            new AP4_OmaDcfEncryptingProcessor(method == METHOD_OMA_PDCF_CTR?
                                              AP4_OMA_DCF_CIPHER_MODE_CTR :
                                              AP4_OMA_DCF_CIPHER_MODE_CBC);
        oma_processor->GetKeyMap().SetKeys(key_map);
        oma_processor->GetPropertyMap().SetProperties(property_map);
        processor = oma_processor;
    } else if (method == METHOD_PIFF_CTR  ||
               method == METHOD_PIFF_CBC  ||
               method == METHOD_MPEG_CENC ||
               method == METHOD_MPEG_CBC1 ||
               method == METHOD_MPEG_CENS ||
               method == METHOD_MPEG_CBCS) {
        AP4_CencVariant variant = AP4_CENC_VARIANT_MPEG_CENC;
        AP4_UI32        options = 0;
        
        // init local options based on global options
        if (AP4_GlobalOptions::GetBool("mpeg-cenc.eme-pssh")) {
            options |= AP4_CencEncryptingProcessor::OPTION_EME_PSSH;
        }
        if (AP4_GlobalOptions::GetBool("mpeg-cenc.piff-compatible")) {
            options |= AP4_CencEncryptingProcessor::OPTION_PIFF_COMPATIBILITY;
        }
        if (AP4_GlobalOptions::GetBool("mpeg-cenc.piff-iv-size-16")) {
            options |= AP4_CencEncryptingProcessor::OPTION_PIFF_IV_SIZE_16;
        }
        if (AP4_GlobalOptions::GetBool("mpeg-cenc.iv-size-8")) {
            options |= AP4_CencEncryptingProcessor::OPTION_IV_SIZE_8;
        }
        if (AP4_GlobalOptions::GetBool("mpeg-cenc.no-senc")) {
            options |= AP4_CencEncryptingProcessor::OPTION_NO_SENC;
        }

        switch (method) {
            case METHOD_PIFF_CBC:
                variant = AP4_CENC_VARIANT_PIFF_CBC;
                break;
                
            case METHOD_PIFF_CTR:
                variant = AP4_CENC_VARIANT_PIFF_CTR;
                break;
                
            case METHOD_MPEG_CENC:
                variant = AP4_CENC_VARIANT_MPEG_CENC;
                break;
                
            case METHOD_MPEG_CBC1:
                variant = AP4_CENC_VARIANT_MPEG_CBC1;
                break;

            case METHOD_MPEG_CENS:
                variant = AP4_CENC_VARIANT_MPEG_CENS;
                break;

            case METHOD_MPEG_CBCS:
                variant = AP4_CENC_VARIANT_MPEG_CBCS;
                break;

            default:
                break;
        }
        AP4_CencEncryptingProcessor* cenc_processor = new AP4_CencEncryptingProcessor(variant, options);
        cenc_processor->GetKeyMap().SetKeys(key_map);
        cenc_processor->GetPropertyMap().SetProperties(property_map);
        for (unsigned int i=0; i<pssh_atoms.ItemCount(); i++) {
            cenc_processor->GetPsshAtoms().Append(pssh_atoms[i]);
        }
        processor = cenc_processor;
    }
    
    // create the input stream
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }

    // create the output stream
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%s)\n", output_filename);
        return 1;
    }

    // create the fragments info stream if needed
    AP4_ByteStream* fragments_info = NULL;
    if (fragments_info_filename) {
        result = AP4_FileByteStream::Create(fragments_info_filename, AP4_FileByteStream::STREAM_MODE_READ, fragments_info);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot open fragments info file (%s)\n", fragments_info_filename);
            return 1;
        }
    }
    
    // process/decrypt the file
    ProgressListener listener;
    if (fragments_info) {
        bool check = CheckWarning(*fragments_info, key_map, method);
        if (strict && check) return 1;
        result = processor->Process(*input, *output, *fragments_info, show_progress?&listener:NULL);
    } else {
        bool check = CheckWarning(*input, key_map, method);
        if (strict && check) return 1;
        result = processor->Process(*input, *output, show_progress?&listener:NULL);
    }
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to process the file (%d)\n", result);
    }

    // cleanup
    delete processor;
    input->Release();
    output->Release();
    if (fragments_info) fragments_info->Release();
    for (unsigned int i=0; i<pssh_atoms.ItemCount(); i++) {
        delete pssh_atoms[i];
    }
    
    return 0;
}
