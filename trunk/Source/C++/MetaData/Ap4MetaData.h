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

#ifndef _AP4_META_DATA_H_
#define _AP4_META_DATA_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4AtomFactory.h"
#include "Ap4String.h"
#include "Ap4Array.h"
#include "Ap4List.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class AP4_File;
class AP4_MoovAtom;
class AP4_DataBuffer;
class AP4_ContainerAtom;
class AP4_DataAtom;
class AP4_3GppAtom;

/*----------------------------------------------------------------------
|   metadata keys
+---------------------------------------------------------------------*/
const AP4_Atom::Type AP4_ATOM_TYPE_DATA = AP4_ATOM_TYPE('d','a','t','a'); // data
const AP4_Atom::Type AP4_ATOM_TYPE_MEAN = AP4_ATOM_TYPE('m','e','a','n'); // namespace
const AP4_Atom::Type AP4_ATOM_TYPE_NAME = AP4_ATOM_TYPE('n','a','m','e'); // name
const AP4_Atom::Type AP4_ATOM_TYPE_dddd = AP4_ATOM_TYPE('-','-','-','-'); // free form
const AP4_Atom::Type AP4_ATOM_TYPE_cNAM = AP4_ATOM_TYPE('©','n','a','m'); // name
const AP4_Atom::Type AP4_ATOM_TYPE_cART = AP4_ATOM_TYPE('©','A','R','T'); // artist
const AP4_Atom::Type AP4_ATOM_TYPE_cCOM = AP4_ATOM_TYPE('©','c','o','m'); // composer
const AP4_Atom::Type AP4_ATOM_TYPE_cWRT = AP4_ATOM_TYPE('©','w','r','t'); // writer
const AP4_Atom::Type AP4_ATOM_TYPE_cALB = AP4_ATOM_TYPE('©','a','l','b'); // album
const AP4_Atom::Type AP4_ATOM_TYPE_cGEN = AP4_ATOM_TYPE('©','g','e','n'); // genre
const AP4_Atom::Type AP4_ATOM_TYPE_cGRP = AP4_ATOM_TYPE('©','g','r','p'); // group
const AP4_Atom::Type AP4_ATOM_TYPE_cDAY = AP4_ATOM_TYPE('©','d','a','y'); // date
const AP4_Atom::Type AP4_ATOM_TYPE_cTOO = AP4_ATOM_TYPE('©','t','o','o'); // tool
const AP4_Atom::Type AP4_ATOM_TYPE_cCMT = AP4_ATOM_TYPE('©','c','m','t'); // comment
const AP4_Atom::Type AP4_ATOM_TYPE_cLYR = AP4_ATOM_TYPE('©','l','y','r'); // lyrics
const AP4_Atom::Type AP4_ATOM_TYPE_TRKN = AP4_ATOM_TYPE('t','r','k','n'); // track#
const AP4_Atom::Type AP4_ATOM_TYPE_DISK = AP4_ATOM_TYPE('d','i','s','k'); // disk#
const AP4_Atom::Type AP4_ATOM_TYPE_COVR = AP4_ATOM_TYPE('c','o','v','r'); // cover art
const AP4_Atom::Type AP4_ATOM_TYPE_DESC = AP4_ATOM_TYPE('d','e','s','c'); // description
const AP4_Atom::Type AP4_ATOM_TYPE_CPIL = AP4_ATOM_TYPE('c','p','i','l'); // compilation?
const AP4_Atom::Type AP4_ATOM_TYPE_TMPO = AP4_ATOM_TYPE('t','m','p','o'); // tempo
const AP4_Atom::Type AP4_ATOM_TYPE_apID = AP4_ATOM_TYPE('a','p','I','D');
const AP4_Atom::Type AP4_ATOM_TYPE_cnID = AP4_ATOM_TYPE('c','n','I','D');
const AP4_Atom::Type AP4_ATOM_TYPE_atID = AP4_ATOM_TYPE('a','t','I','D');
const AP4_Atom::Type AP4_ATOM_TYPE_plID = AP4_ATOM_TYPE('p','l','I','D');
const AP4_Atom::Type AP4_ATOM_TYPE_geID = AP4_ATOM_TYPE('g','e','I','D');
const AP4_Atom::Type AP4_ATOM_TYPE_sfID = AP4_ATOM_TYPE('s','f','I','D');
const AP4_Atom::Type AP4_ATOM_TYPE_akID = AP4_ATOM_TYPE('a','k','I','D');
const AP4_Atom::Type AP4_ATOM_TYPE_aART = AP4_ATOM_TYPE('a','A','R','T');
const AP4_Atom::Type AP4_ATOM_TYPE_TVNN = AP4_ATOM_TYPE('t','v','n','n'); // TV network
const AP4_Atom::Type AP4_ATOM_TYPE_TVSH = AP4_ATOM_TYPE('t','v','s','h'); // TV show
const AP4_Atom::Type AP4_ATOM_TYPE_TVEN = AP4_ATOM_TYPE('t','v','e','n'); // TV episode name
const AP4_Atom::Type AP4_ATOM_TYPE_TVSN = AP4_ATOM_TYPE('t','v','s','n'); // TV show season #
const AP4_Atom::Type AP4_ATOM_TYPE_TVES = AP4_ATOM_TYPE('t','v','e','s'); // TV show episode #
const AP4_Atom::Type AP4_ATOM_TYPE_STIK = AP4_ATOM_TYPE('s','t','i','k');
const AP4_Atom::Type AP4_ATOM_TYPE_PGAP = AP4_ATOM_TYPE('p','g','a','p'); // Gapless Playback
const AP4_Atom::Type AP4_ATOM_TYPE_TITL = AP4_ATOM_TYPE('t','i','t','l'); // 3GPP: title
const AP4_Atom::Type AP4_ATOM_TYPE_DSCP = AP4_ATOM_TYPE('d','s','c','p'); // 3GPP: description
const AP4_Atom::Type AP4_ATOM_TYPE_CPRT = AP4_ATOM_TYPE('c','p','r','t'); // ISO or ILST: copyright
const AP4_Atom::Type AP4_ATOM_TYPE_PERF = AP4_ATOM_TYPE('p','e','r','f'); // 3GPP: performer
const AP4_Atom::Type AP4_ATOM_TYPE_AUTH = AP4_ATOM_TYPE('a','u','t','h'); // 3GPP: author
const AP4_Atom::Type AP4_ATOM_TYPE_GNRE = AP4_ATOM_TYPE('g','n','r','e'); // 3GPP or ILST: genre (in 3GPP -> string, in ILST -> ID3v1 index + 1)
const AP4_Atom::Type AP4_ATOM_TYPE_RTNG = AP4_ATOM_TYPE('r','t','n','g'); // 3GPP or ILST: rating
const AP4_Atom::Type AP4_ATOM_TYPE_CLSF = AP4_ATOM_TYPE('c','l','s','f'); // 3GPP: classification
const AP4_Atom::Type AP4_ATOM_TYPE_KYWD = AP4_ATOM_TYPE('k','y','w','d'); // 3GPP: keywords
const AP4_Atom::Type AP4_ATOM_TYPE_LOCI = AP4_ATOM_TYPE('l','o','c','i'); // 3GPP: location information
const AP4_Atom::Type AP4_ATOM_TYPE_ALBM = AP4_ATOM_TYPE('a','l','b','m'); // 3GPP: album title and track number
const AP4_Atom::Type AP4_ATOM_TYPE_YRRC = AP4_ATOM_TYPE('y','r','r','c'); // 3GPP: recording year
const AP4_Atom::Type AP4_ATOM_TYPE_TSEL = AP4_ATOM_TYPE('t','s','e','l'); // 3GPP: track selection

