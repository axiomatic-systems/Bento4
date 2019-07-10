/*****************************************************************
|
|    AP4 - MP4 File Processor
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
#define BANNER "MP4 File Editor - Version 1.2\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2017 Axiomatic Systems, LLC"

const AP4_LargeSize AP4_MP4EDIT_MAX_PAYLOAD_SIZE = (AP4_LargeSize)0xFFFFFFFFUL;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4edit [commands] <input> <output>\n"
            "    where commands include one or more of:\n"
            "    --insert <atom_path>:<atom_source>[:<position>]\n"
            "    --remove <atom_path>\n"
            "    --replace <atom_path>:<atom_source>\n"
            "\n"
            "    and <atom_source> may be either a filename for a file\n"
            "    that contains the atom data (header and payload), or\n"
            "    <uuid>#<file> with <uuid> specifying an atom uuid type,\n"
            "    as a 32-character hex value, and <file> a file with the atom\n"
            "    payload only.\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor
+---------------------------------------------------------------------*/
class AP4_EditingProcessor : public AP4_Processor
{
public:
    // types
    class Command {
    public:
        // types
        typedef enum {
            TYPE_INSERT,
            TYPE_REMOVE,
            TYPE_REPLACE
        } Type;

        // constructor
        Command(Type type, const char* atom_path, 
                           const char* atom_source,
                           int         position = -1) :
          m_Type(type), 
          m_AtomPath(atom_path), 
          m_AtomSource(atom_source),
          m_Position(position) {}

        // members
        Type       m_Type;
        AP4_String m_AtomPath;
        AP4_String m_AtomSource;
        int        m_Position;
    };

    // constructor and destructor
    virtual ~AP4_EditingProcessor();

    // methods
    virtual AP4_Result Initialize(AP4_AtomParent&   top_level,
                                  AP4_ByteStream&   stream,
                                  ProgressListener* listener);
    AP4_Result         AddCommand(Command::Type type, 
                                  const char*   atom_path, 
                                  const char*   file_path,
                                  int           position = -1);

private:
    // methods
    AP4_Result InsertAtom(AP4_String&     atom_source,
                          AP4_AtomParent* container,
                          int             position);
    AP4_Result DoRemove(Command* command, AP4_AtomParent& top_level);
    AP4_Result DoInsert(Command* command, AP4_AtomParent& top_level);
    AP4_Result DoReplace(Command* command, AP4_AtomParent& top_level);

