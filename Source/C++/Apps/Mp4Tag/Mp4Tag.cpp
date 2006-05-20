/*****************************************************************
|
|    AP4 - MP4 File Tagger
|
|    Copyright 2002-2003 Gilles Boccon-Gibod
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
#define BANNER "MP4 File Tagger - Version 0.2a "\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2005 Gilles Boccon-Gibod & Julien Boeuf"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
class AP4_TaggerCommand 
{
public:
    // types
    typedef enum {
        TYPE_SET,
        TYPE_UNSET,
        TYPE_EXTRACT
    } Type;

    // constructor
    AP4_TaggerCommand(Type        type,
                      const char* key, 
                      const char* arg1 = NULL,
                      const char* arg2 = NULL) :
        m_Type(type),
        m_Key(key),
        m_Arg1(arg1),
        m_Arg2(arg2) {}

    // members
    Type       m_Type;
    AP4_String m_Key;
    AP4_String m_Arg1;
    AP4_String m_Arg2;
};

typedef struct {
    const char*  label;
    unsigned int width;
} TableCell;

/*----------------------------------------------------------------------
|   options
+---------------------------------------------------------------------*/
static struct {
    const char*                 input_filename;
    const char*                 output_filename;
    bool                        show_tags;
    bool                        list_symbols;
    bool                        list_keys;
    AP4_List<AP4_TaggerCommand> command;
} Options;

static const int LINE_WIDTH = 79;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4tag [options] [commands...] <input> [<output>]\n"
            "options:\n"
            "  --help            print this usage information\n"
            "  --show-tags       show tags found in the input file\n"
            "  --list-symbols    list all the builtin symbols\n"
            "  --list-keys       list all builtin keys\n"
            "commands:\n"
            "  --set <key>:<encoding>:<value>\n"
            "  --add <key>:<encoding>:<value>\n"
            "      set/add a tag (if the tag does not already exist, set behaves like add)\n"
            "      where <encoding> is:\n"
            "        s if <value> is a direct string\n"
            "        i if <value> is a decimal integer\n"
            "        f if <value> is a filename\n"
            "        z if <value> is a the name of a builtin  symbol\n"
            "  --remove <key>\n"
            "       remove a tag\n"
            "  --extract <key>:<file>\n"
            "       extract the value of a tag and save it to a file\n"
            "\n"
            "  in all commands, <key> can be a key name, or <key_name#n> where n is\n"
            "  the index of the key when there is more than one key with the same name\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ParseCommandLine
+---------------------------------------------------------------------*/
static void
ParseCommandLine(int argc, char** argv) 
{
    for (int i=0; i<argc; i++) {
        if (AP4_CompareStrings("--show-tags", argv[i]) == 0) {
            Options.show_tags = true;
        } else if (AP4_CompareStrings("--help", argv[i]) == 0) {
            PrintUsageAndExit();
        } else if (AP4_CompareStrings("--list-keys", argv[i]) == 0) {
            Options.list_keys = true;
        } else if (AP4_CompareStrings("--list-symbols", argv[i]) == 0) {
            Options.list_symbols = true;
        } else {
            if (Options.input_filename == NULL) {
                Options.input_filename = argv[i];
            } else if (Options.output_filename == NULL) {
                Options.output_filename = argv[i];
            } else {
                fprintf(stderr, "ERROR: unpexpected option \"%s\"\n", argv[i]);
                PrintUsageAndExit();
            }
        }
    }
}

/*----------------------------------------------------------------------
|   PrintTableSeparator
+---------------------------------------------------------------------*/
static void
PrintTableSeparator(TableCell* cells, AP4_Cardinal cell_count)
{
    while (cell_count--) {
        printf("+");
        for (unsigned int i=0; i<cells->width; i++) {
            printf("-");
        }
        cells++;
    }
    printf("+\n");
}

/*----------------------------------------------------------------------
|   PrintTableRow
+---------------------------------------------------------------------*/
static void
PrintTableRow(TableCell* cells, AP4_Cardinal cell_count)
{
    while (cell_count--) {
        printf("|");
        printf("%s", cells->label);
        for (unsigned int x=(unsigned int)AP4_StringLength(cells->label); x<cells->width; x++) {
            printf(" ");
        }
        cells++;
    }
    printf("|\n");
}

/*----------------------------------------------------------------------
|   TypeCode
+---------------------------------------------------------------------*/
static const char*
TypeCode(AP4_MetaData::Value::Type type) {
    switch (type) {
        case AP4_MetaData::Value::TYPE_BINARY:  return "B";
        case AP4_MetaData::Value::TYPE_STRING:  return "S";
        case AP4_MetaData::Value::TYPE_INTEGER: return "I";
        default: return "?";
    }
}

