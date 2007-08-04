/*****************************************************************
|
|    AP4 - MetaData 
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf
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
#include "Ap4File.h"
#include "Ap4Movie.h"
#include "Ap4MetaData.h"
#include "Ap4ContainerAtom.h"
#include "Ap4MoovAtom.h"
#include "Ap4HdlrAtom.h"
#include "Ap4DataBuffer.h"
#include "Ap4Utils.h"
#include "Ap4String.h"

/*----------------------------------------------------------------------
|   metadata keys
+---------------------------------------------------------------------*/
static const AP4_MetaData::KeyInfo AP4_MetaData_KeyInfos [] = {
    {"Name",        "Name",        AP4_ATOM_TYPE_cNAM, AP4_MetaData::Value::TYPE_STRING},
    {"Artist",      "Artist",      AP4_ATOM_TYPE_cART, AP4_MetaData::Value::TYPE_STRING},
    {"Composer",    "Composer",    AP4_ATOM_TYPE_cCOM, AP4_MetaData::Value::TYPE_STRING},
    {"Writer",      "Writer",      AP4_ATOM_TYPE_cWRT, AP4_MetaData::Value::TYPE_STRING},
    {"Album",       "Album",       AP4_ATOM_TYPE_cALB, AP4_MetaData::Value::TYPE_STRING},
    {"Genre",       "Genre",       AP4_ATOM_TYPE_GNRE, AP4_MetaData::Value::TYPE_INTEGER},
    {"Genre2",      "Genre",       AP4_ATOM_TYPE_cGEN, AP4_MetaData::Value::TYPE_INTEGER},
    {"Grouping",    "Grouping",    AP4_ATOM_TYPE_cGRP, AP4_MetaData::Value::TYPE_STRING},
    {"Date",        "Date",        AP4_ATOM_TYPE_cDAY, AP4_MetaData::Value::TYPE_STRING},
    {"Tool",        "Tool",        AP4_ATOM_TYPE_cTOO, AP4_MetaData::Value::TYPE_STRING},
    {"Comment",     "Comment",     AP4_ATOM_TYPE_cCMT, AP4_MetaData::Value::TYPE_STRING},
    {"Lyrics",      "Lyrics",      AP4_ATOM_TYPE_cCMT, AP4_MetaData::Value::TYPE_STRING},
    {"Copyright",   "Copyright",   AP4_ATOM_TYPE_CPRT, AP4_MetaData::Value::TYPE_STRING},
    {"Track",       "Track Number",AP4_ATOM_TYPE_TRKN, AP4_MetaData::Value::TYPE_INTEGER},
    {"Disc",        "Disc Number", AP4_ATOM_TYPE_DISK, AP4_MetaData::Value::TYPE_INTEGER},
    {"Cover",       "Cover Art",   AP4_ATOM_TYPE_COVR, AP4_MetaData::Value::TYPE_BINARY},
    {"Description", "Description", AP4_ATOM_TYPE_DESC, AP4_MetaData::Value::TYPE_STRING},
    {"Rating",      "Rating",      AP4_ATOM_TYPE_RTNG, AP4_MetaData::Value::TYPE_INTEGER},
    {"Compilation", "Compilation", AP4_ATOM_TYPE_CPIL, AP4_MetaData::Value::TYPE_INTEGER},
    {"Gapless",     "Gapless",     AP4_ATOM_TYPE_PGAP, AP4_MetaData::Value::TYPE_INTEGER},
    {"Title",       "Title",       AP4_ATOM_TYPE_TITL, AP4_MetaData::Value::TYPE_STRING},
    {"Description", "Description", AP4_ATOM_TYPE_DSCP, AP4_MetaData::Value::TYPE_STRING}
};
AP4_Array<AP4_MetaData::KeyInfo> AP4_MetaData::KeysInfos(
    AP4_MetaData_KeyInfos, 
    sizeof(AP4_MetaData_KeyInfos)/sizeof(KeyInfo));

/*----------------------------------------------------------------------
|   genre IDs
+---------------------------------------------------------------------*/
static const char* const Ap4Id3Genres[] = 
{
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychadelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
    "Folk",
    "Folk-Rock",
    "National Folk",
    "Swing",
    "Fast Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",
    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",
    "Satire",
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",
    "Punk Rock",
    "Drum Solo",
    "Acapella",
    "Euro-House",
    "Dance Hall"
};

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_Size AP4_DATA_ATOM_MAX_SIZE = 0x40000000;