/*----------------------------------------------------------------------
|   AP4_MetaData
+---------------------------------------------------------------------*/
class AP4_MetaData {
public:
    typedef enum {
        DATA_TYPE_BINARY             = 0,
        DATA_TYPE_STRING_UTF8        = 1,
        DATA_TYPE_STRING_UTF16       = 2,
        DATA_TYPE_STRING_MAC_ENCODED = 3,
        DATA_TYPE_JPEG               = 14,
        DATA_TYPE_SIGNED_INT_BE      = 21, /* the size of the integer is derived from the container size */
        DATA_TYPE_FLOAT32_BE         = 22,
        DATA_TYPE_FLOAT64_BE         = 23
    } DataType;

    typedef enum {
        LANGUAGE_ENGLISH = 0
    } Language;

    class Key {
    public:
        // constructors
        Key(const char* name, const char* ns) :
          m_Name(name), m_Namespace(ns) {}

        // methods
        const char* GetNamespace() { return m_Namespace.GetChars(); }
        const char* GetName()      { return m_Name.GetChars();      }

    private:
        // members
        const AP4_String m_Name;
        const AP4_String m_Namespace;
    };

    class Value {
    public:
        // types 
        typedef enum {
            TYPE_STRING,
            TYPE_BINARY,
            TYPE_INTEGER
        } Type;

