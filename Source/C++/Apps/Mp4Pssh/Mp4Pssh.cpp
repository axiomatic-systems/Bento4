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
#define BANNER "MP4 PSSH Editor - Version 1.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2014 Axiomatic Systems, LLC"

const unsigned int MP4_PSSH_MAX_DATA_SIZE = 8*1024*1024; // 8MB

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4pssh [commands] <input> <output>\n"
            "    where commands include one or more of:\n"
            "    --insert <system-id-hex>:<pssh-data-file>\n"
            "    --remove <system-id-hex>\n"
            "    --replace <system-id-hex>:<pssh-data-file>\n");
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
        Command(Type type,
                const unsigned char* system_id,
                AP4_DataBuffer*      pssh_data) :
          m_Type(type) {
            AP4_CopyMemory(m_SystemId, system_id, 16);
            m_Pssh = new AP4_PsshAtom(system_id);
            if (pssh_data) {
                m_Pssh->SetData(pssh_data->GetData(), pssh_data->GetDataSize());
            }
        }

        // members
        Type           m_Type;
        AP4_UI08       m_SystemId[16];
        AP4_PsshAtom*  m_Pssh;
    };

    // constructor and destructor
    virtual ~AP4_EditingProcessor();

    // methods
    virtual AP4_Result Initialize(AP4_AtomParent&   top_level,
                                  AP4_ByteStream&   stream,
                                  ProgressListener* listener);
    AP4_Result         AddCommand(Command::Type        type,
                                  const unsigned char* system_id,
                                  AP4_DataBuffer*      pssh_data);

private:
    // methods
    AP4_Result DoRemove(Command* command, AP4_AtomParent& top_level);
    AP4_Result DoInsert(Command* command, AP4_AtomParent& top_level);
    AP4_Result DoReplace(Command* command, AP4_AtomParent& top_level);

    // members
    AP4_List<Command> m_Commands;
    AP4_AtomParent    m_TopLevelParent;
};

