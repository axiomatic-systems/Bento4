/*****************************************************************
|
|    AP4 - Protected Streams support
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf & Julien Boeuf
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

#ifndef _AP4_PROTECTION_H_
#define _AP4_PROTECTION_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4SampleEntry.h"
#include "Ap4Atom.h"
#include "Ap4AtomFactory.h"
#include "Ap4SampleDescription.h"
#include "Ap4Processor.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
// this is fixed for now
const unsigned int AP4_PROTECTION_KEY_LENGTH = 16;

/*----------------------------------------------------------------------
|   AP4_EncaSampleEntry
+---------------------------------------------------------------------*/
class AP4_EncaSampleEntry : public AP4_AudioSampleEntry
{
public:
    // methods
    AP4_EncaSampleEntry(AP4_Size         size,
                        AP4_ByteStream&  stream,
                        AP4_AtomFactory& atom_factory);
    AP4_EncaSampleEntry(AP4_UI32         type,
                        AP4_Size         size,
                        AP4_ByteStream&  stream,
                        AP4_AtomFactory& atom_factory);

    // methods
    AP4_SampleDescription* ToSampleDescription();
};

/*----------------------------------------------------------------------
|   AP4_EncvSampleEntry
+---------------------------------------------------------------------*/
class AP4_EncvSampleEntry : public AP4_VisualSampleEntry
{
public:
    // constructors
    AP4_EncvSampleEntry(AP4_Size         size,
                        AP4_ByteStream&  stream,
                        AP4_AtomFactory& atom_factory);
    AP4_EncvSampleEntry(AP4_UI32         type,
                        AP4_Size         size,
                        AP4_ByteStream&  stream,
                        AP4_AtomFactory& atom_factory);

    // methods
    AP4_SampleDescription* ToSampleDescription();
};

/*----------------------------------------------------------------------
|   AP4_DrmsSampleEntry
+---------------------------------------------------------------------*/
class AP4_DrmsSampleEntry : public AP4_EncaSampleEntry
{
public:
    // methods
    AP4_DrmsSampleEntry(AP4_Size         size,
                        AP4_ByteStream&  stream,
                        AP4_AtomFactory& atom_factory);
};

/*----------------------------------------------------------------------
|   AP4_DrmiSampleEntry
+---------------------------------------------------------------------*/
class AP4_DrmiSampleEntry : public AP4_EncvSampleEntry
{
public:
    // methods
    AP4_DrmiSampleEntry(AP4_Size         size,
                        AP4_ByteStream&  stream,
                        AP4_AtomFactory& atom_factory);
};

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap
+---------------------------------------------------------------------*/
class AP4_ProtectionKeyMap
{
public:
    // constructors and destructor
    AP4_ProtectionKeyMap();
    ~AP4_ProtectionKeyMap();

    // methods
    AP4_Result      SetKey(AP4_UI32 track_id, const AP4_UI08* key, const AP4_UI08* iv = NULL);
    AP4_Result      SetKeys(const AP4_ProtectionKeyMap& key_map);
    AP4_Result      GetKeyAndIv(AP4_UI32 track_id, const AP4_UI08*& key, const AP4_UI08*& iv);
    const AP4_UI08* GetKey(AP4_UI32 track_id);

private:
    // types
    class KeyEntry {
    public:
        KeyEntry(AP4_UI32 track_id, const AP4_UI08* key, const AP4_UI08* iv = NULL);
        void SetKey(const AP4_UI08* key, const AP4_UI08* iv);
        AP4_Ordinal m_TrackId;
        AP4_UI08    m_Key[AP4_PROTECTION_KEY_LENGTH];
        AP4_UI08    m_IV[AP4_PROTECTION_KEY_LENGTH];
    };

    // methods
    KeyEntry* GetEntry(AP4_UI32 track_id);

    // members
    AP4_List<KeyEntry> m_KeyEntries;
};

/*----------------------------------------------------------------------
|   AP4_ProtectionSchemeInfo
+---------------------------------------------------------------------*/
class AP4_ProtectionSchemeInfo
{
public:
    // constructors and destructor
    AP4_ProtectionSchemeInfo(AP4_ContainerAtom* schi);
    virtual ~AP4_ProtectionSchemeInfo(){}

    // accessors
    AP4_ContainerAtom& GetSchiAtom() { return m_SchiAtom; }

protected:
    AP4_ContainerAtom m_SchiAtom;
};

/*----------------------------------------------------------------------
|   AP4_ProtectedSampleDescription
+---------------------------------------------------------------------*/
class AP4_ProtectedSampleDescription : public AP4_SampleDescription
{
public:
    // constructor and destructor
    AP4_ProtectedSampleDescription(AP4_UI32               format,
                                   AP4_SampleDescription* original_sample_description,
                                   AP4_UI32               original_format,
                                   AP4_UI32               scheme_type,
                                   AP4_UI32               scheme_version,
                                   const char*            scheme_uri,
                                   AP4_ContainerAtom*     schi_atom);
    ~AP4_ProtectedSampleDescription();
    
    // accessors
    AP4_SampleDescription* GetOriginalSampleDescription() {
        return m_OriginalSampleDescription;
    }
    AP4_UI32          GetOriginalFormat() const { return m_OriginalFormat; }
    AP4_UI32          GetSchemeType()     const { return m_SchemeType;     }
    AP4_UI32          GetSchemeVersion()  const { return m_SchemeVersion;  }
    const AP4_String& GetSchemeUri()      const { return m_SchemeUri;      }
    AP4_ProtectionSchemeInfo* GetSchemeInfo() const { 
        return m_SchemeInfo; 
    }

    // implementation of abstract base class methods
    virtual AP4_Atom* ToAtom() const;

private:
    // members
    AP4_SampleDescription*    m_OriginalSampleDescription;
    AP4_UI32                  m_OriginalFormat;
    AP4_UI32                  m_SchemeType;
    AP4_UI32                  m_SchemeVersion;
    AP4_String                m_SchemeUri;
    AP4_ProtectionSchemeInfo* m_SchemeInfo;
};

/*----------------------------------------------------------------------
|   AP4_StandardDecryptingProcessor
+---------------------------------------------------------------------*/
class AP4_StandardDecryptingProcessor : public AP4_Processor
{
public:
    // accessors
    AP4_ProtectionKeyMap& GetKeyMap() { return m_KeyMap; }

    // methods
    virtual AP4_Processor::TrackHandler* CreateTrackHandler(AP4_TrakAtom* trak);

private:
    // members
    AP4_ProtectionKeyMap m_KeyMap;
};

#endif // _AP4_PROTECTION_H_