        typedef enum {
            MEANING_UNKNOWN,
            MEANING_ID3_GENRE,
            MEANING_BOOLEAN
        } Meaning;

        // destructor
        virtual ~Value() {}

        // methods
        Type               GetType()    { return m_Type;    }
        Meaning            GetMeaning() { return m_Meaning; }
        virtual AP4_String ToString() = 0;
        virtual AP4_Result ToBytes(AP4_DataBuffer& bytes) = 0;
        virtual long       ToInteger() = 0;

    protected:
        // constructor
        Value(Type type, Meaning meaning = MEANING_UNKNOWN) : 
             m_Type(type), m_Meaning(meaning) {}

        // members
        Type    m_Type;
        Meaning m_Meaning;
    };

    class StringValue : public Value {
    public:
        StringValue(const char* value) : Value(TYPE_STRING), m_Value(value) {}
        virtual AP4_String ToString();
        virtual AP4_Result ToBytes(AP4_DataBuffer& bytes);
        virtual long       ToInteger();
        
    private:
        AP4_String m_Value;
    };
    
    class KeyInfo {
    public:
        // members
        const char* name;
        const char* description;
        AP4_UI32    four_cc;
        Value::Type value_type;
    };

    class Entry {
    public:
        // constructor
        Entry(const char* name, const char* ns, Value* value) :
          m_Key(name, ns), m_Value(value) {}

        // destructor
        ~Entry() { delete m_Value; }

        // members
        Key    m_Key;
        Value* m_Value;    
    };

    // class members
    static AP4_Array<KeyInfo> KeysInfos;

    // constructor
    AP4_MetaData(AP4_File* file);
    
    // methods
    AP4_Result ParseMoov(AP4_MoovAtom* moov);
    AP4_Result ParseUdta(AP4_ContainerAtom* udta);
    
    // destructor
    ~AP4_MetaData();

    // accessors
    const AP4_List<Entry>& GetEntries() const { return m_Entries; }

    // methods
    AP4_Result AddIlstEntries(AP4_ContainerAtom* atom);
    AP4_Result Add3GppEntry(AP4_3GppAtom* atom);

private:
    // members
    AP4_List<Entry> m_Entries;
};

/*----------------------------------------------------------------------
|   AP4_MetaDataAtomTypeHandler
+---------------------------------------------------------------------*/
class AP4_MetaDataAtomTypeHandler : public AP4_AtomFactory::TypeHandler
{
public:
    // constructor
    AP4_MetaDataAtomTypeHandler(AP4_AtomFactory* atom_factory) :
      m_AtomFactory(atom_factory) {}
    virtual AP4_Result CreateAtom(AP4_Atom::Type  type,
                                  AP4_UI32        size,
                                  AP4_ByteStream& stream,
                                  AP4_Atom::Type  context,
                                  AP4_Atom*&      atom);

private:
    // types
    struct TypeList {
        const AP4_Atom::Type* m_Types;
        AP4_Size              m_Size;
    };
    