/*----------------------------------------------------------------------
|   FindPssh
+---------------------------------------------------------------------*/
static AP4_PsshAtom*
FindPssh(const unsigned char* system_id, AP4_AtomParent& top_level)
{
    AP4_ContainerAtom* moov = AP4_DYNAMIC_CAST(AP4_ContainerAtom, top_level.GetChild(AP4_ATOM_TYPE_MOOV));
    if (moov == NULL) {
        return NULL;
    }
    for (AP4_List<AP4_Atom>::Item* child = moov->GetChildren().FirstItem();
                                   child;
                                   child = child->GetNext()) {
        AP4_Atom* atom = child->GetData();
        if (atom->GetType() == AP4_ATOM_TYPE_PSSH) {
            AP4_PsshAtom* pssh = AP4_DYNAMIC_CAST(AP4_PsshAtom, atom);
            if (pssh && AP4_CompareMemory(pssh->GetSystemId(), system_id, 16) == 0) {
                return pssh;
            }
        }
    }
    
    return NULL;
}

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
AP4_EditingProcessor::AddCommand(Command::Type        type,
                                 const unsigned char* system_id,
                                 AP4_DataBuffer*      pssh_data)
{
    return m_Commands.Add(new Command(type, system_id, pssh_data));
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
    AP4_PsshAtom* pssh = FindPssh(command->m_SystemId, top_level);
    if (pssh == NULL) {
        fprintf(stderr, "ERROR: no 'pssh' box with the requested ID was found\n");
        return AP4_FAILURE;
    }

    pssh->Detach();
    delete pssh;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::DoInsert
+---------------------------------------------------------------------*/
AP4_Result
AP4_EditingProcessor::DoInsert(Command* command, AP4_AtomParent& top_level)
{
    AP4_ContainerAtom* moov = AP4_DYNAMIC_CAST(AP4_ContainerAtom, top_level.GetChild(AP4_ATOM_TYPE_MOOV));

    // check that we have a place to insert into
    if (moov == NULL) {
        fprintf(stderr, "ERROR: no 'moov' atom found\n");
        return AP4_FAILURE;
    }

    return moov->AddChild(command->m_Pssh);
}

/*----------------------------------------------------------------------
|   AP4_EditingProcessor::DoReplace
+---------------------------------------------------------------------*/
AP4_Result
AP4_EditingProcessor::DoReplace(Command* command, AP4_AtomParent& top_level)
{
    AP4_ContainerAtom* moov = AP4_DYNAMIC_CAST(AP4_ContainerAtom, top_level.GetChild(AP4_ATOM_TYPE_MOOV));

    // check that we have a place to insert into
    if (moov == NULL) {
        fprintf(stderr, "ERROR: no 'moov' atom found\n");
        return AP4_FAILURE;
    }

    // remove the existing one
    AP4_PsshAtom* pssh = FindPssh(command->m_SystemId, top_level);
    if (pssh) {
        pssh->Detach();
        delete pssh;
    }

    // insert the new one
    return moov->AddChild(command->m_Pssh);
}

/*----------------------------------------------------------------------
|   LoadFile
+---------------------------------------------------------------------*/
static AP4_Result
LoadFile(const char* filename, AP4_DataBuffer& buffer)
{
	// create the input stream
    AP4_ByteStream* input = NULL;
    AP4_Result result = AP4_FileByteStream::Create(filename, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file (%s)\n", filename);
        return result;
    }
    AP4_LargeSize file_size = 0;
    result = input->GetSize(file_size);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot get file size (%d)\n", result);
        input->Release();
        return result;
    }
    if (file_size > MP4_PSSH_MAX_DATA_SIZE) {
        fprintf(stderr, "ERROR: data file size too large\n");
        input->Release();
        return AP4_ERROR_OUT_OF_RANGE;
    }
    buffer.SetDataSize((AP4_UI32)file_size);
    
    result = input->Read(buffer.UseData(), (AP4_UI32)file_size);
    if (file_size > MP4_PSSH_MAX_DATA_SIZE) {
        fprintf(stderr, "ERROR: failed to read data from file (%d)\n", result);
        input->Release();
        return result;
    }

    input->Release();
    return AP4_SUCCESS;
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
    AP4_Result result;
    while ((arg = *++argv)) {
        if (!AP4_CompareStrings(arg, "--insert")) {
            char* param = *++argv;
            if (param == NULL) {
                fprintf(stderr, "ERROR: missing argument for --insert command\n");
                return 1;
            }
            char* system_id_hex = NULL;
            char* file_path = NULL;
            if (AP4_SUCCEEDED(AP4_SplitArgs(param, system_id_hex, file_path))) {
                if (file_path[0] == '\0') {
                    // empty file path
                    fprintf(stderr, "ERROR: missing file path in --insert argument\n");
                    return 1;
                }
                AP4_DataBuffer pssh_data;
                unsigned char  system_id[16];
                result = LoadFile(file_path, pssh_data);
                if (AP4_FAILED(result)) return 1;
                result = AP4_ParseHex(system_id_hex, system_id, 16);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: invalid system ID (%s)\n", system_id_hex);
                    return 1;
                }
                processor.AddCommand(AP4_EditingProcessor::Command::TYPE_INSERT, system_id, &pssh_data);
            } else {
                fprintf(stderr, "ERROR: invalid format for --insert command argument\n");
                return 1;
            }
        } else if (!AP4_CompareStrings(arg, "--remove")) {
            char* system_id_hex = *++argv;
            if (system_id_hex == NULL) {
                fprintf(stderr, "ERROR: missing argument for --remove command\n");
                return 1;
            }
            unsigned char  system_id[16];
            result = AP4_ParseHex(system_id_hex, system_id, 16);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: invalid system ID (%s)\n", system_id_hex);
                return 1;
            }
            processor.AddCommand(AP4_EditingProcessor::Command::TYPE_REMOVE, system_id, NULL);
        } else if (!AP4_CompareStrings(arg, "--replace")) {
            char* param = *++argv;
            if (param == NULL) {
                fprintf(stderr, "ERROR: missing argument for --replace command\n");
                return 1;
            }
            char* system_id_hex = NULL;
            char* file_path = NULL;
            if (AP4_SUCCEEDED(AP4_SplitArgs(param, system_id_hex, file_path))) {
                if (file_path[0] == '\0') {
                    // empty file path
                    fprintf(stderr, "ERROR: missing file path in --insert argument\n");
                    return 1;
                }
                AP4_DataBuffer pssh_data;
                unsigned char  system_id[16];
                result = LoadFile(file_path, pssh_data);
                if (AP4_FAILED(result)) return 1;
                result = AP4_ParseHex(system_id_hex, system_id, 16);
                if (AP4_FAILED(result)) {
                    fprintf(stderr, "ERROR: invalid system ID (%s)\n", system_id_hex);
                    return 1;
                }
                processor.AddCommand(AP4_EditingProcessor::Command::TYPE_REPLACE, system_id, &pssh_data);
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

