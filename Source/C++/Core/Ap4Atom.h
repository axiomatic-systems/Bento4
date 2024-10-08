/*****************************************************************
|
|    AP4 - Atoms 
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
/**
* @file 
* @brief Atoms
*/

#ifndef _AP4_ATOM_H_
#define _AP4_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4List.h"
#include "Ap4ByteStream.h"
#include "Ap4String.h"
#include "Ap4Debug.h"
#include "Ap4DynamicCast.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define AP4_ATOM_TYPE(c1,c2,c3,c4)       \
   (((static_cast<AP4_UI32>(c1))<<24) |  \
    ((static_cast<AP4_UI32>(c2))<<16) |  \
    ((static_cast<AP4_UI32>(c3))<< 8) |  \
    ((static_cast<AP4_UI32>(c4))    ))

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_ATOM_HEADER_SIZE         = 8;
const AP4_UI32 AP4_ATOM_HEADER_SIZE_64      = 16;
const AP4_UI32 AP4_FULL_ATOM_HEADER_SIZE    = 12;
const AP4_UI32 AP4_FULL_ATOM_HEADER_SIZE_64 = 20;
const AP4_UI32 AP4_ATOM_MAX_NAME_SIZE       = 256;
const AP4_UI32 AP4_ATOM_MAX_URI_SIZE        = 512;

/*----------------------------------------------------------------------
|   forward references
+---------------------------------------------------------------------*/
class AP4_AtomParent;

/*----------------------------------------------------------------------
|   AP4_AtomInspector
+---------------------------------------------------------------------*/
/**
 * Class used in a visitor pattern to walk all the atoms in a tree of
 * #AP4_Atom objects. 
 */
class AP4_AtomInspector {
public:
    // types
    typedef enum {
        HINT_NONE    = 0,
        HINT_HEX     = 1,
        HINT_BOOLEAN = 2
    } FormatHint;

    // constructor and destructor
    AP4_AtomInspector() : m_Verbosity(0) {}
    virtual ~AP4_AtomInspector() {}

    // methods
    void        SetVerbosity(AP4_Ordinal verbosity) { m_Verbosity = verbosity; }
    AP4_Ordinal GetVerbosity()                      { return m_Verbosity;      }
    
    // virtual methods
    virtual void StartAtom(const char* /* name        */,
                           AP4_UI08    /* version     */,
                           AP4_UI32    /* flags       */,
                           AP4_Size    /* header_size */,
                           AP4_UI64    /*size         */) {}
    virtual void EndAtom() {}
    virtual void StartDescriptor(const char* /* name        */,
                                 AP4_Size    /* header_size */,
                                 AP4_UI64    /*size         */) {}
    virtual void EndDescriptor() {}
    virtual void StartArray(const char* /* name */, unsigned int element_count = 0) {}
    virtual void EndArray() {}
    virtual void StartObject(const char* /* name */, unsigned int field_count = 0, bool compact = false) {}
    virtual void EndObject() {}
    virtual void AddField(const char* /* name */,
                          AP4_UI64    /* value */, 
                          FormatHint  hint = HINT_NONE) {
        (void)hint; // gcc warning
    }
    virtual void AddFieldF(const char* /* name */, 
                           float       /* value */, 
                           FormatHint  hint = HINT_NONE) {
        (void)hint; // gcc warning
    }
    virtual void AddField(const char* /* name */, 
                          const char* /* value */, 
                          FormatHint  hint = HINT_NONE) {
        (void)hint; // gcc warning 
    }
    virtual void AddField(const char*          /* name */, 
                          const unsigned char* /* bytes */, 
                          AP4_Size             /* byte_count */, 
                          FormatHint           hint = HINT_NONE) {
        (void)hint; // gcc warning 
    }
    
protected:
    AP4_Ordinal m_Verbosity;
};

/*----------------------------------------------------------------------
|   AP4_PrintInspector
+---------------------------------------------------------------------*/
class AP4_PrintInspector : public AP4_AtomInspector {
public:
    AP4_PrintInspector(AP4_ByteStream& stream, AP4_Cardinal indent=0);
    ~AP4_PrintInspector();