    // class constants
    static const AP4_Atom::Type IlstTypes[];
    static const TypeList       IlstTypeList;
    static const AP4_Atom::Type _3gppTypes[];
    static const TypeList       _3gppTypeList;
    
    // class methods
    static bool IsTypeInList(AP4_Atom::Type type, const TypeList& list);

    // members
    AP4_AtomFactory* m_AtomFactory;
};

/*----------------------------------------------------------------------
|   AP4_MetaDataTag
+---------------------------------------------------------------------*/
class AP4_MetaDataTag
{
public:

    // destructor
    virtual ~AP4_MetaDataTag() {}

    // methods
    virtual AP4_Result Write(AP4_ByteStream& stream);
    virtual AP4_Result Inspect(AP4_AtomInspector& inspector);

protected:
    // constructor
    AP4_MetaDataTag(AP4_UI32        data_type, 
                    AP4_UI32        data_lang,
                    AP4_Size        size, 
                    AP4_ByteStream& stream);

};

/*----------------------------------------------------------------------
|   AP4_3GppAtom
+---------------------------------------------------------------------*/
class AP4_3GppAtom : public AP4_Atom
{
public:
    // factory method
    static AP4_3GppAtom* Create(Type type, AP4_UI32 size, AP4_ByteStream& stream);
     
    // constructor
    AP4_3GppAtom(Type            type, 
                 AP4_UI32        size, 
                 AP4_UI32        version,
                 AP4_UI32        flags,
                 AP4_ByteStream& stream);
    
    // AP4_Atom methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);
    
    // methods
    const char*           GetLanguage() const { return m_Language; }
    const AP4_DataBuffer& GetPayload() const  { return m_Payload;  }
    
private:
    // members
    char           m_Language[4];
    AP4_DataBuffer m_Payload;
};

/*----------------------------------------------------------------------
|   AP4_StringAtom
+---------------------------------------------------------------------*/
class AP4_StringAtom : public AP4_Atom
{
public:
    // constructor
    AP4_StringAtom(Type type, AP4_UI32 size, AP4_ByteStream& stream);

    // AP4_Atom methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // methods
    const AP4_String& GetValue() { return m_Value; }

private:
    // members
    AP4_UI32   m_Reserved;
    AP4_String m_Value;
};

/*----------------------------------------------------------------------
|   AP4_DataAtom
+---------------------------------------------------------------------*/
class AP4_DataAtom : public AP4_Atom
{
public:
    // constructor
    AP4_DataAtom(AP4_UI32 size, AP4_ByteStream& stream);

    // destructor
    ~AP4_DataAtom();

    // AP4_Atom methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_MetaData::DataType GetDataType() { return m_DataType; }
    AP4_MetaData::Language GetDataLang() { return m_DataLang; }

    // methods
    AP4_Result LoadString(AP4_String*& string);
    AP4_Result LoadBytes(AP4_DataBuffer& bytes);
    AP4_Result LoadInteger(long& value);

private:
    // members
    AP4_MetaData::DataType m_DataType;
    AP4_MetaData::Language m_DataLang;
    AP4_ByteStream*        m_Source;
};

/*----------------------------------------------------------------------
|   AP4_AtomMetaDataValue
+---------------------------------------------------------------------*/
class AP4_AtomMetaDataValue : public AP4_MetaData::Value
{
public:
    // constructor
    AP4_AtomMetaDataValue(AP4_DataAtom* data_atom, AP4_UI32 parent_type);

    // AP4_MetaData::Value methods
    virtual AP4_String ToString();
    virtual AP4_Result ToBytes(AP4_DataBuffer& bytes);
    virtual long       ToInteger();

private:
    // class methods
    static AP4_MetaData::Value::Type MapDataType(AP4_MetaData::DataType data_type);

    // members
    AP4_DataAtom* m_DataAtom;
};

#endif // _AP4_META_DATA_H_
