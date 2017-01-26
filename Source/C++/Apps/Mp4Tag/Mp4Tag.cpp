/*****************************************************************
|
|    AP4 - MP4 File Tagger
|
|    Copyright 2002-2009 Axiomatic Systems, LLC Boccon-Gibod
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
#define BANNER "MP4 File Tagger - Version 1.2 "\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-2008 Axiomatic Systems, LLC"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
class Command 
{
public:
    // types
    typedef enum {
        TYPE_SHOW_TAGS,
        TYPE_LIST_KEYS,
        TYPE_LIST_SYMBOLS,
        TYPE_SET,
        TYPE_ADD,
        TYPE_REMOVE,
        TYPE_EXTRACT
    } Type;

    // constructors
    Command(Type        type,
            const char* arg1 = NULL,
            const char* arg2 = NULL) :
        m_Type(type),
        m_Arg1(arg1),
        m_Arg2(arg2) {}

    // members
    Type       m_Type;
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
    const char*       input_filename;
    const char*       output_filename;
    AP4_List<Command> commands;
    bool              need_input;
    bool              need_output;
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
            "commands:\n"
            "  --help            print this usage information\n"
            "  --show-tags       show tags found in the input file\n"
            "  --list-symbols    list all the builtin symbols\n"
            "  --list-keys       list all the builtin symbolic key names\n"
            "  --set <key>:<type>:<value>\n"
            "      set a tag (if the tag does not already exist, set behaves like add)\n"
            "  --add <key>:<type>:<value>\n"
            "      set/add a tag\n"
            "      where <type> is:\n"
            "        S    if <value> is a UTF-8 string\n"
            "        LS   if <value> is a UTF-8 string with a language code (see notes)\n"
            "        I8   if <value> is an 8-bit integer\n"
            "        I16  if <value> is a 16-bit integer\n"
            "        I32  if <value> is a 32-bit integer\n"
            "        JPEG if <value> is the name of a JPEG file\n"
            "        GIF  if <value> is the name of a GIF file\n"
            "        B    if <value> is a binary string (see notes)\n"
            "        Z    if <value> is the name of a builtin symbol\n"
            "  --remove <key>\n"
            "       remove a tag\n"
            "  --extract <key>:<file>\n"
            "       extract the value of a tag and save it to a file\n"
            "\n"
            "NOTES:\n"
            "  In all commands with a <key> argument, except for '--add', <key> can be \n"
            "  <key-name> or <key-name>#n where n is the zero-based index of the key when\n"
            "  there is more than one key with the same name (ex: multiple images for cover\n"
            "  art).\n"
            "  A <key-name> has the form <namespace>/<name> or simply <name> (in which case\n"
            "  the namespace defaults to 'meta').\n"
            "  The <name> of a key is either one of a symbolic keys\n"
            "  (see --list-keys) or a 4-character atom name.\n"
            "  The namespace can be 'meta' for itunes-style metadata or 'dcf' for\n"
            "  OMA-DCF-style metadata, or a user-defined long-form namespace\n"
            "  (ex: com.mycompany.foo).\n"
            "  Binary strings can be expressed as a normal string of ASCII characters\n"
            "  prefixed by a + character if all the bytes fall in the ASCII range,\n"
            "  or hex-encoded prefixed by a # character (ex: +hello, or #0FC4)\n"
            "  Strings with a language code are expressed as: <lang>:<string>,\n"
            "  where <lang> is a 3 character language code (ex: eng:hello)\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ParseCommandLine
+---------------------------------------------------------------------*/
static void
ParseCommandLine(int argc, char** argv) 
{
    for (int i=0; i<argc; i++) {
        if (AP4_CompareStrings("--help", argv[i]) == 0) {
        PrintUsageAndExit();
        } else if (AP4_CompareStrings("--show-tags", argv[i]) == 0) {
            Options.commands.Add(new Command(Command::TYPE_SHOW_TAGS));
            Options.need_input = true;
        } else if (AP4_CompareStrings("--list-keys", argv[i]) == 0) {
            Options.commands.Add(new Command(Command::TYPE_LIST_KEYS));
        } else if (AP4_CompareStrings("--list-symbols", argv[i]) == 0) {
            Options.commands.Add(new Command(Command::TYPE_LIST_SYMBOLS));
        } else if (AP4_CompareStrings("--add", argv[i]) == 0) {
            if (i == argc-1) {
                fprintf(stderr, "ERROR: missing argument after --add command");
                PrintUsageAndExit();
            }
            Options.commands.Add(new Command(Command::TYPE_ADD, argv[++i]));
            Options.need_input  = true;
            Options.need_output = true;
        } else if (AP4_CompareStrings("--set", argv[i]) == 0) {
            if (i == argc-1) {
                fprintf(stderr, "ERROR: missing argument after --set command");
                PrintUsageAndExit();
            }
            Options.commands.Add(new Command(Command::TYPE_SET, argv[++i]));
            Options.need_input  = true;
            Options.need_output = true;
        } else if (AP4_CompareStrings("--remove", argv[i]) == 0) {
            if (i == argc-1) {
                fprintf(stderr, "ERROR: missing argument after --remove command");
                PrintUsageAndExit();
            }
            Options.commands.Add(new Command(Command::TYPE_REMOVE, argv[++i]));
            Options.need_input  = true;
            Options.need_output = true;
        } else if (AP4_CompareStrings("--extract", argv[i]) == 0) {
            if (i == argc-1) {
                fprintf(stderr, "ERROR: missing argument after --extract command");
                PrintUsageAndExit();
            }
            Options.commands.Add(new Command(Command::TYPE_EXTRACT, argv[++i]));
            Options.need_input  = true;
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

    if (Options.commands.ItemCount() == 0) {
        if (Options.input_filename) {
            Options.commands.Add(new Command(Command::TYPE_SHOW_TAGS));
            Options.need_input = true;
        } else {
            PrintUsageAndExit();
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
        case AP4_MetaData::Value::TYPE_BINARY:         return "B";
        case AP4_MetaData::Value::TYPE_STRING_UTF_8:   return "S";
        case AP4_MetaData::Value::TYPE_STRING_UTF_16:  return "W";
        case AP4_MetaData::Value::TYPE_INT_08_BE:      return "I8";
        case AP4_MetaData::Value::TYPE_INT_16_BE:      return "I16";
        case AP4_MetaData::Value::TYPE_INT_32_BE:      return "I32";
        case AP4_MetaData::Value::TYPE_FLOAT_32_BE:    return "F32";
        case AP4_MetaData::Value::TYPE_FLOAT_64_BE:    return "F64";
        case AP4_MetaData::Value::TYPE_JPEG:           return "JPEG";
        case AP4_MetaData::Value::TYPE_GIF:            return "GIF";
        default: return "?";
    }
}

/*----------------------------------------------------------------------
|   MeaningCode
+---------------------------------------------------------------------*/
static const char*
MeaningCode(AP4_MetaData::Value::Meaning meaning) {
    switch (meaning) {
        case AP4_MetaData::Value::MEANING_BOOLEAN:    return "/Bool";
        case AP4_MetaData::Value::MEANING_ID3_GENRE:  return "/ID3";
        default: return "";
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
    for (AP4_Ordinal i=0; i<AP4_MetaData::KeyInfos.ItemCount(); i++) {
        AP4_MetaData::KeyInfo& key = AP4_MetaData::KeyInfos[i];
        unsigned int key_width = (unsigned int)AP4_StringLength(key.name);
        if (key_width > c1_width) c1_width = key_width;
    }
    c1_width += 2; // account for padding
    unsigned int c3_width = 4;
    unsigned int c2_width = LINE_WIDTH-c1_width-c3_width-1-8;
    TableCell cells[4] = {
        {" Key",         c1_width},
        {" Description", c2_width},
        {" 4CC",         4       },
        {"Type",         4       }
    };
    PrintTableSeparator(cells, 4);
    PrintTableRow(cells, 4);
    PrintTableSeparator(cells, 4);

    char four_cc[5];
    for (AP4_Ordinal i=0; i<AP4_MetaData::KeyInfos.ItemCount(); i++) {
        AP4_MetaData::KeyInfo& key = AP4_MetaData::KeyInfos[i];
        AP4_FormatFourCharsPrintable(four_cc, key.four_cc);
        cells[0].label = key.name;
        cells[1].label = key.description;
        cells[2].label = four_cc;
        cells[3].label = TypeCode(key.value_type);
        PrintTableRow(cells, 4);
    }
    PrintTableSeparator(cells, 4);
    
    printf("\nDCF Special 4CC types:\n");
    printf("dcfD\n\n");
    printf("DCF String 4CC types:\n");
    const char* sep = "";
    for (unsigned int i=0; i<AP4_MetaDataAtomTypeHandler::DcfStringTypeList.m_Size; i++) {
        AP4_FormatFourCharsPrintable(four_cc, AP4_MetaDataAtomTypeHandler::DcfStringTypeList.m_Types[i]);
        printf("%s%s", sep, four_cc);
        sep = ", ";
    }
    printf("\n\n3GPP Localized String 4CC types:\n");
    sep = "";
    for (unsigned int i=0; i<AP4_MetaDataAtomTypeHandler::_3gppLocalizedStringTypeList.m_Size; i++) {
        AP4_FormatFourCharsPrintable(four_cc, AP4_MetaDataAtomTypeHandler::_3gppLocalizedStringTypeList.m_Types[i]);
        printf("%s%s", sep, four_cc);
        sep = ", ";
    }
    printf("\n\n3GPP Other 4CC types:\n");
    sep = "";
    for (unsigned int i=0; i<AP4_MetaDataAtomTypeHandler::_3gppOtherTypeList.m_Size; i++) {
        AP4_FormatFourCharsPrintable(four_cc, AP4_MetaDataAtomTypeHandler::_3gppOtherTypeList.m_Types[i]);
        printf("%s%s", sep, four_cc);
        sep = ", ";
    }
    printf("\n");
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
    // look for a match in the key infos
    const char* key_name = entry->m_Key.GetName().GetChars();
    bool        show_namespace = true;
    AP4_Atom::Type atom_type = AP4_Atom::TypeFromString(entry->m_Key.GetName().GetChars());
    for (unsigned int i=0; i<AP4_MetaData::KeyInfos.ItemCount(); ++i) {
        if (AP4_MetaData::KeyInfos[i].four_cc == atom_type) {
            key_name = AP4_MetaData::KeyInfos[i].name;
            show_namespace = false;
            break;
        }
    }

    // print the key namespace unless we have a symbolic name
    if (show_namespace) {
        printf("%s/", entry->m_Key.GetNamespace().GetChars());
    }

    // print the key name
    printf("%s:", key_name);

    // print the value type
    printf("[%s%s]",
           TypeCode(entry->m_Value->GetType()),
           MeaningCode(entry->m_Value->GetMeaning()));
    
    // print the language, unless it is not set
    if (entry->m_Value->GetLanguage().GetLength()) {
        printf(" (%s)", entry->m_Value->GetLanguage().GetChars());
    }
    
    // print the value 
    printf(" %s", entry->m_Value->ToString().GetChars());

    printf("\n");
}

/*----------------------------------------------------------------------
|   ShowTags
+---------------------------------------------------------------------*/
static void
ShowTags(AP4_File* file) 
{
    const AP4_MetaData* metadata = file->GetMetaData();
    
    // iterator over the entries
    AP4_List<AP4_MetaData::Entry>::Item* item = metadata->GetEntries().FirstItem();
    while (item) {
        AP4_MetaData::Entry* entry = item->GetData();
        ShowTag(entry);
        item = item->GetNext();
    }
}

/*----------------------------------------------------------------------
|   SplitString
+---------------------------------------------------------------------*/
static AP4_Result
SplitString(const AP4_String&     arg, 
            char                  separator,
            AP4_List<AP4_String>& components, 
            unsigned int          count)
{
    AP4_String* component;
    int start=0, end;
    for (unsigned int i=0; i<count; i++) {
        end = i==count-1?(int)arg.GetLength():arg.Find(separator, start);
        if (end == -1) return AP4_FAILURE;
        component = new AP4_String();
        component->Assign(arg.GetChars()+start, end-start);
        components.Add(component);
        start = end+1;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   ParseKeySpec
+---------------------------------------------------------------------*/
static AP4_Result
ParseKeySpec(AP4_String&  key, 
             AP4_String*& key_namespace, 
             AP4_String*& key_name,
             AP4_Ordinal& key_index)
{
    key_namespace = NULL;
    key_name = NULL;
    key_index = 0;
    
    // deal with the key index specifier
    AP4_String processed_key(key);
    for (int i=key.GetLength()-1; i>=0; --i) {
        if (key[i] == '#') {
            processed_key.Assign(key.GetChars(), i);
            key_index = (AP4_Ordinal)strtoul(key.GetChars()+i+1, NULL, 10);
        }
    }
    
    if (processed_key.Find('/') != -1) {
        // the key is in the form namespace/name
        AP4_List<AP4_String> key_components;
        SplitString(processed_key, '/', key_components, 2);
        key_components.Get(0, key_namespace);
        key_components.Get(1, key_name);
    } else {
        // assume the namespace is 'meta'
        key_namespace = new AP4_String("meta");
        key_name      = new AP4_String(processed_key);
    }    

    // look for the name in the list of keys
    for (AP4_Ordinal i=0; i<AP4_MetaData::KeyInfos.ItemCount(); i++) {
        AP4_MetaData::KeyInfo& key_info = AP4_MetaData::KeyInfos[i];
        if (*key_name == key_info.name) {
            char four_cc[5];
            four_cc[0] = key_info.four_cc>>24;
            four_cc[1] = key_info.four_cc>>16;
            four_cc[2] = key_info.four_cc>> 8;
            four_cc[3] = key_info.four_cc    ;
            four_cc[4] = '\0';
            delete key_name;
            key_name = new AP4_String(four_cc);
            break;
        }
    }
    if (key_name == NULL) {
        fprintf(stderr, "ERROR: unknown key\n");
        delete key_namespace;
        key_namespace = NULL;
        return AP4_FAILURE;
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetNibble
+---------------------------------------------------------------------*/
static unsigned int
GetNibble(char c)
{
    switch (c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a':
        case 'A': return 0xA;
        case 'b':
        case 'B': return 0xB;
        case 'c':
        case 'C': return 0xC;
        case 'd':
        case 'D': return 0xD;
        case 'e':
        case 'E': return 0xE;
        case 'f':
        case 'F': return 0xF;
        default: return 0;
    }
}

/*----------------------------------------------------------------------
|   RemoveTag
+---------------------------------------------------------------------*/
static AP4_Result
RemoveTag(AP4_File* file, AP4_String& key, bool silent) 
{
    AP4_String* key_namespace;
    AP4_String* key_name;
    AP4_Ordinal key_index;
    AP4_Result result = ParseKeySpec(key, key_namespace, key_name, key_index);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: invalid key name (%s)\n", key.GetChars());
        return AP4_ERROR_INVALID_PARAMETERS;
    }
    
    AP4_MetaData::Entry entry(key_name->GetChars(), key_namespace->GetChars(), NULL);
    result = entry.RemoveFromFile(*file, key_index);
    if (AP4_FAILED(result) && !silent) {
        fprintf(stderr, "ERROR: failed to remove entry (%d)\n", result);
    }

    delete key_namespace;
    delete key_name;
    
    return result;
}

/*----------------------------------------------------------------------
|   AddTag
+---------------------------------------------------------------------*/
static AP4_Result
AddTag(AP4_File* file, AP4_String& arg, bool remove_first) 
{
    AP4_Result result;
    
    AP4_List<AP4_String> arg_components;
    result = SplitString(arg, ':', arg_components, 3);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: invalid argument syntax for --add command\n");
        return result;
    }
    
    // split the components
    AP4_String* key   = NULL;
    AP4_String* type  = NULL;
    AP4_String* value = NULL;
    arg_components.Get(0, key);
    arg_components.Get(1, type);
    arg_components.Get(2, value);
    
    AP4_String* key_namespace = NULL;
    AP4_String* key_name = NULL;
    AP4_Ordinal key_index = 0;
    result = ParseKeySpec(*key, key_namespace, key_name, key_index);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: invalid key name (%s)\n", key->GetChars());
        delete key;
        delete type;
        delete value;
        return result;
    }
        
    // remove the entry first if necessary
    if (remove_first) {
        RemoveTag(file, *key, true);
    }
    
    // create an entry
    AP4_MetaData::Value* vobj = NULL;
    if (*type == "S") {
        vobj = new AP4_StringMetaDataValue(value->GetChars());
    } else if (*type == "LS") {
        char lang[4];
        if (value->GetLength() < 4 || (*value)[3] != ':') {
            fprintf(stderr, "ERROR: invalid syntax for LS value\n");
            result = AP4_ERROR_INVALID_FORMAT;
            goto end;
        }
        lang[0] = (*value)[0];
        lang[1] = (*value)[1];
        lang[2] = (*value)[2];
        lang[3] = 0;
        vobj = new AP4_StringMetaDataValue(value->GetChars()+4);
        vobj->SetLanguage(lang);
    } else if (*type == "I8") {
        long v = strtol(value->GetChars(), NULL, 10);
        vobj = new AP4_IntegerMetaDataValue(AP4_MetaData::Value::TYPE_INT_08_BE, v);
    } else if (*type == "I16") {
        long v = strtol(value->GetChars(), NULL, 10);
        vobj = new AP4_IntegerMetaDataValue(AP4_MetaData::Value::TYPE_INT_16_BE, v);
    } else if (*type == "I32") {
        long v = strtol(value->GetChars(), NULL, 10);
        vobj = new AP4_IntegerMetaDataValue(AP4_MetaData::Value::TYPE_INT_32_BE, v);
    } else if (*type == "JPEG" || *type == "GIF") {
        AP4_MetaData::Value::Type data_type = 
            (*type == "JPEG" ? AP4_MetaData::Value::TYPE_JPEG : AP4_MetaData::Value::TYPE_GIF);
        AP4_ByteStream* data_file = NULL;
        result = AP4_FileByteStream::Create(value->GetChars(), AP4_FileByteStream::STREAM_MODE_READ, data_file);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot open file %s\n", value->GetChars());
            goto end;
        } 
        AP4_DataBuffer buffer;
        AP4_LargeSize  data_size;
        data_file->GetSize(data_size);
        buffer.SetDataSize((AP4_Size)data_size);
        data_file->Read(buffer.UseData(), (AP4_Size)data_size);
        vobj = new AP4_BinaryMetaDataValue(data_type, buffer.GetData(), buffer.GetDataSize());
        if (data_file) data_file->Release();
    } else if (*type == "B") {
        if (value->GetLength() == 0) {
            fprintf(stderr, "ERROR: invalid binary encoding\n");
            result = AP4_ERROR_INVALID_PARAMETERS;
            goto end;
        } else if (value->GetChars()[0] == '+') {
            vobj = new AP4_BinaryMetaDataValue(AP4_MetaData::Value::TYPE_BINARY, 
                                               (const AP4_UI08*)value->GetChars()+1, 
                                               value->GetLength()-1);
        } else if (value->GetChars()[0] == '#') {
            if (((value->GetLength()-1)%2) != 0) {
                fprintf(stderr, "ERROR: invalid hex encoding\n");
                result = AP4_ERROR_INVALID_PARAMETERS;
                goto end;
            } else {
                AP4_Size  binary_string_length = (value->GetLength()-1)/2;
                AP4_UI08* binary_string = new AP4_UI08[binary_string_length];
                for (unsigned int i=0; i<binary_string_length; i++) {
                    unsigned int nibble_0 = GetNibble(value->GetChars()[1+i*2  ]);
                    unsigned int nibble_1 = GetNibble(value->GetChars()[1+i*2+1]);
                    binary_string[i] = (nibble_0<<4)|nibble_1;
                }
                vobj = new AP4_BinaryMetaDataValue(AP4_MetaData::Value::TYPE_BINARY, 
                                                   binary_string, 
                                                   binary_string_length);
            }
        } else {
            fprintf(stderr, "ERROR: invalid binary string (does not start with + or #)\n");
            result = AP4_ERROR_INVALID_PARAMETERS;
            goto end;
        }
    } else {
        fprintf(stderr, "ERROR: unsupported type '%s'\n", type->GetChars());
        result = AP4_ERROR_INVALID_PARAMETERS;
        goto end;
    }
    {
        AP4_MetaData::Entry entry(key_name->GetChars(), key_namespace->GetChars(), vobj);
    
        // add the entry to the file
        result = entry.AddToFile(*file, key_index);
        if (AP4_FAILED(result)) {
            fprintf(stderr, 
                    "ERROR: cannot add entry %s (%d:%s)\n", 
                    key->GetChars(),
                    result, AP4_ResultText(result));
        }
    }
    
end:
    delete key;
    delete key_namespace;
    delete key_name;
    delete type;
    delete value;

    return result;
}

/*----------------------------------------------------------------------
|   ExtractTag
+---------------------------------------------------------------------*/
static AP4_Result
ExtractTag(AP4_File* file, AP4_String& arg) 
{
    AP4_List<AP4_String> arg_components;
    AP4_Result result = SplitString(arg, ':', arg_components, 2);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: invalid argument syntax for --extract command\n");
        return result;
    }
    
    // split the components
    AP4_String* key = NULL;
    AP4_String* output_filename = NULL;
    arg_components.Get(0, key);
    arg_components.Get(1, output_filename);
    
    AP4_String* key_namespace = NULL;
    AP4_String* key_name = NULL;
    AP4_Ordinal key_index;
    result = ParseKeySpec(*key, key_namespace, key_name, key_index);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: invalid key name (%s)\n", key->GetChars());
        delete key;
        delete output_filename;
        return result;
    }

    // get the metadata
    const AP4_MetaData* metadata = file->GetMetaData();
    
    // iterate over the entries and look for a match
    bool found = false;
    AP4_List<AP4_MetaData::Entry>::Item* item = NULL; // we init the  var here to work around a gcc/mingw bug
    for (item = metadata->GetEntries().FirstItem();
         item;
         item = item->GetNext()) {
        AP4_MetaData::Entry* entry = item->GetData();
        if (entry->m_Key.GetNamespace() == *key_namespace &&
            entry->m_Key.GetName()      == *key_name      &&
            key_index--                 == 0) {
            // match
            if (entry->m_Value == NULL) break;
            AP4_DataBuffer buffer;
            if (AP4_FAILED(entry->m_Value->ToBytes(buffer))) break;
            found = true;
            AP4_ByteStream* output = NULL;
            result = AP4_FileByteStream::Create(output_filename->GetChars(), AP4_FileByteStream::STREAM_MODE_WRITE, output);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: cannot open output/extract file\n");
                goto end;
            }
            output->Write(buffer.GetData(), buffer.GetDataSize());
            output->Release();
        }
    }
    if (!found) {
        fprintf(stderr, "ERROR: cannot extract tag, entry not found\n");
        result = AP4_ERROR_NO_SUCH_ITEM;
    }
    
end:
    delete key;
    delete key_namespace;
    delete key_name;
    delete output_filename;
    
    return result;
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
    Options.need_input      = false;
    Options.need_output     = false;

    // parse command line
    ParseCommandLine(argc-1, argv+1);

    // check options
    if (Options.need_input) {
        if (Options.input_filename == NULL) {
            fprintf(stderr, "ERROR: input file name missing\n");
            PrintUsageAndExit();
        }
    } else {
        if (Options.input_filename != NULL) {
            fprintf(stderr, "ERROR: unexpected input file name\n");
            PrintUsageAndExit();
        }
    }
    if (Options.need_output) {
        if (Options.output_filename == NULL) {
            fprintf(stderr, "ERROR: output file name missing\n");
            PrintUsageAndExit();
        }
    } else {
        if (Options.output_filename != NULL) {
            fprintf(stderr, "ERROR: unexpected output file name\n");
            PrintUsageAndExit();
        }
    }

    AP4_ByteStream* input     = NULL;
    AP4_File*       file      = NULL;
    AP4_Movie*      movie     = NULL;
    AP4_MoovAtom*   moov      = NULL;
    AP4_LargeSize   moov_size = 0;
    AP4_Result      result    = AP4_SUCCESS;
    if (Options.need_input) {
        result =AP4_FileByteStream::Create(Options.input_filename, AP4_FileByteStream::STREAM_MODE_READ, input);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot open input file\n");
            return 1;
        }
        file = new AP4_File(*input);
        
        // remember the size of the moov atom
        movie = file->GetMovie();
        if (movie) {
            moov = movie->GetMoovAtom();
            if (moov) {
                moov_size = moov->GetSize();
            }
        }
    }

    AP4_ByteStream* output = NULL;
    if (Options.need_output) {
        result = AP4_FileByteStream::Create(Options.output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
        if (AP4_FAILED(result)) {
            fprintf(stderr, "ERROR: cannot open output file for writing\n");
            return 1;
        }
    }

    for (AP4_List<Command>::Item* item = Options.commands.FirstItem();
         item;
         item = item->GetNext()) {
        result = AP4_SUCCESS;
        switch (item->GetData()->m_Type) {
            case Command::TYPE_LIST_KEYS:
                ListKeys();
                break;

            case Command::TYPE_LIST_SYMBOLS:
                ListSymbols();
                break;

            case Command::TYPE_SHOW_TAGS:
                ShowTags(file);
                break;

            case Command::TYPE_SET:
                result = AddTag(file, item->GetData()->m_Arg1, true);
                break;

            case Command::TYPE_ADD:
                result = AddTag(file, item->GetData()->m_Arg1, false);
                break;

            case Command::TYPE_REMOVE:
                result = RemoveTag(file, item->GetData()->m_Arg1, false);
                break;

            case Command::TYPE_EXTRACT:
                result = ExtractTag(file, item->GetData()->m_Arg1);
                break;

            default:
                break;
        }
        
        if (AP4_FAILED(result)) goto end;
    }

    if (output) {
        // adjust the chunk offsets if the moov is before the mdat
        if (moov && file->IsMoovBeforeMdat()) {
            AP4_LargeSize new_moov_size = moov->GetSize();
            AP4_SI64 size_diff = new_moov_size-moov_size;
            if (size_diff) {
                moov->AdjustChunkOffsets(size_diff);
            }
        }
        
        // write the modified file
        AP4_FileCopier::Write(*file, *output);
    }
    
end:
    delete file;
    delete input;
    delete output;
    Options.commands.DeleteReferences();

    return result;
}
