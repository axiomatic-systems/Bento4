/*****************************************************************
|
|    AP4 - MP4 DCF Packager
|
|    Copyright 2008 Gilles Boccon-Gibod
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
#define BANNER "MP4 DCF Packager - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2008 Gilles Boccon-Gibod & Julien Boeuf"

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
        "  --content-type: content mime type\n"
        "  --key <k>:<iv>\n"   
        "      Specifies the key to use for encryption.\n"
        "      <k> a 128-bit key in hex (32 characters)\n"
        "      and <iv> a 64-bit IV or salting key in hex (16 characters)\n"
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
    AP4_UI08      encryption_method        = 0;
    bool          encryption_method_is_set = false;
    AP4_UI08      padding_scheme           = 0;
    const char*   input_filename           = NULL;
    const char*   output_filename          = NULL;
    bool          show_progress            = false;
    bool          key_is_set               = false;
    unsigned char key[16];
    unsigned char iv[16];
    const char*   content_type             = "";
    const char*   content_id               = "";
    const char*   rights_issuer_url        = "";
    AP4_LargeSize plaintext_length         = 0;
    AP4_UI08*     textual_headers          = NULL;
    AP4_Size      textual_headers_size     = 0;
    
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
            } else if (!strcmp(arg, "CTR")) {
                encryption_method = AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR;
                encryption_method_is_set = true;
                padding_scheme = AP4_OMA_DCF_PADDING_SCHEME_NONE;
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
            if (AP4_ParseHex(iv_ascii, iv, 8)) {
                fprintf(stderr, "ERROR: invalid hex format for iv\n");
                return 1;
            }
            // check that the key is not already there
            if (key_is_set) {
                fprintf(stderr, "ERROR: key already set\n");
                return 1;
            }
            key_is_set = true;
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

    // create the file
    AP4_File file;
    
    // set the brand
    AP4_UI32 compatible_brands[1] = {AP4_OMA_DCF_BRAND_ODCF};
    file.SetFileType(AP4_OMA_DCF_BRAND_ODCF, 2, compatible_brands, 1);
    
    // create the odrm atom (force a 64-bit size)
    AP4_ContainerAtom* odrm = new AP4_ContainerAtom(AP4_ATOM_TYPE_ODRM, 0, 0, 0);
    odrm->SetSize(AP4_FULL_ATOM_HEADER_SIZE+8, true);
    
    // create the ohdr atom
    AP4_OhdrAtom* ohdr = new AP4_OhdrAtom(encryption_method, 
                                          padding_scheme,
                                          plaintext_length,
                                          content_id,
                                          rights_issuer_url,
                                          textual_headers,
                                          textual_headers_size);

    // create the odhe atom (the ownership is transfered)
    AP4_OdheAtom* odhe = new AP4_OdheAtom(content_type, ohdr);
    
    // add the odhe atom to the odrm container
    odrm->AddChild(odhe);
    
    // create the odda atom
    //AP4_OddaAtom* odda = new AP4_OddaAtom(encrypted_data);
    
    // add the odrm atom to the file (the owndership is transfered)
    file.GetOtherAtoms().Add(odrm);
    
    // create a writer to write the file
    AP4_FileWriter writer(file);

    // write the file to the output
    writer.Write(*output);
    
    // cleanup
    input->Release();
    output->Release();

    return 0;
}
