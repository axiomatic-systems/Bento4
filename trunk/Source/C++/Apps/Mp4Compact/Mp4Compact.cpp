/*****************************************************************
|
|    AP4 - MP4 Compacter
|
|    Copyright 2002-2011 Axiomatic Systems, LLC
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
#define BANNER "MP4 Compacter - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2011 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
        BANNER 
        "\n\n"
        "usage: mp4compact [options] <input> <output>\n"
        "Options:\n"
        "  --verbose\n"
        );
    exit(1);
}

/*----------------------------------------------------------------------
|   AP4_CompactingProcessor
+---------------------------------------------------------------------*/
class AP4_CompactingProcessor : public AP4_Processor
{
public:
    //  inner classes
    class TrackHandler : public AP4_Processor::TrackHandler 
    {
    public:
        TrackHandler(AP4_CompactingProcessor& outer, AP4_TrakAtom* trak_atom) :
            m_Outer(outer),
            m_TrakAtom(trak_atom),
            m_StszAtom(NULL) {}
        ~TrackHandler();
        virtual AP4_Result ProcessTrack();
        virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                         AP4_DataBuffer& data_out) {
            return data_out.SetData(data_in.GetData(), data_in.GetDataSize());
        }
    private:
        AP4_CompactingProcessor& m_Outer;
        AP4_TrakAtom*            m_TrakAtom;
        AP4_StszAtom*            m_StszAtom;
    };
    
    // methods
    AP4_CompactingProcessor(bool verbose) : m_Verbose(verbose), m_SizeReduction(0) {}
    ~AP4_CompactingProcessor();
    virtual TrackHandler* CreateTrackHandler(AP4_TrakAtom* trak_atom) { 
        return new TrackHandler(*this, trak_atom); 
    }
    
    // members
    bool     m_Verbose;
    AP4_UI32 m_SizeReduction;
};

/*----------------------------------------------------------------------
|   AP4_CompactingProcessor::~AP4_CompactingProcessor
+---------------------------------------------------------------------*/
AP4_CompactingProcessor::~AP4_CompactingProcessor()
{
    if (m_Verbose) {
        printf("Total stz2 reduction = %d bytes\n", m_SizeReduction);
    }
}

/*----------------------------------------------------------------------
|   AP4_CompactingProcessor::TrackHandler::~TrackHandler()
+---------------------------------------------------------------------*/
AP4_CompactingProcessor::TrackHandler::~TrackHandler()
{
    delete m_StszAtom;
}

/*----------------------------------------------------------------------
|   AP4_CompactingProcessor::TrackHandler::ProcessTrack
+---------------------------------------------------------------------*/
AP4_Result
AP4_CompactingProcessor::TrackHandler::ProcessTrack()
{
    // find the stsz atom
    AP4_ContainerAtom* stbl = AP4_DYNAMIC_CAST(AP4_ContainerAtom, m_TrakAtom->FindChild("mdia/minf/stbl"));
    if (stbl == NULL) return AP4_SUCCESS;
    AP4_StszAtom* stsz = AP4_DYNAMIC_CAST(AP4_StszAtom, stbl->GetChild(AP4_ATOM_TYPE_STSZ));
    if (stsz == NULL) return AP4_SUCCESS;
    
    // check if we can reduce the size of stsz by changing it to stz2
    AP4_UI32 max_size = 0;
    for (unsigned int i=1; i<=stsz->GetSampleCount(); i++) {
        AP4_Size sample_size;
        stsz->GetSampleSize(i, sample_size);
        if (sample_size > max_size) {
            max_size = sample_size;
        }
    }
    AP4_UI08 field_size = 0;
    if (max_size <= 0xFF) {
        field_size = 1;
    } else if (max_size <= 0xFFFF) {
        field_size = 2;
    }
    if (m_Outer.m_Verbose) printf("Track %d: ", m_TrakAtom->GetId());
    if (field_size == 0) {
        if (m_Outer.m_Verbose) {
            printf("no stz2 reduction possible\n");
        }
        return AP4_SUCCESS;
    } else {
        if (m_Outer.m_Verbose) {
            unsigned int reduction = (4-field_size)*stsz->GetSampleCount();
            printf("stz2 reduction = %d bytes\n", reduction);
            m_Outer.m_SizeReduction += reduction;
        }
    }
    
    // detach the original stsz atom so we can destroy it later
    m_StszAtom = stsz;
    stsz->Detach();
    
    // create an stz2 atom and populate its entries
    AP4_Stz2Atom* stz2 = new AP4_Stz2Atom(field_size);
    for (unsigned int i=1; i<=m_StszAtom->GetSampleCount(); i++) {
        AP4_Size sample_size;
        m_StszAtom->GetSampleSize(i, sample_size);
        stz2->AddEntry(sample_size);
    }
    stbl->AddChild(stz2);
    
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
    const char* input_filename  = NULL;
    const char* output_filename = NULL;
    bool        verbose         = false;
    AP4_Result  result;

    // parse the command line arguments
    char* arg;
    while ((arg = *++argv)) {
        if (!AP4_CompareStrings(arg, "--verbose")) {
            verbose = true;
        } else if (input_filename == NULL) {
            input_filename = arg;
        } else if (output_filename == NULL) {
            output_filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument (%s)\n", arg);
            return 1;
        }
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

    // process the file
    AP4_CompactingProcessor* processor = new AP4_CompactingProcessor(verbose);
    result = processor->Process(*input, *output, NULL);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to process the file (%d)\n", result);
    }

    // cleanup
    delete processor;
    input->Release();
    output->Release();

    return 0;
}