/*----------------------------------------------------------------------
|   ListKeys
+---------------------------------------------------------------------*/
static void
ListKeys()
{
    // measure the widest key
    unsigned int c1_width = 0;
    for (AP4_Ordinal i=0; i<AP4_MetaData::KeysInfos.ItemCount(); i++) {
        AP4_MetaData::KeyInfo& key = AP4_MetaData::KeysInfos[i];
        unsigned int key_width = (unsigned int)AP4_StringLength(key.name);
        if (key_width > c1_width) c1_width = key_width;
    }
    c1_width += 2; // account for padding
    int c3_width = 4;
    int c2_width = LINE_WIDTH-c1_width-c3_width-1-5;
    TableCell cells[4] = {
        {" Key",         c1_width},
        {" Description", c2_width},
        {" 4CC",         4       },
        {"T",            1       }
    };
    PrintTableSeparator(cells, 4);
    PrintTableRow(cells, 4);
    PrintTableSeparator(cells, 4);

    for (AP4_Ordinal i=0; i<AP4_MetaData::KeysInfos.ItemCount(); i++) {
        AP4_MetaData::KeyInfo& key = AP4_MetaData::KeysInfos[i];
        char four_cc[5];
        AP4_FormatFourCharsPrintable(four_cc, key.four_cc);
        cells[0].label = key.name;
        cells[1].label = key.description;
        cells[2].label = four_cc;
        cells[3].label = TypeCode(key.value_type);
        PrintTableRow(cells, 4);
    }
    PrintTableSeparator(cells, 4);
}

/*----------------------------------------------------------------------
|   ListSymbols
+---------------------------------------------------------------------*/
static void
ListSymbols()
{

}

/*----------------------------------------------------------------------
|   ShowTag
+---------------------------------------------------------------------*/
static void
ShowTag(AP4_MetaData::Entry* entry)
{
    // print the key namespace unless it is empty
    if (entry->m_Key.GetNamespace()[0] != '\0') {
        printf("%s/", entry->m_Key.GetNamespace());
    }

    // print the key name
    printf("%s:", entry->m_Key.GetName());

    // print the value type
    printf("[%s] %s", 
           TypeCode(entry->m_Value->GetType()),
           entry->m_Value->ToString().GetChars());
    printf("\n");
}

/*----------------------------------------------------------------------
|   ShowTags
+---------------------------------------------------------------------*/
static void
ShowTags(AP4_File* file) 
{
    // get the file's movie
    AP4_Movie* movie = file->GetMovie();
    if (movie == NULL) return;

    // get the movie's metadata
    const AP4_MetaData* metadata = movie->GetMetaData();
    if (metadata == NULL) return;

    // iterator over the entries
    AP4_List<AP4_MetaData::Entry>::Item* item = metadata->GetEntries().FirstItem();
    while (item) {
        AP4_MetaData::Entry* entry = item->GetData();
        ShowTag(entry);
        item = item->GetNext();
    }
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
    
    // init options
    Options.input_filename  = NULL;
    Options.output_filename = NULL;
    Options.show_tags       = false;
    Options.list_symbols    = false;
    Options.list_keys       = false;

    // parse command line
    ParseCommandLine(argc-1, argv+1);

    // process general options first
    if (Options.list_keys) {
        ListKeys();
    }
    if (Options.list_symbols) {
        ListSymbols();
    }

    // check options
    if (Options.input_filename == NULL &&
        !Options.list_keys &&
        !Options.list_symbols) {
        fprintf(stderr, "ERROR: input file name missing\n");
        PrintUsageAndExit();
    }
    //if (Options.output_filename != NULL) {
    //    fprintf(stderr, "ERROR: unexpected output file name\n");
    //    PrintUsageAndExit();
    //}

    AP4_ByteStream* input;
    try {
        input = new AP4_FileByteStream(Options.input_filename,
                                       AP4_FileByteStream::STREAM_MODE_READ);
    } catch (AP4_Exception&) {
        fprintf(stderr, "ERROR: cannot open input file\n");
        return 1;
    }

    AP4_ByteStream* output = NULL;
    //    new AP4_FileByteStream(argv[2],
    //                           AP4_FileByteStream::STREAM_MODE_WRITE);

    AP4_File* file = new AP4_File(*input);

    // show the tags if required
    if (Options.show_tags) {
        ShowTags(file);
    }

    //file->Write(output);
    
    delete file;
    delete input;
    delete output;
    Options.command.DeleteReferences();

    return 0;
}