/*----------------------------------------------------------------------
|   AP4_MetaDataAtomTypeHandler::IsInTypeList
+---------------------------------------------------------------------*/
const AP4_Atom::Type AP4_MetaDataAtomTypeHandler::_3gppTypes[] = {
    AP4_ATOM_TYPE_TITL,
    AP4_ATOM_TYPE_DSCP,
    AP4_ATOM_TYPE_CPRT,
    AP4_ATOM_TYPE_PERF,
    AP4_ATOM_TYPE_AUTH,
    AP4_ATOM_TYPE_GNRE,
    AP4_ATOM_TYPE_RTNG,
    AP4_ATOM_TYPE_CLSF,
    AP4_ATOM_TYPE_KYWD,
    AP4_ATOM_TYPE_LOCI,
    AP4_ATOM_TYPE_ALBM,
    AP4_ATOM_TYPE_YRRC,
    AP4_ATOM_TYPE_TSEL
};
const AP4_MetaDataAtomTypeHandler::TypeList AP4_MetaDataAtomTypeHandler::_3gppTypeList = {
    _3gppTypes,
    sizeof(_3gppTypes)/sizeof(_3gppTypes[0])
};

/*----------------------------------------------------------------------
|   atom type lists
+---------------------------------------------------------------------*/
const AP4_Atom::Type AP4_MetaDataAtomTypeHandler::IlstTypes[] = 
{
    AP4_ATOM_TYPE_dddd,
    AP4_ATOM_TYPE_cNAM,
    AP4_ATOM_TYPE_cART,
    AP4_ATOM_TYPE_cCOM,
    AP4_ATOM_TYPE_cWRT,
    AP4_ATOM_TYPE_cALB,
    AP4_ATOM_TYPE_cGEN,
    AP4_ATOM_TYPE_cGRP,
    AP4_ATOM_TYPE_cDAY,
    AP4_ATOM_TYPE_cTOO,
    AP4_ATOM_TYPE_cCMT,
    AP4_ATOM_TYPE_CPRT,
    AP4_ATOM_TYPE_TRKN,
    AP4_ATOM_TYPE_DISK,
    AP4_ATOM_TYPE_COVR,
    AP4_ATOM_TYPE_DESC,
    AP4_ATOM_TYPE_GNRE,
    AP4_ATOM_TYPE_CPIL,
    AP4_ATOM_TYPE_TMPO,
    AP4_ATOM_TYPE_RTNG,
    AP4_ATOM_TYPE_apID,
    AP4_ATOM_TYPE_cnID,
    AP4_ATOM_TYPE_atID,
    AP4_ATOM_TYPE_plID,
    AP4_ATOM_TYPE_geID,
    AP4_ATOM_TYPE_sfID,
    AP4_ATOM_TYPE_akID,
    AP4_ATOM_TYPE_aART,
    AP4_ATOM_TYPE_TVNN,
    AP4_ATOM_TYPE_TVSH,
    AP4_ATOM_TYPE_TVEN,
    AP4_ATOM_TYPE_TVSN,
    AP4_ATOM_TYPE_TVES,
    AP4_ATOM_TYPE_STIK,
    AP4_ATOM_TYPE_PGAP
};
const AP4_MetaDataAtomTypeHandler::TypeList AP4_MetaDataAtomTypeHandler::IlstTypeList = {
    IlstTypes,
    sizeof(IlstTypes)/sizeof(IlstTypes[0])
};

