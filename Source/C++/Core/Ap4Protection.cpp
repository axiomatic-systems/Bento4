/*****************************************************************
|
|    AP4 - Protected Stream Support
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
#include "Ap4Protection.h"
#include "Ap4SchmAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4Sample.h"
#include "Ap4StreamCipher.h"
#include "Ap4IsfmAtom.h"
#include "Ap4FrmaAtom.h"
#include "Ap4IkmsAtom.h"
#include "Ap4IsfmAtom.h"
#include "Ap4IsltAtom.h"
#include "Ap4Utils.h"
#include "Ap4TrakAtom.h"
#include "Ap4IsmaCryp.h"
#include "Ap4OmaDcf.h"

/*----------------------------------------------------------------------
|   AP4_EncaSampleEntry::AP4_EncaSampleEntry
+---------------------------------------------------------------------*/
AP4_EncaSampleEntry::AP4_EncaSampleEntry(AP4_UI32         type,
                                         AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_AudioSampleEntry(type, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_EncaSampleEntry::AP4_EncaSampleEntry
+---------------------------------------------------------------------*/
AP4_EncaSampleEntry::AP4_EncaSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_AudioSampleEntry(AP4_ATOM_TYPE_ENCA, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_EncaSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_EncaSampleEntry::ToSampleDescription()
{
    // get the original sample format
    AP4_FrmaAtom* frma = (AP4_FrmaAtom*)FindChild("sinf/frma");

    // get the schi atom
    AP4_ContainerAtom* schi;
    schi = static_cast<AP4_ContainerAtom*>(FindChild("sinf/schi"));

    // get the scheme info
    AP4_SchmAtom* schm = (AP4_SchmAtom*)FindChild("sinf/schm");
    AP4_UI32 original_format = frma?frma->GetOriginalFormat():0;
    if (schm) {
        // create the original sample description
        return new AP4_ProtectedSampleDescription(
            m_Type,
            ToTargetSampleDescription(original_format),
            original_format,
            schm->GetSchemeType(),
            schm->GetSchemeVersion(),
            schm->GetSchemeUri().GetChars(),
            schi);
    } else if (schi) {
        // try to see if we can guess the protection scheme from the 'schi' contents
        AP4_Atom* odkm = schi->GetChild(AP4_ATOM_TYPE_ODKM);
        if (odkm) {
            // create the original sample description
            return new AP4_ProtectedSampleDescription(
                m_Type,
                ToTargetSampleDescription(original_format),
                original_format,
                AP4_PROTECTION_SCHEME_TYPE_OMA,
                AP4_PROTECTION_SCHEME_VERSION_OMA_20,
                NULL,
                schi);
        }
    }

    // unknown scheme
    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_EncvSampleEntry::AP4_EncvSampleEntry
+---------------------------------------------------------------------*/
AP4_EncvSampleEntry::AP4_EncvSampleEntry(AP4_UI32         type,
                                         AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_VisualSampleEntry(type, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_EncvSampleEntry::AP4_EncvSampleEntry
+---------------------------------------------------------------------*/
AP4_EncvSampleEntry::AP4_EncvSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_VisualSampleEntry(AP4_ATOM_TYPE_ENCV, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_EncvSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_EncvSampleEntry::ToSampleDescription()
{
    // get the original sample format
    AP4_FrmaAtom* frma = (AP4_FrmaAtom*)FindChild("sinf/frma");

    // get the schi atom
    AP4_ContainerAtom* schi;
    schi = static_cast<AP4_ContainerAtom*>(FindChild("sinf/schi"));

    // get the scheme info
    AP4_SchmAtom* schm = (AP4_SchmAtom*)FindChild("sinf/schm");
    AP4_UI32 original_format = frma?frma->GetOriginalFormat():AP4_ATOM_TYPE_MP4V;
    if (schm) {
        // create the sample description
        return new AP4_ProtectedSampleDescription(
            m_Type,
            ToTargetSampleDescription(original_format),
            original_format,
            schm->GetSchemeType(),
            schm->GetSchemeVersion(),
            schm->GetSchemeUri().GetChars(),
            schi);
    } else if (schi) {
        // try to see if we can guess the protection scheme from the 'schi' contents
        AP4_Atom* odkm = schi->GetChild(AP4_ATOM_TYPE_ODKM);
        if (odkm) {
            // create the original sample description
            return new AP4_ProtectedSampleDescription(
                m_Type,
                ToTargetSampleDescription(original_format),
                original_format,
                AP4_PROTECTION_SCHEME_TYPE_OMA,
                AP4_PROTECTION_SCHEME_VERSION_OMA_20,
                NULL,
                schi);
        }
    }

    // unknown scheme
    return NULL;

}

/*----------------------------------------------------------------------
|   AP4_DrmsSampleEntry::AP4_DrmsSampleEntry
+---------------------------------------------------------------------*/
AP4_DrmsSampleEntry::AP4_DrmsSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_EncaSampleEntry(AP4_ATOM_TYPE_DRMS, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_DrmiSampleEntry::AP4_DrmiSampleEntry
+---------------------------------------------------------------------*/
AP4_DrmiSampleEntry::AP4_DrmiSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_EncvSampleEntry(AP4_ATOM_TYPE_DRMI, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|   AP4_ProtectionSchemeInfo::~AP4_ProtectionSchemeInfo
+---------------------------------------------------------------------*/
AP4_ProtectionSchemeInfo::~AP4_ProtectionSchemeInfo()
{
    delete m_SchiAtom;
}

/*----------------------------------------------------------------------
|   AP4_ProtectionSchemeInfo::AP4_ProtectionSchemeInfo
+---------------------------------------------------------------------*/
AP4_ProtectionSchemeInfo::AP4_ProtectionSchemeInfo(AP4_ContainerAtom* schi)
{
    if (schi) {
        m_SchiAtom = (AP4_ContainerAtom*)schi->Clone();
    } else {
        m_SchiAtom = NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::AP4_ProtectionKeyMap
+---------------------------------------------------------------------*/
AP4_ProtectionKeyMap::AP4_ProtectionKeyMap()
{
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::~AP4_ProtectionKeyMap
+---------------------------------------------------------------------*/
AP4_ProtectionKeyMap::~AP4_ProtectionKeyMap()
{
    m_KeyEntries.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::SetKey
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ProtectionKeyMap::SetKey(AP4_UI32 track_id, const AP4_UI08* key, const AP4_UI08* iv)
{
    KeyEntry* entry = GetEntry(track_id);
    if (entry == NULL) {
        m_KeyEntries.Add(new KeyEntry(track_id, key, iv));
    } else {
        entry->SetKey(key, iv);
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::SetKey
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ProtectionKeyMap::SetKeys(const AP4_ProtectionKeyMap& key_map)
{
    AP4_List<KeyEntry>::Item* item = key_map.m_KeyEntries.FirstItem();
    while (item) {
        KeyEntry* entry = item->GetData();
        m_KeyEntries.Add(new KeyEntry(entry->m_TrackId,
                                      entry->m_Key,
                                      entry->m_IV));
        item = item->GetNext();
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::GetKeyIv
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ProtectionKeyMap::GetKeyAndIv(AP4_UI32         track_id, 
                                  const AP4_UI08*& key, 
                                  const AP4_UI08*& iv)
{
    KeyEntry* entry = GetEntry(track_id);
    if (entry) {
        key = entry->m_Key;
        iv = entry->m_IV;
        return AP4_SUCCESS;
    } else {
        return AP4_ERROR_NO_SUCH_ITEM;
    }
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::GetKey
+---------------------------------------------------------------------*/
const AP4_UI08* 
AP4_ProtectionKeyMap::GetKey(AP4_UI32 track_id) const
{
    KeyEntry* entry = GetEntry(track_id);
    if (entry) {
        return entry->m_Key;
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::GetEntry
+---------------------------------------------------------------------*/
AP4_ProtectionKeyMap::KeyEntry*
AP4_ProtectionKeyMap::GetEntry(AP4_UI32 track_id) const
{
    AP4_List<KeyEntry>::Item* item = m_KeyEntries.FirstItem();
    while (item) {
        KeyEntry* entry = (KeyEntry*)item->GetData();
        if (entry->m_TrackId == track_id) return entry;
        item = item->GetNext();
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::KeyEntry::KeyEntry
+---------------------------------------------------------------------*/
AP4_ProtectionKeyMap::KeyEntry::KeyEntry(AP4_UI32        track_id, 
                                         const AP4_UI08* key, 
                                         const AP4_UI08* iv /* = NULL */) :
    m_TrackId(track_id)
{
    SetKey(key, iv);
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::KeyEntry::SetKey
+---------------------------------------------------------------------*/
void
AP4_ProtectionKeyMap::KeyEntry::SetKey(const AP4_UI08* key, const AP4_UI08* iv)
{
    AP4_CopyMemory(m_Key, key, sizeof(m_Key));
    if (iv) {
        AP4_CopyMemory(m_IV, iv, sizeof(m_IV));
    } else {
        AP4_SetMemory(m_IV, 0, sizeof(m_IV));
    }
}

/*----------------------------------------------------------------------
|   AP4_ProtectedSampleDescription::AP4_ProtectedSampleDescription
+---------------------------------------------------------------------*/
AP4_ProtectedSampleDescription::AP4_ProtectedSampleDescription(
    AP4_UI32               format,
    AP4_SampleDescription* original_sample_description,
    AP4_UI32               original_format,
    AP4_UI32               scheme_type,
    AP4_UI32               scheme_version,
    const char*            scheme_uri,
    AP4_ContainerAtom*     schi) :
    AP4_SampleDescription(TYPE_PROTECTED, format),
    m_OriginalSampleDescription(original_sample_description),
    m_OriginalFormat(original_format),
    m_SchemeType(scheme_type),
    m_SchemeVersion(scheme_version),
    m_SchemeUri(scheme_uri)
{
    m_SchemeInfo = new AP4_ProtectionSchemeInfo(schi);
}

/*----------------------------------------------------------------------
|   AP4_ProtectedSampleDescription::~AP4_ProtectedSampleDescription
+---------------------------------------------------------------------*/
AP4_ProtectedSampleDescription::~AP4_ProtectedSampleDescription()
{
    delete m_SchemeInfo;
    delete m_OriginalSampleDescription;
}

/*----------------------------------------------------------------------
|   AP4_ProtectedSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_ProtectedSampleDescription::ToAtom() const
{
    // TODO: not implemented yet
    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_SampleDecrypter:Create
+---------------------------------------------------------------------*/
AP4_SampleDecrypter* 
AP4_SampleDecrypter::Create(AP4_ProtectedSampleDescription* sample_description,
                            const AP4_UI08*                 key,
                            AP4_Size                        key_size)
{
    if (sample_description == NULL || key == NULL) return NULL;
    switch(sample_description->GetSchemeType()) {
        case AP4_PROTECTION_SCHEME_TYPE_OMA:
            return AP4_OmaDcfSampleDecrypter::Create(sample_description, key, key_size);

        case AP4_PROTECTION_SCHEME_TYPE_IAEC:
            return AP4_IsmaCipher::CreateSampleDecrypter(sample_description, key, key_size);

        default:
            return NULL;
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_StandardDecryptingProcessor:CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_StandardDecryptingProcessor::CreateTrackHandler(AP4_TrakAtom* trak)
{
    // find the stsd atom
    AP4_StsdAtom* stsd = dynamic_cast<AP4_StsdAtom*>(
        trak->FindChild("mdia/minf/stbl/stsd"));

    // avoid tracks with no stsd atom (should not happen)
    if (stsd == NULL) return NULL;

    // we only look at the first sample description
    AP4_SampleDescription* desc = stsd->GetSampleDescription(0);
    AP4_SampleEntry* entry = stsd->GetSampleEntry(0);
    if (desc == NULL || entry == NULL) return NULL;
    if (desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
        // create a handler for this track
        AP4_ProtectedSampleDescription* protected_desc = 
            static_cast<AP4_ProtectedSampleDescription*>(desc);
        if (protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_OMA) {
            const AP4_UI08* key = m_KeyMap.GetKey(trak->GetId());
            if (key) {
                return AP4_OmaDcfTrackDecrypter::Create(key, protected_desc, entry);
            }
        } else if (protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_IAEC) {
            const AP4_UI08* key = m_KeyMap.GetKey(trak->GetId());
            if (key) {
                return AP4_IsmaTrackDecrypter::Create(key, protected_desc, entry);
            }
        } 
    }

    return NULL;
}
