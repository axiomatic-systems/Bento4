/*****************************************************************
|
|    AP4 - Basic Test
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
#define BANNER "Basic Test - Version 1.0\n"\
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
            "\n\nusage: basictest [options] <input> <output>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   SampleDescriptionCloneTest
+---------------------------------------------------------------------*/
static bool
SampleDescriptionCloneTest(AP4_File* file)
{
    AP4_Track* track = file->GetMovie()->GetTrack(AP4_Track::TYPE_VIDEO);
    AP4_SampleDescription* sdesc = track->GetSampleDescription(0);
    if (sdesc) {
        AP4_SampleDescription* clone = sdesc->Clone();
    
        if (clone == NULL) return false;
        if (clone->GetFormat()         != sdesc->GetFormat())         return false;
    }
    
    AP4_ProtectedSampleDescription* psdesc = AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, sdesc);
    if (psdesc) {
        AP4_SampleDescription* clone = psdesc->Clone();
        AP4_ProtectedSampleDescription* pclone = AP4_DYNAMIC_CAST(AP4_ProtectedSampleDescription, clone);
    
        if (pclone == NULL) return false;
        if (pclone->GetFormat()         != psdesc->GetFormat())         return false;
        if (pclone->GetOriginalFormat() != psdesc->GetOriginalFormat()) return false;
    }
    
    return true;
}

/*----------------------------------------------------------------------
|   AtomCloneTest
+---------------------------------------------------------------------*/
static bool
AtomCloneTest(AP4_Atom* atom, unsigned int depth)
{
    for (unsigned int i=0l; i<depth; i++) {
        printf(" ");
    }

    char str[5];
    AP4_FormatFourCharsPrintable(str, atom->GetType());
    str[4] = 0;
    printf("+%s\n", str);

    AP4_ContainerAtom* container = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
    if (container) {
        AP4_List<AP4_Atom>& children = container->GetChildren();
        AP4_List<AP4_Atom>::Item* child = children.FirstItem();
        while (child) {
            AtomCloneTest(child->GetData(), depth+1);
            child = child->GetNext();
        }
    } else {
        AP4_Atom* clone = atom->Clone();
        if (clone == NULL) {
            fprintf(stderr, "ERROR: clone is NULL\n");
            return false;
        }
        if (clone->GetSize() != atom->GetSize()) {
            fprintf(stderr, "ERROR: clone.GetSize() [%lld] != atom->GetSize() [%lld]\n", clone->GetSize(), atom->GetSize());
            return false;
        }
        AP4_MemoryByteStream* mbs = new AP4_MemoryByteStream();
        clone->Write(*mbs);
        if (mbs->GetDataSize() != clone->GetSize()) {
            fprintf(stderr, "ERROR: serialized size [%d] != atom size [%lld]\n", mbs->GetDataSize(), clone->GetSize());
            return false;
        }
        mbs->Release();
    }
    
    return true;
}

/*----------------------------------------------------------------------
|   AtomCloneTest
+---------------------------------------------------------------------*/
static bool
AtomCloneTest(AP4_ByteStream* input)
{
    AP4_DefaultAtomFactory factory;
    
    AP4_Result result;
    do {
        AP4_Atom* atom = NULL;
        result = factory.CreateAtomFromStream(*input, atom);
        if (AP4_SUCCEEDED(result)) {
            AtomCloneTest(atom, 0);
        }
    } while (AP4_SUCCEEDED(result));
    
    return true;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc != 3) {
        PrintUsageAndExit();
    }
    const char* input_filename  = argv[1];
    const char* output_filename = argv[2];
    
    // open the input
    AP4_ByteStream* input = NULL;
    try {
        input = new AP4_FileByteStream(input_filename,
                               AP4_FileByteStream::STREAM_MODE_READ);
    } catch (AP4_Exception) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }
    
    bool result = AtomCloneTest(input);
    if (!result) {
        fprintf(stderr, "SampleDescriptionCloneTest failed\n");
    }
    input->Seek(0);
    
    // open the output
    AP4_ByteStream* output = new AP4_FileByteStream(
        output_filename,
        AP4_FileByteStream::STREAM_MODE_WRITE);
    
    // parse the file
    AP4_File* mp4_file = new AP4_File(*input);
    
    result = SampleDescriptionCloneTest(mp4_file);
    if (!result) {
        fprintf(stderr, "SampleDescriptionCloneTest failed\n");
    }
    
    // write the file to the output
    AP4_FileWriter::Write(*mp4_file, *output);
    
    // cleanup
    delete mp4_file;
    input->Release();
    output->Release();

    return 0;                                            
}