/*----------------------------------------------------------------------
|   AP4_MetaDataAtomTypeHandler::CreateAtom
+---------------------------------------------------------------------*/
AP4_Result 
AP4_MetaDataAtomTypeHandler::CreateAtom(AP4_Atom::Type  type,
                                        AP4_UI32        size,
                                        AP4_ByteStream& stream,
                                        AP4_Atom::Type  context,
                                        AP4_Atom*&      atom)
{
    atom = NULL;

    if (context == AP4_ATOM_TYPE_ILST) {
        if (IsTypeInList(type, IlstTypeList)) {
            m_AtomFactory->SetContext(type);
            atom = AP4_ContainerAtom::Create(type, size, false, stream, *m_AtomFactory);
            m_AtomFactory->SetContext(context);
        }
    } else if (type == AP4_ATOM_TYPE_DATA) {
        if (IsTypeInList(context, IlstTypeList)) {
            atom = new AP4_DataAtom(size, stream);
        }
    } else if (context == AP4_ATOM_TYPE_dddd) {
        if (type == AP4_ATOM_TYPE_MEAN || type == AP4_ATOM_TYPE_NAME) {
            atom = new AP4_StringAtom(type, size, stream);
        }
    } else if (context == AP4_ATOM_TYPE_UDTA) {
        if (IsTypeInList(type, _3gppTypeList)) {
            atom = AP4_3GppAtom::Create(type, size, stream);
        }
    }

    return atom?AP4_SUCCESS:AP4_FAILURE;
}

/*----------------------------------------------------------------------
|   AP4_MetaDataAtomTypeHandler::IsInTypeList
+---------------------------------------------------------------------*/
bool
AP4_MetaDataAtomTypeHandler::IsTypeInList(AP4_Atom::Type type, const AP4_MetaDataAtomTypeHandler::TypeList& list)
{
    for (unsigned int i=0; i<list.m_Size; i++) {
        if (type == list.m_Types[i]) return true;
    }
    return false;
}

/*----------------------------------------------------------------------
|   AP4_MetaData::AP4_MetaData
+---------------------------------------------------------------------*/
AP4_MetaData::AP4_MetaData(AP4_File* file)
{
    // get the file's movie
    AP4_Movie* movie = file->GetMovie();

    // handle the movie's metadata if there is a movie in the file
    if (movie) {
        AP4_MoovAtom* moov = movie->GetMoovAtom();
        if (moov == NULL) return;
        ParseMoov(moov);
    } else {
        // if we don't have a movie, try to show metadata from a udta atom
        AP4_List<AP4_Atom>& top_level_atoms = file->GetOtherAtoms();
        
        AP4_List<AP4_Atom>::Item* atom_item = top_level_atoms.FirstItem();
        while (atom_item) {
            AP4_ContainerAtom* container = dynamic_cast<AP4_ContainerAtom*>(atom_item->GetData());
            if (container) {
                // look for a udta in a DCF layout
                AP4_Atom* udta = container->FindChild("odhe/udta");
                if (udta) {
                    AP4_ContainerAtom* udta_container = dynamic_cast<AP4_ContainerAtom*>(udta);
                    if (udta_container) {
                        ParseUdta(udta_container);
                    }
                }
            }
            atom_item = atom_item->GetNext();
        }
    } 
}