    // methods
    void StartAtom(const char* name,
                   AP4_UI08    version,
                   AP4_UI32    flags,
                   AP4_Size    header_size,
                   AP4_UI64    size);
    void EndAtom();
    void StartDescriptor(const char* name,
                         AP4_Size    header_size,
                         AP4_UI64    size);
    void EndDescriptor();
    void StartArray(const char* name, unsigned int element_count);
    void EndArray();
    void StartObject(const char* name, unsigned int field_count, bool compact);
    void EndObject();
    void AddField(const char* name, AP4_UI64 value, FormatHint hint);
    void AddFieldF(const char* name, float value, FormatHint hint);
    void AddField(const char* name, const char* value, FormatHint hint);
    void AddField(const char* name, const unsigned char* bytes, AP4_Size size, FormatHint hint);

private:
    // types
    struct Context {
        typedef enum {
            TOP_LEVEL,
            ATOM,
            ARRAY,
            OBJECT,
            COMPACT_OBJECT
        } Type;

        Context(Type type) : m_Type(type), m_ArrayIndex(0) {}

        Type         m_Type;
        AP4_Cardinal m_ArrayIndex;
     };

    // methods
    void     PushContext(Context::Type type);
    void     PopContext();
    Context& LastContext() { return m_Contexts[m_Contexts.ItemCount() - 1]; }
    void     PrintPrefix();
    void     PrintSuffix();

    // members
    AP4_ByteStream*    m_Stream;
    AP4_Array<Context> m_Contexts;
};

/*----------------------------------------------------------------------
|   AP4_JsonInspector
+---------------------------------------------------------------------*/
class AP4_JsonInspector : public AP4_AtomInspector {
public:
    AP4_JsonInspector(AP4_ByteStream& stream);
    ~AP4_JsonInspector();

    // methods
    void StartAtom(const char* name,
                   AP4_UI08    version,
                   AP4_UI32    flags,
                   AP4_Size    header_size,
                   AP4_UI64    size);
    void EndAtom();
    void StartDescriptor(const char* name,
                         AP4_Size    header_size,
                         AP4_UI64    size);
    void EndDescriptor();
    void StartArray(const char* name, unsigned int element_count);
    void EndArray();
    void StartObject(const char* name, unsigned int field_count, bool compact);
    void EndObject();
    void AddField(const char* name, AP4_UI64 value, FormatHint hint);
    void AddFieldF(const char* name, float value, FormatHint hint);
    void AddField(const char* name, const char* value, FormatHint hint);
    void AddField(const char* name, const unsigned char* bytes, AP4_Size size, FormatHint hint);

private:
    // types
    struct Context {
        typedef enum {
            TOP_LEVEL,
            ATOM,
            ARRAY,
            OBJECT
        } Type;

        Context(Type type) : m_Type(type), m_FieldCount(0), m_ChildrenCount(0) {}

        Type         m_Type;
        AP4_Cardinal m_FieldCount;
        AP4_Cardinal m_ChildrenCount; // to count atoms within atoms
     };

    // methods
    static AP4_String EscapeString(const char* string);
    void              PushContext(Context::Type type);
    void              PopContext();
    Context&          LastContext() { return m_Contexts[m_Contexts.ItemCount() - 1]; }
    void              OnFieldAdded();
    void              PrintFieldName(const char* name);
    
    // members
    AP4_ByteStream*    m_Stream;
    AP4_Array<Context> m_Contexts;
    char               m_Prefix[256];
};

/*----------------------------------------------------------------------
|   AP4_Atom
+---------------------------------------------------------------------*/
/**
 * Abstract base class for all atom types.
 */
class AP4_Atom {
public:
     AP4_IMPLEMENT_DYNAMIC_CAST(AP4_Atom)

   // types
    typedef AP4_UI32 Type;

    // class methods
    static Type TypeFromString(const char* four_cc);
    static AP4_Result ReadFullHeader(AP4_ByteStream& stream, 
                                     AP4_UI08&       version, 
                                     AP4_UI32&       flags);

    // constructors
    /**
     * Create a simple atom with a specified type and 32-bit size.
     */
    explicit AP4_Atom(Type type, AP4_UI32 size = AP4_ATOM_HEADER_SIZE);

    /**
     * Create a simple atom with a specified type and 64-bit size.
     */
    explicit AP4_Atom(Type type, AP4_UI64 size, bool force_64=false);

    /**
     * Create a full atom with a specified type, 32-bit size, version and flags.
     */
    explicit AP4_Atom(Type     type, 
                      AP4_UI32 size,
                      AP4_UI08 version, 
                      AP4_UI32 flags);

