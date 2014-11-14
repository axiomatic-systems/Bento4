/*****************************************************************
|
|    AP4 - MP4 DCF Packager
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
#define BANNER "MP4 DCF Packager - Version 1.0.1\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2009 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
        BANNER 
        "\n\n"
        "usage: mp4dcfpackager --method <method> [options] <input> <output>\n"
        "  --method: <method> is NULL, CBC or CTR\n"
        "  Options:\n"
        "  --show-progress: show progress details\n"
        "  --content-type:  content MIME type\n"
        "  --content-id:    content ID\n"
        "  --rights-issuer: rights issuer URL\n"
        "  --key <k>:<iv>\n"   
        "      Specifies the key to use for encryption.\n"
        "      <k> a 128-bit key in hex (32 characters)\n"
        "      and <iv> a 128-bit IV or salting key in hex (32 characters)\n"
        "  --textual-header <name>:<value>\n"
        "      Specifies a textual header where <name> is the header name,\n"
        "      and <value> is the header value\n"
        "      (several --textual-header options can be used)\n"
        "\n"
        );
    exit(1);
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc == 1) PrintUsageAndExit();

    // parse options
    AP4_UI08       encryption_method        = 0;
    bool           encryption_method_is_set = false;
    AP4_UI08       padding_scheme           = 0;
    const char*    input_filename           = NULL;
    const char*    output_filename          = NULL;
    bool           show_progress            = false;
    bool           key_is_set               = false;
    unsigned char  key[16];
    unsigned char  iv[16];
    AP4_BlockCipher::CipherMode cipher_mode = AP4_BlockCipher::CBC;
    const char*    content_type             = "";
    const char*    content_id               = "";
    const char*    rights_issuer_url        = "";
    AP4_LargeSize  plaintext_length         = 0;
    AP4_DataBuffer textual_headers_buffer;
    AP4_TrackPropertyMap textual_headers;

    AP4_SetMemory(key, 0, sizeof(key));
    AP4_SetMemory(iv, 0, sizeof(iv));

    // parse the command line arguments
    char* arg;
    while ((arg = *++argv)) {
        if (!strcmp(arg, "--method")) {
            arg = *++argv;
            if (!strcmp(arg, "CBC")) {
                encryption_method = AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC;
                encryption_method_is_set = true;
                padding_scheme = AP4_OMA_DCF_PADDING_SCHEME_RFC_2630;
                cipher_mode = AP4_BlockCipher::CBC;
            } else if (!strcmp(arg, "CTR")) {
                encryption_method = AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR;
                encryption_method_is_set = true;
                padding_scheme = AP4_OMA_DCF_PADDING_SCHEME_NONE;
                cipher_mode = AP4_BlockCipher::CTR;
            } else if (!strcmp(arg, "NULL")) {
                encryption_method = AP4_OMA_DCF_ENCRYPTION_METHOD_NULL;
                encryption_method_is_set = true;
                padding_scheme = AP4_OMA_DCF_PADDING_SCHEME_NONE;
            } else {
                fprintf(stderr, "ERROR: invalid value for --method argument\n");
                return 1;
            }
        } else if (!strcmp(arg, "--show-progress")) {
            show_progress = true;
        } else if (!strcmp(arg, "--content-type")) {
            content_type = *++argv;
            if (content_type == NULL) {
                fprintf(stderr, "ERROR: missing argument for --content-type option\n");
                return 1;
            }
        } else if (!strcmp(arg, "--content-id")) {
            content_id = *++argv;
            if (content_id == NULL) {
                fprintf(stderr, "ERROR: missing argument for --content-id option\n");
                return 1;
            }
        } else if (!strcmp(arg, "--rights-issuer")) {
             rights_issuer_url = *++argv;
            if (rights_issuer_url == NULL) {
                fprintf(stderr, "ERROR: missing argument for --rights-issuer option\n");
                return 1;
            }
        } else if (!strcmp(arg, "--key")) {
            if (!encryption_method_is_set) {
                fprintf(stderr, "ERROR: --method argument must appear before --key\n");
                return 1;
            } else if (encryption_method_is_set == AP4_OMA_DCF_ENCRYPTION_METHOD_NULL) {
                fprintf(stderr, "ERROR: --key cannot be used with --method NULL\n");
                return 1;
            }
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --key option\n");
                return 1;
            }
            char* key_ascii = NULL;
            char* iv_ascii = NULL;
            if (AP4_FAILED(AP4_SplitArgs(arg, key_ascii, iv_ascii))) {
                fprintf(stderr, "ERROR: invalid argument for --key option\n");
                return 1;
            }
            if (AP4_ParseHex(key_ascii, key, 16)) {
                fprintf(stderr, "ERROR: invalid hex format for key\n");
            }
            if (AP4_ParseHex(iv_ascii, iv, 16)) {
                fprintf(stderr, "ERROR: invalid hex format for iv\n");
                return 1;
            }
            // check that the key is not already there
            if (key_is_set) {
                fprintf(stderr, "ERROR: key already set\n");
                return 1;
            }
            key_is_set = true;
        } else if (!strcmp(arg, "--textual-header")) {
            char* name = NULL;
            char* value = NULL;
            arg = *++argv;
            if (arg == NULL) {
                fprintf(stderr, "ERROR: missing argument for --textual-header option\n");
                return 1;
            }
            if (AP4_FAILED(AP4_SplitArgs(arg, name, value))) {
                fprintf(stderr, "ERROR: invalid argument for --textual-header option\n");
                return 1;
            }

            // check that the property is not already set
            if (textual_headers.GetProperty(0, name)) {
                fprintf(stderr, "ERROR: textual header %s already set\n", name);
                return 1;
            }
            // set the property in the map
            textual_headers.SetProperty(0, name, value);
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
    if (!encryption_method_is_set) {
        fprintf(stderr, "ERROR: missing --method argument\n");
        return 1;
    }
    if (!key_is_set) {
        fprintf(stderr, "ERROR: encryption key not specified\n");
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
    (void)show_progress; // avoid warnings

    // convert to a textual headers buffer
    textual_headers.GetTextualHeaders(0, textual_headers_buffer);

    // create the input stream
    AP4_Result result;
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s) %d\n", input_filename, result);
        return 1;
    }

    // get the size of the input
    result = input->GetSize(plaintext_length);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot get the size of the input\n");
        return 1;
    }
    
    // create an encrypting stream for the input
    AP4_ByteStream* encrypted_stream;
    if (encryption_method == AP4_OMA_DCF_ENCRYPTION_METHOD_NULL) {
        encrypted_stream = input;
    } else {
        result = AP4_EncryptingStream::Create(cipher_mode, *input, iv, 16, key, 16, true, &AP4_DefaultBlockCipherFactory::Instance, encrypted_stream);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: failed to create cipher (%d)\n", result);
            return 1;
        }
    }
    
    // create the output stream
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%s) %d\n", output_filename, result);
        return 1;
    }

    // create the file
    AP4_File file;
    
    // set the brand
    AP4_UI32 compatible_brands[1] = {AP4_OMA_DCF_BRAND_ODCF};
    file.SetFileType(AP4_OMA_DCF_BRAND_ODCF, 2, compatible_brands, 1);
    
    // create the odrm atom (force a 64-bit size)
    AP4_ContainerAtom* odrm = new AP4_ContainerAtom(AP4_ATOM_TYPE_ODRM, AP4_FULL_ATOM_HEADER_SIZE_64, true, 0, 0);
    
    // create the ohdr atom
    AP4_OhdrAtom* ohdr = new AP4_OhdrAtom(encryption_method, 
                                          padding_scheme,
                                          plaintext_length,
                                          content_id,
                                          rights_issuer_url,
                                          textual_headers_buffer.GetData(),
                                          textual_headers_buffer.GetDataSize());

    // create the odhe atom (the ownership is transfered)
    AP4_OdheAtom* odhe = new AP4_OdheAtom(content_type, ohdr);
    odrm->AddChild(odhe);
    
    // create the odda atom
    AP4_OddaAtom* odda = new AP4_OddaAtom(*encrypted_stream);
    odrm->AddChild(odda);
    
    // add the odrm atom to the file (the owndership is transfered)
    file.GetTopLevelAtoms().Add(odrm);
    
    // write the file to the output
    AP4_FileWriter::Write(file, *output);
    
    // cleanup
    input->Release();
    output->Release();

    return 0;
}
