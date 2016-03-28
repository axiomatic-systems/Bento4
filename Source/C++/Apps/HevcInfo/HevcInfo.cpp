/*****************************************************************
|
|    AP4 - HEVC Bitstream Stream Info
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
#include "Ap4HevcParser.h"
#include "Ap4BitStream.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "HEVC Bitstream Info - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2014 Axiomatic Systems, LLC"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: hevcinfo [options] <input>\n");
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
    const char* filename = NULL;

    while (char* arg = *++argv) {
        if (filename == NULL) {
            filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
            return 1;
        }
    }
    if (filename == NULL) {
        fprintf(stderr, "ERROR: filename missing\n");
        return 1;
    }

    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename, 
                                                   AP4_FileByteStream::STREAM_MODE_READ, 
                                                   input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file %s (%d)\n", filename, result);
        return 1;
    }

    AP4_HevcNalParser parser;
    unsigned int  nalu_count = 0;
    for (;;) {
        bool eos;
        unsigned char input_buffer[4096];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        do {
            const AP4_DataBuffer* nalu = NULL;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset], bytes_in_buffer, bytes_consumed, nalu, eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            } 
            if (nalu) {
                const unsigned char* nalu_payload = (const unsigned char*)nalu->GetData();
                unsigned int         nalu_type = (nalu_payload[0] >> 1) & 0x3F;
                const char*          nalu_type_name = AP4_HevcNalParser::NaluTypeName(nalu_type);
                if (nalu_type_name == NULL) nalu_type_name = "UNKNOWN";
                printf("NALU %5d: size=%5d, type=%02d (%s)", 
                       nalu_count, 
                       (int)nalu->GetDataSize(),
                       nalu_type,
                       nalu_type_name);
                if (nalu_type == AP4_HEVC_NALU_TYPE_AUD_NUT) {
                    // Access Unit Delimiter
                    unsigned int pic_type = (nalu_payload[1]>>5);
                    const char*  pic_type_name = AP4_HevcNalParser::PicTypeName(pic_type);
                    if (pic_type_name == NULL) pic_type_name = "UNKNOWN";
                    printf(" [%d:%s]\n", pic_type, pic_type_name);
                } else {
                    printf("\n");
                }
                nalu_count++;
            }
            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer);
        if (eos) break;
    }
    
    input->Release();

    return 0;
}