    /**
     * Create a full atom with a specified type, 64-bit size, version and flags.
     */
    explicit AP4_Atom(Type     type, 
                      AP4_UI64 size,
                      bool     force_64,
                      AP4_UI08 version, 
                      AP4_UI32 flags);

    // destructor
    virtual ~AP4_Atom() {}
    
    // methods
    AP4_UI32           GetFlags() const { return m_Flags; }
    void               SetFlags(AP4_UI32 flags) { m_Flags = flags; }
    AP4_UI08           GetVersion() const {return m_Version;}
    void               SetVersion(AP4_UI08 version) { m_Version = version; }
    Type               GetType() const { return m_Type; }
    void               SetType(Type type) { m_Type = type; }
    virtual AP4_Size   GetHeaderSize() const;
    AP4_UI64           GetSize() const { return m_Size32 == 1?m_Size64:m_Size32; }
    void               SetSize(AP4_UI64 size, bool force_64 = false);
    AP4_UI32           GetSize32() const { return m_Size32; }
    void               SetSize32(AP4_UI32 size) { m_Size32 = size; }
    AP4_UI64           GetSize64() const { return m_Size64; }
    void               SetSize64(AP4_UI64 size) { m_Size64 = size; }
    virtual AP4_Result Write(AP4_ByteStream& stream);
    virtual AP4_Result WriteHeader(AP4_ByteStream& stream);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream) = 0;
    virtual AP4_Result Inspect(AP4_AtomInspector& inspector);
    virtual AP4_Result InspectHeader(AP4_AtomInspector& inspector);
    virtual AP4_Result InspectFields(AP4_AtomInspector& /* inspector */) {
        return AP4_SUCCESS; 
    }

    // parent/child relationship methods
    virtual AP4_Result SetParent(AP4_AtomParent* parent) {
        m_Parent = parent;
        return AP4_SUCCESS;
    }
    virtual AP4_AtomParent* GetParent() { return m_Parent; }
    virtual AP4_Result      Detach();

    /**
     * Create a clone of the object.
     * This method returns a clone of the atom, or NULL if
     * the atom cannot be cloned.
     * Override this if your want to make an atom cloneable in a more
     * efficient way than the default implementation.
     */ 
    virtual AP4_Atom* Clone();

 protected:
    // members
    Type            m_Type;
    AP4_UI32        m_Size32; 
    AP4_UI64        m_Size64; // this is 0 if m_Size is not 1 (encoded in 32-bits)
                              // and non-zero only if m_Size is 1 (encoded in 64-bits)
    bool            m_IsFull;
    AP4_UI08        m_Version;
    AP4_UI32        m_Flags;
    AP4_AtomParent* m_Parent;
};

/*----------------------------------------------------------------------
|   AP4_AtomParent
+---------------------------------------------------------------------*/
/**
 * Base class for containers of atoms.
 * This class also implements the logic for finding descendents by name.
 */
class AP4_AtomParent {
public:
    AP4_IMPLEMENT_DYNAMIC_CAST(AP4_AtomParent)

    // base methods
    virtual ~AP4_AtomParent();
    AP4_List<AP4_Atom>& GetChildren() { return m_Children; }
    AP4_Result          CopyChildren(AP4_AtomParent& destination) const;
    virtual AP4_Result  AddChild(AP4_Atom* child, int position = -1);
    virtual AP4_Result  RemoveChild(AP4_Atom* child);
    virtual AP4_Result  DeleteChild(AP4_Atom::Type type, AP4_Ordinal index = 0);
    virtual AP4_Atom*   GetChild(AP4_Atom::Type type, AP4_Ordinal index = 0) const;
    virtual AP4_Atom*   GetChild(const AP4_UI08* uuid, AP4_Ordinal index = 0) const;
    virtual AP4_Atom*   FindChild(const char* path, 
                                  bool        auto_create = false,
                                  bool        auto_create_full = false);

    // methods designed to be overridden
    virtual void OnChildChanged(AP4_Atom* /* child */) {}
    virtual void OnChildAdded(AP4_Atom* /* child */)   {}
    virtual void OnChildRemoved(AP4_Atom* /* child */) {}

protected:
    // members
    AP4_List<AP4_Atom> m_Children;
};

/*----------------------------------------------------------------------
|   AP4_UnknownAtom
+---------------------------------------------------------------------*/
/**
 * Class that represents atoms for which there is no specific support.
 * Instances of this class keep a reference to the stream from which 
 * the atom is parsed, so that it can read the atom's payload when it
 * is serialized.
 * If the atom is small, its payload is actually read and stored in 
 * a data buffer, so no reference to the source stream is kept
 */