/*----------------------------------------------------------------------
|   AP4_MetaData::ParseMoov
+---------------------------------------------------------------------*/
AP4_Result
AP4_MetaData::ParseMoov(AP4_MoovAtom* moov)
{
    // look for a 'meta' atom with 'hdlr' type 'mdir'
    AP4_HdlrAtom* hdlr = dynamic_cast<AP4_HdlrAtom*>(moov->FindChild("udta/meta/hdlr"));
    if (hdlr == NULL || hdlr->GetHandlerType() != AP4_HANDLER_TYPE_MDIR) return AP4_ERROR_NO_SUCH_ITEM;

    // get the list of entries
    AP4_ContainerAtom* ilst = dynamic_cast<AP4_ContainerAtom*>(moov->FindChild("udta/meta/ilst"));
    if (ilst == NULL) return AP4_ERROR_NO_SUCH_ITEM;
    
    AP4_List<AP4_Atom>::Item* ilst_item = ilst->GetChildren().FirstItem();
    while (ilst_item) {
        AP4_ContainerAtom* entry_atom = dynamic_cast<AP4_ContainerAtom*>(ilst_item->GetData()); 
        if (entry_atom) {
            AddIlstEntries(entry_atom);
        }
        ilst_item = ilst_item->GetNext();
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_MetaData::ParseUdta
+---------------------------------------------------------------------*/
AP4_Result
AP4_MetaData::ParseUdta(AP4_ContainerAtom* udta)
{
    // check that the atom is indeed a 'udta' atom
    if (udta->GetType() != AP4_ATOM_TYPE_UDTA) {
        return AP4_ERROR_INVALID_PARAMETERS;
    }
    
    AP4_List<AP4_Atom>::Item* udta_item = udta->GetChildren().FirstItem();
    while (udta_item) {
        AP4_3GppAtom* entry_atom = dynamic_cast<AP4_3GppAtom*>(udta_item->GetData()); 
        if (entry_atom) {
            Add3GppEntry(entry_atom);
        }
        udta_item = udta_item->GetNext();
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_MetaData::~AP4_MetaData
+---------------------------------------------------------------------*/
AP4_MetaData::~AP4_MetaData()
{
    m_Entries.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_MetaData::AddIlstEntries
+---------------------------------------------------------------------*/
AP4_Result
AP4_MetaData::AddIlstEntries(AP4_ContainerAtom* atom)
{
    AP4_MetaData::Value* value = NULL;

    if (atom->GetType() == AP4_ATOM_TYPE_dddd) {
        // look for the namespace
        AP4_StringAtom* mean = static_cast<AP4_StringAtom*>(atom->GetChild(AP4_ATOM_TYPE_MEAN));
        if (mean == NULL) return AP4_ERROR_INVALID_FORMAT;

        // look for the name
        AP4_StringAtom* name = static_cast<AP4_StringAtom*>(atom->GetChild(AP4_ATOM_TYPE_NAME));
        if (name == NULL) return AP4_ERROR_INVALID_FORMAT;

        // get the value
        AP4_DataAtom* data_atom = static_cast<AP4_DataAtom*>(atom->GetChild(AP4_ATOM_TYPE_DATA));
        value = new AP4_AtomMetaDataValue(data_atom, atom->GetType());
        if (value == NULL) return AP4_ERROR_INVALID_FORMAT;
        
        return m_Entries.Add(new Entry(name->GetValue().GetChars(),
                                       mean->GetValue().GetChars(),
                                       value));
    } else {
        const char* key_name = NULL;
        const char* key_namespace = NULL;
        char        four_cc[5];

        // look for a match in the key infos
        for (unsigned int i=0; 
             i<sizeof(AP4_MetaData_KeyInfos)/sizeof(AP4_MetaData_KeyInfos[0]); 
             i++) {
            if (AP4_MetaData_KeyInfos[i].four_cc == atom->GetType()) {
                key_name = AP4_MetaData_KeyInfos[i].name;
                break;
            }
        }
        if (key_name == NULL) {
            // this key was not found in the key infos, create a name for it
            AP4_FormatFourChars(four_cc, (AP4_UI32)atom->GetType());
            key_name = four_cc;

            // put this name in the 'meta' namespace
            key_namespace = "meta";
        }

        // add one entry for each data atom
        AP4_List<AP4_Atom>::Item* data_item = atom->GetChildren().FirstItem();
        while (data_item) {
            AP4_Atom* item_atom = data_item->GetData();
            if (item_atom->GetType() == AP4_ATOM_TYPE_DATA) {
                AP4_DataAtom* data_atom = static_cast<AP4_DataAtom*>(item_atom);
                value = new AP4_AtomMetaDataValue(data_atom, atom->GetType());
                m_Entries.Add(new Entry(key_name, key_namespace, value));
            }
            data_item = data_item->GetNext();
        }

        return AP4_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   AP4_MetaData::Add3GppEntry
+---------------------------------------------------------------------*/
AP4_Result
AP4_MetaData::Add3GppEntry(AP4_3GppAtom* atom)
{
    const char* key_name = NULL;
    const char* key_namespace = NULL;
    char        four_cc[5];

    switch (atom->GetType()) {
        case AP4_ATOM_TYPE_TITL:
        case AP4_ATOM_TYPE_DSCP:
        case AP4_ATOM_TYPE_CPRT:
        case AP4_ATOM_TYPE_PERF:
        case AP4_ATOM_TYPE_AUTH:
        case AP4_ATOM_TYPE_GNRE:
        case AP4_ATOM_TYPE_RTNG:
        case AP4_ATOM_TYPE_ALBM: {
            // look for a match in the key infos
            for (unsigned int i=0; 
                 i<sizeof(AP4_MetaData_KeyInfos)/sizeof(AP4_MetaData_KeyInfos[0]); 
                 i++) {
                if (AP4_MetaData_KeyInfos[i].four_cc == atom->GetType()) {
                    key_name = AP4_MetaData_KeyInfos[i].name;
                    break;
                }
            }
            if (key_name == NULL) {
                // this key was not found in the key infos, create a name for it
                AP4_FormatFourChars(four_cc, (AP4_UI32)atom->GetType());
                key_name = four_cc;

                // put this name in the 'udta' namespace
                key_namespace = "udta";
            }

            AP4_MetaData::Value* value = new AP4_MetaData::StringValue((const char*)atom->GetPayload().GetData());
            m_Entries.Add(new Entry(key_name, key_namespace, value));
            break;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_MetaData::StringValue::ToString
+---------------------------------------------------------------------*/
AP4_String
AP4_MetaData::StringValue::ToString()
{
    return m_Value;
}

/*----------------------------------------------------------------------
|   AP4_MetaData::StringValue::ToBytes
+---------------------------------------------------------------------*/
AP4_Result
AP4_MetaData::StringValue::ToBytes(AP4_DataBuffer& /* bytes */)
{
    return AP4_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   AP4_MetaData::StringValue::ToInteger
+---------------------------------------------------------------------*/
long
AP4_MetaData::StringValue::ToInteger()
{
    return 0;
}

/*----------------------------------------------------------------------
|   AP4_AtomMetaDataValue::MapDataType
+---------------------------------------------------------------------*/
AP4_MetaData::Value::Type
AP4_AtomMetaDataValue::MapDataType(AP4_MetaData::DataType data_type)
{
    switch (data_type) {
        case AP4_MetaData::DATA_TYPE_SIGNED_INT_BE:
            return AP4_MetaData::Value::TYPE_INTEGER;

        case AP4_MetaData::DATA_TYPE_STRING_UTF8:
        case AP4_MetaData::DATA_TYPE_STRING_UTF16:
        case AP4_MetaData::DATA_TYPE_STRING_MAC_ENCODED:
            return AP4_MetaData::Value::TYPE_STRING;

        default:
            return AP4_MetaData::Value::TYPE_BINARY;
    }
}

/*----------------------------------------------------------------------
|   AP4_AtomMetaDataValue::AP4_AtomMetaDataValue
+---------------------------------------------------------------------*/
AP4_AtomMetaDataValue::AP4_AtomMetaDataValue(AP4_DataAtom*  atom,
                                             AP4_UI32       parent_type) :
    Value(MapDataType(atom->GetDataType())),
    m_DataAtom(atom)
{
    switch (parent_type) {
        case AP4_ATOM_TYPE_GNRE:
            m_Meaning = MEANING_ID3_GENRE;
            break;

        case AP4_ATOM_TYPE_CPIL:
            m_Meaning = MEANING_BOOLEAN;
            break;

        case AP4_ATOM_TYPE_PGAP:
            m_Meaning = MEANING_BOOLEAN;
            break;

        default:
            break;
    }
}

/*----------------------------------------------------------------------
|   AP4_AtomMetaDataValue::ToString
+---------------------------------------------------------------------*/
AP4_String 
AP4_AtomMetaDataValue::ToString() 
{
    switch (m_Type) {
        case AP4_MetaData::Value::TYPE_INTEGER: 
            {
                long value;
                char string[32];
                if (AP4_SUCCEEDED(m_DataAtom->LoadInteger(value))) {
                    if (m_Meaning == MEANING_BOOLEAN) {
                        if (value) {
                            return "True";
                        } else {
                            return "False";
                        }
                    } else {
                        AP4_FormatString(string, sizeof(string), "%ld", value);
                        return AP4_String((const char*)string);
                    }
                }
                break;
            }

        case AP4_MetaData::Value::TYPE_STRING:
            {
                AP4_String* string;
                if (AP4_SUCCEEDED(m_DataAtom->LoadString(string))) {
                    AP4_String result(*string);
                    delete string;
                    return result;
                }
                break;
            }

        case AP4_MetaData::Value::TYPE_BINARY:
            {
                char string[32] = "";
                AP4_DataBuffer data;
                if (AP4_SUCCEEDED(m_DataAtom->LoadBytes(data))) {
                    if (m_Meaning == MEANING_ID3_GENRE && data.GetDataSize() == 2) {
                        unsigned int genre = (data.GetData()[0])*256+data.GetData()[1];
                        if (genre >= 1 && genre <= sizeof(Ap4Id3Genres)/sizeof(Ap4Id3Genres[0])) {
                            return Ap4Id3Genres[genre-1];
                        } else {
                            return "Unknown";
                        }
                    } else {
                        AP4_FormatString(string, sizeof(string), "[%ld bytes]", data.GetDataSize());
                    }
                }
                return AP4_String(string);
            }
        default:
            return AP4_String();
    }

    return AP4_String();
}

/*----------------------------------------------------------------------
|   AP4_AtomMetaDataValue::ToBytes
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomMetaDataValue::ToBytes(AP4_DataBuffer& bytes)
{
    return m_DataAtom->LoadBytes(bytes);
}

/*----------------------------------------------------------------------
|   AP4_AtomMetaDataValue::ToInteger
+---------------------------------------------------------------------*/
long       
AP4_AtomMetaDataValue::ToInteger()
{
    long value;
    if (AP4_SUCCEEDED(m_DataAtom->LoadInteger(value))) {
        return value;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_DataAtom::AP4_DataAtom
+---------------------------------------------------------------------*/
AP4_DataAtom::AP4_DataAtom(AP4_UI32 size, AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_DATA, size),
    m_DataType(AP4_MetaData::DATA_TYPE_BINARY),
    m_DataLang(AP4_MetaData::LANGUAGE_ENGLISH) 
{
    if (size < AP4_ATOM_HEADER_SIZE+8) return;

    AP4_UI32 i;
    stream.ReadUI32(i); m_DataType = (AP4_MetaData::DataType)i;
    stream.ReadUI32(i); m_DataLang = (AP4_MetaData::Language)i;

    // the stream for the data is a substream of this source
    AP4_Position data_offset;
    stream.Tell(data_offset);
    AP4_Size data_size = size-AP4_ATOM_HEADER_SIZE-8;
    m_Source = new AP4_SubStream(stream, data_offset, data_size);
}

/*----------------------------------------------------------------------
|   AP4_DataAtom::~AP4_DataAtom
+---------------------------------------------------------------------*/
AP4_DataAtom::~AP4_DataAtom()
{
    delete(m_Source);
}

/*----------------------------------------------------------------------
|   AP4_DataAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DataAtom::WriteFields(AP4_ByteStream& stream)
{
    stream.WriteUI32(m_DataType);
    stream.WriteUI32(m_DataLang);
    if (m_Source) {
        AP4_LargeSize size = 0;
        m_Source->GetSize(size);
        m_Source->Seek(0);
        m_Source->CopyTo(stream, size);
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_DataAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DataAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("type", m_DataType);
    inspector.AddField("lang", m_DataLang);

    if (m_DataType == AP4_MetaData::DATA_TYPE_STRING_UTF8) {
        AP4_String* str;
        if (AP4_SUCCEEDED(LoadString(str))) {
            inspector.AddField("value", str->GetChars());
            delete str;
        }
    } else if (m_DataType == AP4_MetaData::DATA_TYPE_SIGNED_INT_BE) {
        long value;
        if (AP4_SUCCEEDED(LoadInteger(value))) {
            inspector.AddField("value", value);
        }
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_DataAtom::LoadString
+---------------------------------------------------------------------*/
AP4_Result
AP4_DataAtom::LoadString(AP4_String*& string)
{
    if (m_Source == NULL) {
        string = new AP4_String();
        return AP4_SUCCESS;
    } else {
        // create a string with enough capactiy for the data
        AP4_LargeSize size = 0;
        m_Source->GetSize(size);
        if (size > AP4_DATA_ATOM_MAX_SIZE) return AP4_ERROR_OUT_OF_RANGE;
        string = new AP4_String((AP4_Size)size);

        // read from the start of the stream
        m_Source->Seek(0);
        AP4_Result result = m_Source->Read(string->UseChars(), (AP4_Size)size);
        if (AP4_FAILED(result)) {
            delete string;
            string = NULL;
        }

        return result;
    }
}

/*----------------------------------------------------------------------
|   AP4_DataAtom::LoadBytes
+---------------------------------------------------------------------*/
AP4_Result
AP4_DataAtom::LoadBytes(AP4_DataBuffer& bytes)
{
    if (m_Source == NULL) {
        bytes.SetDataSize(0);
        return AP4_SUCCESS;
    }
    AP4_LargeSize size = 0;
    m_Source->GetSize(size);
    if (size > AP4_DATA_ATOM_MAX_SIZE) return AP4_ERROR_OUT_OF_RANGE;
    bytes.SetDataSize((AP4_Size)size);
    m_Source->Seek(0);
    AP4_Result result = m_Source->Read(bytes.UseData(), (AP4_Size)size);
    if (AP4_FAILED(result)) {
        bytes.SetDataSize(0);
    }
    return result;
}

/*----------------------------------------------------------------------
|   AP4_DataAtom::LoadInteger
+---------------------------------------------------------------------*/
AP4_Result
AP4_DataAtom::LoadInteger(long& value) 
{
    AP4_Result result = AP4_FAILURE;
    value = 0;
    if (m_Source == NULL) return AP4_SUCCESS;
    AP4_LargeSize size = 0;
    m_Source->GetSize(size);
    if (size > 4) {
        return AP4_ERROR_OUT_OF_RANGE;
    }
    unsigned char bytes[4];
    m_Source->Seek(0);
    m_Source->Read(bytes, (AP4_Size)size);
    result = AP4_SUCCESS;
    switch (size) {
        case 1: value = bytes[0]; break;
        case 2: value = AP4_BytesToInt16BE(bytes); break;
        case 4: value = AP4_BytesToInt32BE(bytes); break;
    }
    return result;
}

/*----------------------------------------------------------------------
|   AP4_StringAtom::AP4_StringAtom
+---------------------------------------------------------------------*/
AP4_StringAtom::AP4_StringAtom(Type type, AP4_UI32 size, AP4_ByteStream& stream) :
    AP4_Atom(type, size),
    m_Value((AP4_Size)(size-AP4_ATOM_HEADER_SIZE-4))
{
    stream.ReadUI32(m_Reserved);
    stream.Read(m_Value.UseChars(), m_Value.GetLength());
}

/*----------------------------------------------------------------------
|   AP4_StringAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_StringAtom::WriteFields(AP4_ByteStream& stream)
{
    stream.WriteUI32(m_Reserved);
    return stream.Write(m_Value.GetChars(), m_Value.GetLength());
}

/*----------------------------------------------------------------------
|   AP4_StringAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_StringAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("value", m_Value.GetChars());
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_3GppAtom::Create
+---------------------------------------------------------------------*/
AP4_3GppAtom*
AP4_3GppAtom::Create(Type type, AP4_UI32 size, AP4_ByteStream& stream) 
{
    AP4_UI32 version;
    AP4_UI32 flags;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version != 0) return NULL;
    return new AP4_3GppAtom(type, size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_3GppAtom::AP4_3GppAtom
+---------------------------------------------------------------------*/
AP4_3GppAtom::AP4_3GppAtom(Type            type, 
                           AP4_UI32        size, 
                           AP4_UI32        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(type, size, version, flags)
{
    // read the language code
    AP4_UI16 packed_language;
    stream.ReadUI16(packed_language);
    m_Language[0] = 0x60+((packed_language>>10)&0x1F);
    m_Language[1] = 0x60+((packed_language>> 5)&0x1F);
    m_Language[2] = 0x60+((packed_language    )&0x1F);
    m_Language[3] = '\0';
    
    // read the payload
    if (size > AP4_FULL_ATOM_HEADER_SIZE+2) {
        AP4_UI32 payload_size = size-(AP4_FULL_ATOM_HEADER_SIZE+2);
        m_Payload.SetBufferSize(payload_size);
        stream.Read(m_Payload.UseData(), payload_size);
        m_Payload.SetDataSize(payload_size);
    }
}

/*----------------------------------------------------------------------
|   AP4_3GppAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_3GppAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_UI16 packed_language = ((m_Language[0]-0x60)<<10) |
                               ((m_Language[1]-0x60)<< 5) |
                               ((m_Language[2]-0x60));
    stream.WriteUI16(packed_language);
    return stream.Write(m_Payload.GetData(), m_Payload.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_3GppAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_3GppAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("language", GetLanguage());
    inspector.AddField("payload size", m_Payload.GetDataSize());
    return AP4_SUCCESS;
}
