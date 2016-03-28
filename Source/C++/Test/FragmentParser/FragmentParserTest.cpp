/*****************************************************************
|
|    AP4 - Fragment Parser Test
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
|   macros
+---------------------------------------------------------------------*/
#define CHECK(x) do { \
    if (!(x)) { fprintf(stderr, "ERROR line %d\n", __LINE__); return -1; }\
} while (0)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "Fragment Parser Test - Version 1.0\n"\
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
            "\n\nusage: fragmentparsertest <test-filename>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ShowSample
+---------------------------------------------------------------------*/
static void
ShowSample(AP4_Sample& sample, unsigned int index, AP4_SampleDecrypter* sample_decrypter)
{
    printf("[%06d] size=%6d duration=%6d", 
           index, 
           (int)sample.GetSize(), 
           (int)sample.GetDuration());
    printf(" offset=%10lld dts=%10lld cts=%10lld ", 
           sample.GetOffset(),
           sample.GetDts(), 
           sample.GetCts());
    if (sample.IsSync()) {
        printf(" [S] ");
    } else {
        printf("     ");
    }

    AP4_DataBuffer sample_data;
    sample.ReadData(sample_data);
    AP4_DataBuffer* data = &sample_data;
    
    AP4_DataBuffer decrypted_sample_data;
    if (sample_decrypter) {
        sample_decrypter->DecryptSampleData(sample_data, decrypted_sample_data);
        data = & decrypted_sample_data;
    }
    
    unsigned int show = data->GetDataSize();
    if (show > 12) show = 12; // max first 12 chars
    
    for (unsigned int i=0; i<show; i++) {
        printf("%02x", data->GetData()[i]);
    }
    if (show == data->GetDataSize()) {
        printf("\n");
    } else {
        printf("...\n");
    }
}

/*----------------------------------------------------------------------
|   ProcessSamples
+---------------------------------------------------------------------*/
static int
ProcessSamples(AP4_Track*               track, 
               AP4_ContainerAtom*       traf,
               AP4_ByteStream&          moof_data,
               AP4_Position             moof_data_offset,
               AP4_FragmentSampleTable* sample_table)
{
    // look at the first sample description
    AP4_SampleDecrypter* sample_decrypter = NULL;
    if (track) {
        AP4_UI08 key_1[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        AP4_UI08 key_2[16] = {0xAA, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        AP4_UI08* key = NULL;
        AP4_Size key_size = 16;
        if (track->GetId() == 1) {
            key = key_1;
        } else {
            key = key_2;
        }
        AP4_SampleDescription* sample_description = track->GetSampleDescription(0);
        if (sample_description->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
            AP4_ProtectedSampleDescription* prot_desc = AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, sample_description);
            sample_decrypter = AP4_SampleDecrypter::Create(prot_desc,
                                                           traf,
                                                           moof_data,
                                                           moof_data_offset,
                                                           key, 
                                                           key_size,
                                                           NULL);
        }
    }
    
    printf("found %d samples\n", sample_table->GetSampleCount());
    for (unsigned int i=0; i<sample_table->GetSampleCount(); i++) {
        AP4_Sample sample;
        AP4_Result result = sample_table->GetSample(i, sample);
        CHECK(AP4_SUCCEEDED(result));
        
        ShowSample(sample, i, sample_decrypter);
    }
    
    return 0;
}

/*----------------------------------------------------------------------
|   ProcessMoof
+---------------------------------------------------------------------*/
static int
ProcessMoof(AP4_Movie*         movie, 
            AP4_ContainerAtom* moof, 
            AP4_ByteStream*    sample_stream, 
            AP4_Position       moof_offset, 
            AP4_Position       mdat_payload_offset)
{
    AP4_Result result;
    
    AP4_MovieFragment* fragment = new AP4_MovieFragment(moof);
    printf("fragment sequence number=%d\n", fragment->GetSequenceNumber());
    
    AP4_FragmentSampleTable* sample_table = NULL;
    
    // get all track IDs in this fragment
    AP4_Array<AP4_UI32> ids;
    fragment->GetTrackIds(ids);
    printf("Found %d tracks in fragment: ", ids.ItemCount());
    for (unsigned int i=0; i<ids.ItemCount(); i++) {
        printf("%d ", ids[i]);
    }
    printf("\n");
    
    for (unsigned int i=0; i<ids.ItemCount(); i++) {
        AP4_Track* track = NULL;
        if (movie) {
            track = movie->GetTrack(ids[i]);
        }
        AP4_ContainerAtom* traf = NULL;
        fragment->GetTrafAtom(ids[i], traf);
        
        printf("processing moof for track id %d\n", ids[i]);
        result = fragment->CreateSampleTable(movie, ids[i], sample_stream, moof_offset, mdat_payload_offset, 0, sample_table);
        CHECK(result == AP4_SUCCESS || result == AP4_ERROR_NO_SUCH_ITEM);
        if (AP4_SUCCEEDED(result) ) {
            ProcessSamples(track, traf, *sample_stream, moof_offset, sample_table);
            delete sample_table;
        } else {
            printf("no sample table for this track\n");
        }
    }
    
    delete fragment;
    return 0;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc != 2) {
        PrintUsageAndExit();
    }
    const char* input_filename  = argv[1];
    
    // open the input
    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }
        
    // get the movie
    AP4_File* file = new AP4_File(*input, true);
    AP4_Movie* movie = file->GetMovie();
    
    AP4_Atom* atom = NULL;
    AP4_DefaultAtomFactory atom_factory;
    do {
        // process the next atom
        result = atom_factory.CreateAtomFromStream(*input, atom);
        if (AP4_SUCCEEDED(result)) {
            printf("atom size=%lld\n", atom->GetSize());
            if (atom->GetType() == AP4_ATOM_TYPE_MOOF) {
                AP4_ContainerAtom* moof = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
                if (moof) {
                    // remember where we are in the stream
                    AP4_Position position = 0;
                    input->Tell(position);
        
                    // process the movie fragment
                    ProcessMoof(movie, moof, input, position-atom->GetSize(), position+8);

                    // go back to where we were before processing the fragment
                    input->Seek(position);
                }
            } else {
                delete atom;
            }            
        }
    } while (AP4_SUCCEEDED(result));
    
    // cleanup
    delete file;
    input->Release();

    return 0;                                            
}