class AP4_UnknownAtom : public AP4_Atom {
public:
    // constructor and destructor
    AP4_UnknownAtom(AP4_Atom::Type   type, 
                    AP4_UI64         size, 
                    AP4_ByteStream&  stream);
    AP4_UnknownAtom(AP4_Atom::Type type, const AP4_UI08* payload, AP4_Size payload_size);
    AP4_UnknownAtom(const AP4_UnknownAtom& other);
    ~AP4_UnknownAtom();

    // methods
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);
    virtual AP4_Atom*  Clone();

private:
    // members
    AP4_ByteStream* m_SourceStream;
    AP4_Position    m_SourcePosition;
    AP4_DataBuffer  m_Payload;
};

/*----------------------------------------------------------------------
|   AP4_NullTerminatedStringAtom
+---------------------------------------------------------------------*/
/**
 * Generic Class usd for all atoms that contain a single null-terminated
 * string.
 */
class AP4_NullTerminatedStringAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_NullTerminatedStringAtom, AP4_Atom)

    // constructors
    AP4_NullTerminatedStringAtom(AP4_Atom::Type type, AP4_UI64 size, AP4_ByteStream& stream);
    AP4_NullTerminatedStringAtom(AP4_Atom::Type type, const char* value);

    // accessors
    const AP4_String& GetValue() { return m_Value; }
    
    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

private:
    // members
    AP4_String m_Value;
};

