/*****************************************************************
|
|    AP4 - MP4 Atom List
|
|    Copyright 2002-2022 Axiomatic Systems, LLC
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
#include <iostream>
#include <string>

#include "Ap4.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 Atom List - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2022 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr,
            BANNER
            "\n\nusage: mp4ls <input>\n");
    exit(1);
}


static void
PrintAtom(AP4_Atom* atom, std::string& path)
{
    char type[5];
    AP4_FormatFourChars(type, atom->GetType());
    std::string new_path = path + ((path.empty()) ? "" : "/") + type;

    AP4_AtomParent* parent = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
    if (parent != NULL){
        AP4_List<AP4_Atom>& children = parent->GetChildren();
        AP4_Cardinal n_children = children.ItemCount();
        for (int i=0; i<n_children; ++i){
            AP4_Atom* child = NULL;
            children.Get(i, child);
            PrintAtom(child, new_path);
        }
    }
    std::cout << new_path << std::endl;
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

    // parse arguments
    const char* input_filename  = NULL;
    char* arg;
    while ((arg = *++argv)) {
        if (input_filename == NULL) {
            input_filename = arg;
        } else {
            fprintf(stderr, "ERROR: invalid command line argument (%s)\n", arg);
            return 1;
        }
    }

    if (input_filename == NULL) {
        fprintf(stderr, "ERROR: missing input filename\n");
        return 1;
    }

    // create the input stream
    AP4_Result result;
    AP4_ByteStream* input = NULL;
    result = AP4_FileByteStream::Create(input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }

    // parse the atoms
    AP4_AtomParent top_level;
    AP4_Atom* atom;
    AP4_DefaultAtomFactory atom_factory;
    while (atom_factory.CreateAtomFromStream(*input, atom) == AP4_SUCCESS) {
        top_level.AddChild(atom);
    }

    // release the input
    input->Release();

    // list the atoms
    AP4_List<AP4_Atom>& children = top_level.GetChildren();
    AP4_Cardinal n_children = children.ItemCount();
    std::string path;
    for (int i=0; i<n_children; ++i){
        children.Get(i, atom);
        PrintAtom(atom, path);
    }

    return 0;
}