    // members
    AP4_List<Command> m_Commands;
    AP4_AtomParent    m_TopLevelParent;
};

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::~AP4_EditingProcessor
+---------------------------------------------------------------------*/
AP4_EditingProcessor::~AP4_EditingProcessor()
{
    m_Commands.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::AddCommand
+---------------------------------------------------------------------*/
AP4_Result
AP4_EditingProcessor::AddCommand(Command::Type type, 
                                 const char*   atom_path, 
                                 const char*   file_path,
                                 int           position)
{
    return m_Commands.Add(new Command(type, atom_path, file_path, position));
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::Initialize
+---------------------------------------------------------------------*/
AP4_Result
AP4_EditingProcessor::Initialize(AP4_AtomParent& top_level,
                                 AP4_ByteStream&,
                                 ProgressListener*)
{
    AP4_Result result;

    AP4_List<Command>::Item* command_item = m_Commands.FirstItem();
    while (command_item) {
        Command* command = command_item->GetData();
        switch (command->m_Type) {
          case Command::TYPE_REMOVE:
            result = DoRemove(command, top_level);
            if (AP4_FAILED(result)) return result;
            break;

          case Command::TYPE_INSERT:
            result = DoInsert(command, top_level);
            if (AP4_FAILED(result)) return result;
            break;

          case Command::TYPE_REPLACE:
            result = DoReplace(command, top_level);
            if (AP4_FAILED(result)) return result;
            break;
        }
        command_item = command_item->GetNext();
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::DoRemove
+---------------------------------------------------------------------*/
AP4_Result
AP4_EditingProcessor::DoRemove(Command* command, AP4_AtomParent& top_level)
{
    AP4_Atom* atom = top_level.FindChild(command->m_AtomPath.GetChars());
    if (atom == NULL) {
        fprintf(stderr, "ERROR: atom '%s' not found\n", command->m_AtomPath.GetChars());
        return AP4_FAILURE;
    } else {
        atom->Detach();
        delete atom;
        return AP4_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::InsertAtom
+---------------------------------------------------------------------*/
AP4_Result
AP4_EditingProcessor::InsertAtom(AP4_String&     atom_source,
                                 AP4_AtomParent* container,
                                 int             position)
{
    const char* file_path = atom_source.GetChars();
    int separator = atom_source.Find('#');
    AP4_UI08 atom_uuid[16];
    bool is_uuid = false;
    if (separator == 32) {
        // uuid
        AP4_ParseHex(atom_source.GetChars(), atom_uuid, 16);
        file_path += separator + 1;
        is_uuid = true;
    }

    AP4_Atom* child = NULL;
    if (is_uuid) {
        // open the payload
        AP4_ByteStream* payload = NULL;
        AP4_Result result = AP4_FileByteStream::Create(file_path, AP4_FileByteStream::STREAM_MODE_READ, payload);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot open atom file (%s)\n", file_path);
            return result;
        }
        
        // check the size
        AP4_LargeSize payload_size = 0;
        result = payload->GetSize(payload_size);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot get atom file size\n");
            payload->Release();
            return result;
        }
        if (payload_size > AP4_MP4EDIT_MAX_PAYLOAD_SIZE) {
            fprintf(stderr, "ERROR: atom payload too large\n");
            payload->Release();
            return AP4_ERROR_OUT_OF_RANGE;
        }
        
        // synthesize a uuid atom
        child = new AP4_UnknownUuidAtom(payload_size + AP4_ATOM_HEADER_SIZE + 16, atom_uuid, *payload);
        payload->Release();
    } else {
        // read the atom to insert
        AP4_ByteStream* input = NULL;
        AP4_Result result = AP4_FileByteStream::Create(file_path, AP4_FileByteStream::STREAM_MODE_READ, input);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot open atom file (%s)\n", file_path);
            return AP4_FAILURE;
        }

        AP4_DefaultAtomFactory atom_factory;
        result = atom_factory.CreateAtomFromStream(*input, child);
        input->Release();
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: failed to create atom\n");
            return AP4_FAILURE;
        }
    }
    
    // insert the atom
    AP4_Result result = container->AddChild(child, position);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to insert atom\n");
        delete child;
        return result;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::DoInsert
+---------------------------------------------------------------------*/
AP4_Result
AP4_EditingProcessor::DoInsert(Command* command, AP4_AtomParent& top_level)
{
    AP4_AtomParent* parent = NULL;
    if (command->m_AtomPath.GetLength() == 0) {
        // insert into the toplevel list
        parent = &top_level;
    } else {
        // find the atom to insert into
        AP4_Atom* atom = top_level.FindChild(command->m_AtomPath.GetChars(), true);
        if (atom == NULL) {
            fprintf(stderr, "ERROR: atom '%s' not found\n", 
                command->m_AtomPath.GetChars());
            return AP4_FAILURE;
        }

        // check that the atom is a container
        parent = AP4_DYNAMIC_CAST(AP4_AtomParent, atom);
    }

    // check that we have a place to insert into
    if (parent == NULL) {
        fprintf(stderr, "ERROR: atom '%s' is not a container\n",
            command->m_AtomPath.GetChars());
        return AP4_FAILURE;
    }

    return InsertAtom(command->m_AtomSource, parent, command->m_Position);
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::DoReplace
+---------------------------------------------------------------------*/
AP4_Result
AP4_EditingProcessor::DoReplace(Command* command, AP4_AtomParent& top_level)
{
    // remove the atom
    AP4_Atom* atom = top_level.FindChild(command->m_AtomPath.GetChars());
    if (atom == NULL) {
        fprintf(stderr, "ERROR: atom '%s' not found\n", command->m_AtomPath.GetChars());
        return AP4_FAILURE;
    } else {
        // find the position of the atom in the parent
        AP4_AtomParent* parent = atom->GetParent();
        int position = 0;
        AP4_List<AP4_Atom>::Item* list_item = parent->GetChildren().FirstItem();
        while (list_item) {
            if (list_item->GetData() == atom) break;
            position++;
            list_item = list_item->GetNext();
        }

        // remove the atom from the parent
        atom->Detach();
        delete atom;

        // insert the replacement
        return InsertAtom(command->m_AtomSource, parent, position);
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 3) {
        PrintUsageAndExit();
    }
    
    // create initial objects
    AP4_EditingProcessor processor;

    // parse arguments
    const char* input_filename = NULL;
    const char* output_filename = NULL;
    char* arg;
    while ((arg = *++argv)) {
        if (!AP4_CompareStrings(arg, "--insert")) {
            char* param = *++argv;
            if (param == NULL) {
                fprintf(stderr, "ERROR: missing argument for --insert command\n");
                return 1;
            }
            char* atom_path = NULL;
            char* file_path = NULL;
            if (AP4_SUCCEEDED(AP4_SplitArgs(param, atom_path, file_path))) {
                if (file_path[0] == '\0') {
                    // empty file path
                    fprintf(stderr, "ERROR: missing file path in --insert argument\n");
                    return 1;
                }
                bool is_windows_path = false;
                if (((file_path[0] >= 'a' && file_path[0] <= 'z') ||
                     (file_path[0] >= 'A' && file_path[0] <= 'Z')) &&
                    file_path[1] == ':' && file_path[2] == '\\') {
                    is_windows_path = true;
                    file_path[1] = '@'; // temporary replacement so that the next split works
                }
                int position = -1;
                char* position_str = NULL;
                if (AP4_SUCCEEDED(AP4_SplitArgs(file_path, file_path, position_str))) {
                    if (position_str) {
                        position = (int)strtoul(position_str, NULL, 10);
                    }
                }
                if (is_windows_path) {
                    file_path[1] = ':';
                }
                processor.AddCommand(AP4_EditingProcessor::Command::TYPE_INSERT, atom_path, file_path, position);
            } else {
                fprintf(stderr, "ERROR: invalid format for --insert command argument\n");
                return 1;
            }
        } else if (!AP4_CompareStrings(arg, "--remove")) {
            char* atom_path = *++argv;
            if (atom_path == NULL) {
                fprintf(stderr, "ERROR: missing argument for --remove command\n");
                return 1;
            }
            processor.AddCommand(AP4_EditingProcessor::Command::TYPE_REMOVE, atom_path, "");
        } else if (!AP4_CompareStrings(arg, "--replace")) {
            char* param = *++argv;
            if (param == NULL) {
                fprintf(stderr, "ERROR: missing argument for --replace command\n");
                return 1;
            }
            char* atom_path = NULL;
            char* file_path = NULL;
            if (AP4_SUCCEEDED(AP4_SplitArgs(param, atom_path, file_path))) {
                processor.AddCommand(AP4_EditingProcessor::Command::TYPE_REPLACE, atom_path, file_path);
            } else {
                fprintf(stderr, "ERROR: invalid format for --replace command argument\n");
                return 1;
            }
        } else if (input_filename == NULL) {
            input_filename = arg;
        } else if (output_filename == NULL) {
            output_filename = arg;
        } else {
            fprintf(stderr, "ERROR: invalid command line argument (%s)\n", arg);
            return 1;
        }
    }

    // check arguments
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
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", input_filename);
        return 1;
    }

	// create the output stream
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open output file (%s)\n", output_filename);
        input->Release();
        return 1;
    }

    // process!
    processor.Process(*input, *output);

    // cleanup
    input->Release();
    output->Release();

    return 0;
}