/*----------------------------------------------------------------------
|   atom types
+---------------------------------------------------------------------*/
const AP4_Atom::Type AP4_ATOM_TYPE_UDTA = AP4_ATOM_TYPE('u','d','t','a');
const AP4_Atom::Type AP4_ATOM_TYPE_URL  = AP4_ATOM_TYPE('u','r','l',' ');
const AP4_Atom::Type AP4_ATOM_TYPE_TRAK = AP4_ATOM_TYPE('t','r','a','k');
const AP4_Atom::Type AP4_ATOM_TYPE_TRAF = AP4_ATOM_TYPE('t','r','a','f');
const AP4_Atom::Type AP4_ATOM_TYPE_TKHD = AP4_ATOM_TYPE('t','k','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_TFHD = AP4_ATOM_TYPE('t','f','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_TRUN = AP4_ATOM_TYPE('t','r','u','n');
const AP4_Atom::Type AP4_ATOM_TYPE_STTS = AP4_ATOM_TYPE('s','t','t','s');
const AP4_Atom::Type AP4_ATOM_TYPE_STSZ = AP4_ATOM_TYPE('s','t','s','z');
const AP4_Atom::Type AP4_ATOM_TYPE_STZ2 = AP4_ATOM_TYPE('s','t','z','2');
const AP4_Atom::Type AP4_ATOM_TYPE_STSS = AP4_ATOM_TYPE('s','t','s','s');
const AP4_Atom::Type AP4_ATOM_TYPE_STSD = AP4_ATOM_TYPE('s','t','s','d');
const AP4_Atom::Type AP4_ATOM_TYPE_STSC = AP4_ATOM_TYPE('s','t','s','c');
const AP4_Atom::Type AP4_ATOM_TYPE_STCO = AP4_ATOM_TYPE('s','t','c','o');
const AP4_Atom::Type AP4_ATOM_TYPE_CO64 = AP4_ATOM_TYPE('c','o','6','4');
const AP4_Atom::Type AP4_ATOM_TYPE_STBL = AP4_ATOM_TYPE('s','t','b','l');
const AP4_Atom::Type AP4_ATOM_TYPE_SINF = AP4_ATOM_TYPE('s','i','n','f');
const AP4_Atom::Type AP4_ATOM_TYPE_SCHM = AP4_ATOM_TYPE('s','c','h','m');
const AP4_Atom::Type AP4_ATOM_TYPE_SCHI = AP4_ATOM_TYPE('s','c','h','i');
const AP4_Atom::Type AP4_ATOM_TYPE_MVHD = AP4_ATOM_TYPE('m','v','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_MEHD = AP4_ATOM_TYPE('m','e','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_MP4S = AP4_ATOM_TYPE('m','p','4','s');
const AP4_Atom::Type AP4_ATOM_TYPE_MP4A = AP4_ATOM_TYPE('m','p','4','a');
const AP4_Atom::Type AP4_ATOM_TYPE_MP4V = AP4_ATOM_TYPE('m','p','4','v');
const AP4_Atom::Type AP4_ATOM_TYPE_AVC1 = AP4_ATOM_TYPE('a','v','c','1');
const AP4_Atom::Type AP4_ATOM_TYPE_AVC2 = AP4_ATOM_TYPE('a','v','c','2');
const AP4_Atom::Type AP4_ATOM_TYPE_AVC3 = AP4_ATOM_TYPE('a','v','c','3');
const AP4_Atom::Type AP4_ATOM_TYPE_AVC4 = AP4_ATOM_TYPE('a','v','c','4');
const AP4_Atom::Type AP4_ATOM_TYPE_DVAV = AP4_ATOM_TYPE('d','v','a','v');
const AP4_Atom::Type AP4_ATOM_TYPE_DVA1 = AP4_ATOM_TYPE('d','v','a','1');
const AP4_Atom::Type AP4_ATOM_TYPE_HEV1 = AP4_ATOM_TYPE('h','e','v','1');
const AP4_Atom::Type AP4_ATOM_TYPE_HVC1 = AP4_ATOM_TYPE('h','v','c','1');
const AP4_Atom::Type AP4_ATOM_TYPE_DVHE = AP4_ATOM_TYPE('d','v','h','e');
const AP4_Atom::Type AP4_ATOM_TYPE_DVH1 = AP4_ATOM_TYPE('d','v','h','1');
const AP4_Atom::Type AP4_ATOM_TYPE_VP08 = AP4_ATOM_TYPE('v','p','0','8');
const AP4_Atom::Type AP4_ATOM_TYPE_VP09 = AP4_ATOM_TYPE('v','p','0','9');
const AP4_Atom::Type AP4_ATOM_TYPE_VP10 = AP4_ATOM_TYPE('v','p','1','0');
const AP4_Atom::Type AP4_ATOM_TYPE_AV01 = AP4_ATOM_TYPE('a','v','0','1');
const AP4_Atom::Type AP4_ATOM_TYPE_ALAC = AP4_ATOM_TYPE('a','l','a','c');
const AP4_Atom::Type AP4_ATOM_TYPE_ENCA = AP4_ATOM_TYPE('e','n','c','a');
const AP4_Atom::Type AP4_ATOM_TYPE_ENCV = AP4_ATOM_TYPE('e','n','c','v');
const AP4_Atom::Type AP4_ATOM_TYPE_MOOV = AP4_ATOM_TYPE('m','o','o','v');
const AP4_Atom::Type AP4_ATOM_TYPE_MOOF = AP4_ATOM_TYPE('m','o','o','f');
const AP4_Atom::Type AP4_ATOM_TYPE_MVEX = AP4_ATOM_TYPE('m','v','e','x');
const AP4_Atom::Type AP4_ATOM_TYPE_TREX = AP4_ATOM_TYPE('t','r','e','x');
const AP4_Atom::Type AP4_ATOM_TYPE_MINF = AP4_ATOM_TYPE('m','i','n','f');
const AP4_Atom::Type AP4_ATOM_TYPE_META = AP4_ATOM_TYPE('m','e','t','a');
const AP4_Atom::Type AP4_ATOM_TYPE_MDHD = AP4_ATOM_TYPE('m','d','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_MFHD = AP4_ATOM_TYPE('m','f','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_ILST = AP4_ATOM_TYPE('i','l','s','t');
const AP4_Atom::Type AP4_ATOM_TYPE_HDLR = AP4_ATOM_TYPE('h','d','l','r');
const AP4_Atom::Type AP4_ATOM_TYPE_FTYP = AP4_ATOM_TYPE('f','t','y','p');
const AP4_Atom::Type AP4_ATOM_TYPE_IODS = AP4_ATOM_TYPE('i','o','d','s');
const AP4_Atom::Type AP4_ATOM_TYPE_ESDS = AP4_ATOM_TYPE('e','s','d','s');
const AP4_Atom::Type AP4_ATOM_TYPE_EDTS = AP4_ATOM_TYPE('e','d','t','s');
const AP4_Atom::Type AP4_ATOM_TYPE_DRMS = AP4_ATOM_TYPE('d','r','m','s');
const AP4_Atom::Type AP4_ATOM_TYPE_DRMI = AP4_ATOM_TYPE('d','r','m','i');
const AP4_Atom::Type AP4_ATOM_TYPE_DREF = AP4_ATOM_TYPE('d','r','e','f');
const AP4_Atom::Type AP4_ATOM_TYPE_DINF = AP4_ATOM_TYPE('d','i','n','f');
const AP4_Atom::Type AP4_ATOM_TYPE_CTTS = AP4_ATOM_TYPE('c','t','t','s');
const AP4_Atom::Type AP4_ATOM_TYPE_MDIA = AP4_ATOM_TYPE('m','d','i','a');
const AP4_Atom::Type AP4_ATOM_TYPE_ELST = AP4_ATOM_TYPE('e','l','s','t');
const AP4_Atom::Type AP4_ATOM_TYPE_VMHD = AP4_ATOM_TYPE('v','m','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_SMHD = AP4_ATOM_TYPE('s','m','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_NMHD = AP4_ATOM_TYPE('n','m','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_STHD = AP4_ATOM_TYPE('s','t','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_HMHD = AP4_ATOM_TYPE('h','m','h','d');
const AP4_Atom::Type AP4_ATOM_TYPE_FRMA = AP4_ATOM_TYPE('f','r','m','a');
const AP4_Atom::Type AP4_ATOM_TYPE_MDAT = AP4_ATOM_TYPE('m','d','a','t');
const AP4_Atom::Type AP4_ATOM_TYPE_FREE = AP4_ATOM_TYPE('f','r','e','e');
const AP4_Atom::Type AP4_ATOM_TYPE_TIMS = AP4_ATOM_TYPE('t','i','m','s');
const AP4_Atom::Type AP4_ATOM_TYPE_RTP_ = AP4_ATOM_TYPE('r','t','p',' ');
const AP4_Atom::Type AP4_ATOM_TYPE_HNTI = AP4_ATOM_TYPE('h','n','t','i');
const AP4_Atom::Type AP4_ATOM_TYPE_SDP_ = AP4_ATOM_TYPE('s','d','p',' ');
const AP4_Atom::Type AP4_ATOM_TYPE_IKMS = AP4_ATOM_TYPE('i','K','M','S');
const AP4_Atom::Type AP4_ATOM_TYPE_ISFM = AP4_ATOM_TYPE('i','S','F','M');
const AP4_Atom::Type AP4_ATOM_TYPE_ISLT = AP4_ATOM_TYPE('i','S','L','T');
const AP4_Atom::Type AP4_ATOM_TYPE_TREF = AP4_ATOM_TYPE('t','r','e','f');
const AP4_Atom::Type AP4_ATOM_TYPE_HINT = AP4_ATOM_TYPE('h','i','n','t');
const AP4_Atom::Type AP4_ATOM_TYPE_CDSC = AP4_ATOM_TYPE('c','d','s','c');
const AP4_Atom::Type AP4_ATOM_TYPE_MPOD = AP4_ATOM_TYPE('m','p','o','d');
const AP4_Atom::Type AP4_ATOM_TYPE_IPIR = AP4_ATOM_TYPE('i','p','i','r');
const AP4_Atom::Type AP4_ATOM_TYPE_CHAP = AP4_ATOM_TYPE('c','h','a','p');
const AP4_Atom::Type AP4_ATOM_TYPE_ALIS = AP4_ATOM_TYPE('a','l','i','s');
const AP4_Atom::Type AP4_ATOM_TYPE_SYNC = AP4_ATOM_TYPE('s','y','n','c');
const AP4_Atom::Type AP4_ATOM_TYPE_DPND = AP4_ATOM_TYPE('d','p','n','d');
const AP4_Atom::Type AP4_ATOM_TYPE_ODRM = AP4_ATOM_TYPE('o','d','r','m');
const AP4_Atom::Type AP4_ATOM_TYPE_ODKM = AP4_ATOM_TYPE('o','d','k','m');
const AP4_Atom::Type AP4_ATOM_TYPE_OHDR = AP4_ATOM_TYPE('o','h','d','r');
const AP4_Atom::Type AP4_ATOM_TYPE_ODDA = AP4_ATOM_TYPE('o','d','d','a');
const AP4_Atom::Type AP4_ATOM_TYPE_ODHE = AP4_ATOM_TYPE('o','d','h','e');
const AP4_Atom::Type AP4_ATOM_TYPE_ODAF = AP4_ATOM_TYPE('o','d','a','f');
const AP4_Atom::Type AP4_ATOM_TYPE_GRPI = AP4_ATOM_TYPE('g','r','p','i');
const AP4_Atom::Type AP4_ATOM_TYPE_IPRO = AP4_ATOM_TYPE('i','p','r','o');
const AP4_Atom::Type AP4_ATOM_TYPE_MDRI = AP4_ATOM_TYPE('m','d','r','i');
const AP4_Atom::Type AP4_ATOM_TYPE_AVCC = AP4_ATOM_TYPE('a','v','c','C');
const AP4_Atom::Type AP4_ATOM_TYPE_HVCC = AP4_ATOM_TYPE('h','v','c','C');
const AP4_Atom::Type AP4_ATOM_TYPE_DVCC = AP4_ATOM_TYPE('d','v','c','C');
const AP4_Atom::Type AP4_ATOM_TYPE_VPCC = AP4_ATOM_TYPE('v','p','c','C');
const AP4_Atom::Type AP4_ATOM_TYPE_DVVC = AP4_ATOM_TYPE('d','v','v','C');
const AP4_Atom::Type AP4_ATOM_TYPE_HVCE = AP4_ATOM_TYPE('h','v','c','E');
const AP4_Atom::Type AP4_ATOM_TYPE_AVCE = AP4_ATOM_TYPE('a','v','c','E');
const AP4_Atom::Type AP4_ATOM_TYPE_AV1C = AP4_ATOM_TYPE('a','v','1','C');
const AP4_Atom::Type AP4_ATOM_TYPE_WAVE = AP4_ATOM_TYPE('w','a','v','e');
const AP4_Atom::Type AP4_ATOM_TYPE_WIDE = AP4_ATOM_TYPE('w','i','d','e');
const AP4_Atom::Type AP4_ATOM_TYPE_UUID = AP4_ATOM_TYPE('u','u','i','d');
const AP4_Atom::Type AP4_ATOM_TYPE_8ID_ = AP4_ATOM_TYPE('8','i','d',' ');
const AP4_Atom::Type AP4_ATOM_TYPE_8BDL = AP4_ATOM_TYPE('8','b','d','l');
const AP4_Atom::Type AP4_ATOM_TYPE_AC_3 = AP4_ATOM_TYPE('a','c','-','3');
const AP4_Atom::Type AP4_ATOM_TYPE_EC_3 = AP4_ATOM_TYPE('e','c','-','3');
const AP4_Atom::Type AP4_ATOM_TYPE_AC_4 = AP4_ATOM_TYPE('a','c','-','4');
const AP4_Atom::Type AP4_ATOM_TYPE_DTSC = AP4_ATOM_TYPE('d','t','s','c');
const AP4_Atom::Type AP4_ATOM_TYPE_DTSH = AP4_ATOM_TYPE('d','t','s','h');
const AP4_Atom::Type AP4_ATOM_TYPE_DTSL = AP4_ATOM_TYPE('d','t','s','l');
const AP4_Atom::Type AP4_ATOM_TYPE_DTSE = AP4_ATOM_TYPE('d','t','s','e');
const AP4_Atom::Type AP4_ATOM_TYPE_FLAC = AP4_ATOM_TYPE('f','L','a','C');
const AP4_Atom::Type AP4_ATOM_TYPE_OPUS = AP4_ATOM_TYPE('O','p','u','s');
const AP4_Atom::Type AP4_ATOM_TYPE_MFRA = AP4_ATOM_TYPE('m','f','r','a');
const AP4_Atom::Type AP4_ATOM_TYPE_TFRA = AP4_ATOM_TYPE('t','f','r','a');
const AP4_Atom::Type AP4_ATOM_TYPE_MFRO = AP4_ATOM_TYPE('m','f','r','o');
const AP4_Atom::Type AP4_ATOM_TYPE_TFDT = AP4_ATOM_TYPE('t','f','d','t');
const AP4_Atom::Type AP4_ATOM_TYPE_TENC = AP4_ATOM_TYPE('t','e','n','c');
const AP4_Atom::Type AP4_ATOM_TYPE_SENC = AP4_ATOM_TYPE('s','e','n','c');
const AP4_Atom::Type AP4_ATOM_TYPE_SAIO = AP4_ATOM_TYPE('s','a','i','o');
const AP4_Atom::Type AP4_ATOM_TYPE_SAIZ = AP4_ATOM_TYPE('s','a','i','z');
const AP4_Atom::Type AP4_ATOM_TYPE_PDIN = AP4_ATOM_TYPE('p','d','i','n');
const AP4_Atom::Type AP4_ATOM_TYPE_BLOC = AP4_ATOM_TYPE('b','l','o','c');
const AP4_Atom::Type AP4_ATOM_TYPE_AINF = AP4_ATOM_TYPE('a','i','n','f');
const AP4_Atom::Type AP4_ATOM_TYPE_PSSH = AP4_ATOM_TYPE('p','s','s','h');
const AP4_Atom::Type AP4_ATOM_TYPE_MARL = AP4_ATOM_TYPE('m','a','r','l');
const AP4_Atom::Type AP4_ATOM_TYPE_MKID = AP4_ATOM_TYPE('m','k','i','d');
const AP4_Atom::Type AP4_ATOM_TYPE_PRFT = AP4_ATOM_TYPE('p','r','f','t');
const AP4_Atom::Type AP4_ATOM_TYPE_STPP = AP4_ATOM_TYPE('s','t','p','p');
const AP4_Atom::Type AP4_ATOM_TYPE_DAC3 = AP4_ATOM_TYPE('d','a','c','3');
const AP4_Atom::Type AP4_ATOM_TYPE_DEC3 = AP4_ATOM_TYPE('d','e','c','3');
const AP4_Atom::Type AP4_ATOM_TYPE_DAC4 = AP4_ATOM_TYPE('d','a','c','4');
const AP4_Atom::Type AP4_ATOM_TYPE_SIDX = AP4_ATOM_TYPE('s','i','d','x');
const AP4_Atom::Type AP4_ATOM_TYPE_SSIX = AP4_ATOM_TYPE('s','s','i','x');
const AP4_Atom::Type AP4_ATOM_TYPE_SBGP = AP4_ATOM_TYPE('s','b','g','p');
const AP4_Atom::Type AP4_ATOM_TYPE_SGPD = AP4_ATOM_TYPE('s','g','p','d');
const AP4_Atom::Type AP4_ATOM_TYPE_XML  = AP4_ATOM_TYPE('x','m','l',' ');

/*----------------------------------------------------------------------
|   AP4_AtomListInspector
+---------------------------------------------------------------------*/
class AP4_AtomListInspector : public AP4_List<AP4_Atom>::Item::Operator
{
 public:
    AP4_AtomListInspector(AP4_AtomInspector& inspector) :
        m_Inspector(inspector) {}
    AP4_Result Action(AP4_Atom* atom) const {
        atom->Inspect(m_Inspector);
        return AP4_SUCCESS;
    }

 private:
    AP4_AtomInspector& m_Inspector;
};

/*----------------------------------------------------------------------
|   AP4_AtomListWriter
+---------------------------------------------------------------------*/
class AP4_AtomListWriter : public AP4_List<AP4_Atom>::Item::Operator
{
 public:
    AP4_AtomListWriter(AP4_ByteStream& stream) :
        m_Stream(stream) {}
    AP4_Result Action(AP4_Atom* atom) const;

 private:
    AP4_ByteStream& m_Stream;
};

/*----------------------------------------------------------------------
|   AP4_AtomFinder
+---------------------------------------------------------------------*/
class AP4_AtomFinder : public AP4_List<AP4_Atom>::Item::Finder
{
 public:
    AP4_AtomFinder(AP4_Atom::Type type, AP4_Ordinal index = 0) : 
       m_Type(type), m_Index(index) {}
    AP4_Result Test(AP4_Atom* atom) const {
        if (atom->GetType() == m_Type) {
            if (m_Index-- == 0) {
                return AP4_SUCCESS;
            } else {
                return AP4_FAILURE;
            }
        } else {
            return AP4_FAILURE;
        }
    }
 private:
    AP4_Atom::Type      m_Type;
    mutable AP4_Ordinal m_Index;
};

/*----------------------------------------------------------------------
|   AP4_AtomSizeAdder
+---------------------------------------------------------------------*/
class AP4_AtomSizeAdder : public AP4_List<AP4_Atom>::Item::Operator {
public:
    AP4_AtomSizeAdder(AP4_UI64& size) : m_Size(size) {}

private:
    AP4_Result Action(AP4_Atom* atom) const {
        m_Size += atom->GetSize();
        return AP4_SUCCESS;
    }
    AP4_UI64& m_Size;
};

#endif // _AP4_ATOM_H_
