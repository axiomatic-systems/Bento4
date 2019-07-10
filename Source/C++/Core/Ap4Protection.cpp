/*****************************************************************
|
|    AP4 - Protected Stream Support
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
#include "Ap4Protection.h"
#include "Ap4SchmAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4FtypAtom.h"
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
#include "Ap4AesBlockCipher.h"
#include "Ap4OmaDcf.h"
#include "Ap4Marlin.h"
#include "Ap4Piff.h"
#include "Ap4CommonEncryption.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_ProtectedSampleDescription)

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
    AP4_FrmaAtom* frma = AP4_DYNAMIC_CAST(AP4_FrmaAtom, FindChild("sinf/frma"));

    // get the schi atom
    AP4_ContainerAtom* schi;
    schi = AP4_DYNAMIC_CAST(AP4_ContainerAtom, FindChild("sinf/schi"));

    // get the scheme info
    AP4_SchmAtom* schm = AP4_DYNAMIC_CAST(AP4_SchmAtom, FindChild("sinf/schm"));
    AP4_UI32 original_format = frma?frma->GetOriginalFormat():AP4_ATOM_TYPE_MP4A;
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
|   AP4_EncaSampleEntry::ToTargetSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_EncaSampleEntry::ToTargetSampleDescription(AP4_UI32 format)
{
    switch (format) {
        case AP4_ATOM_TYPE_MP4A: {
            AP4_EsdsAtom* esds = AP4_DYNAMIC_CAST(AP4_EsdsAtom, GetChild(AP4_ATOM_TYPE_ESDS));
            if (esds == NULL) {
                // check if this is a quicktime style sample description
                if (m_QtVersion > 0) {
                    esds = AP4_DYNAMIC_CAST(AP4_EsdsAtom, FindChild("wave/esds"));
                }
            }
            return new AP4_MpegAudioSampleDescription(
                GetSampleRate(),
                GetSampleSize(),
                GetChannelCount(),
                esds);
        }
        
        default:
            return new AP4_GenericAudioSampleDescription(
                format,
                GetSampleRate(),
                GetSampleSize(),
                GetChannelCount(),
                this);
    }
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
    AP4_FrmaAtom* frma = AP4_DYNAMIC_CAST(AP4_FrmaAtom, FindChild("sinf/frma"));

    // get the schi atom
    AP4_ContainerAtom* schi;
    schi = AP4_DYNAMIC_CAST(AP4_ContainerAtom, FindChild("sinf/schi"));

    // get the scheme info
    AP4_SchmAtom* schm = AP4_DYNAMIC_CAST(AP4_SchmAtom, FindChild("sinf/schm"));
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
|   AP4_EncvSampleEntry::ToTargetSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_EncvSampleEntry::ToTargetSampleDescription(AP4_UI32 format)
{
    switch (format) {
        case AP4_SAMPLE_FORMAT_AVC1:
        case AP4_SAMPLE_FORMAT_AVC2:
        case AP4_SAMPLE_FORMAT_AVC3:
        case AP4_SAMPLE_FORMAT_AVC4:
        case AP4_SAMPLE_FORMAT_DVAV:
        case AP4_SAMPLE_FORMAT_DVA1:
            return new AP4_AvcSampleDescription(
                format,
                m_Width,
                m_Height,
                m_Depth,
                m_CompressorName.GetChars(),
                this);
                
        case AP4_SAMPLE_FORMAT_HVC1:
        case AP4_SAMPLE_FORMAT_HEV1:
        case AP4_SAMPLE_FORMAT_DVHE:
        case AP4_SAMPLE_FORMAT_DVH1:
            return new AP4_HevcSampleDescription(
                format,
                m_Width,
                m_Height,
                m_Depth,
                m_CompressorName.GetChars(),
                this);

        case AP4_ATOM_TYPE_MP4V:
            return new AP4_MpegVideoSampleDescription(
                m_Width,
                m_Height,
                m_Depth,
                m_CompressorName.GetChars(),
                AP4_DYNAMIC_CAST(AP4_EsdsAtom, GetChild(AP4_ATOM_TYPE_ESDS)));

        default:
            return new AP4_GenericVideoSampleDescription(
                format,
                m_Width,
                m_Height,
                m_Depth,
                m_CompressorName.GetChars(),
                this);
    }
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
        m_SchiAtom = static_cast<AP4_ContainerAtom*>(schi->Clone());
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
AP4_ProtectionKeyMap::SetKey(AP4_UI32        track_id, 
                             const AP4_UI08* key, 
                             AP4_Size        key_size,
                             const AP4_UI08* iv,
                             AP4_Size        iv_size)
{
    KeyEntry* entry = GetEntry(track_id);
    if (entry == NULL) {
        m_KeyEntries.Add(new KeyEntry(track_id, key, key_size, iv, iv_size));
    } else {
        entry->SetKey(key, key_size, iv, iv_size);
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::SetKeyForKid
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ProtectionKeyMap::SetKeyForKid(const AP4_UI08* kid,
                                   const AP4_UI08* key,
                                   AP4_Size        key_size,
                                   const AP4_UI08* iv,
                                   AP4_Size        iv_size)
{
    KeyEntry* entry = GetEntryByKid(kid);
    if (entry == NULL) {
        m_KeyEntries.Add(new KeyEntry(kid, key, key_size, iv, iv_size));
    } else {
        entry->SetKey(key, key_size, iv, iv_size);
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
                                      entry->m_Key.GetData(),
                                      entry->m_Key.GetDataSize(),
                                      entry->m_IV.GetData(),
                                      entry->m_IV.GetDataSize()));
        item = item->GetNext();
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::GetKeyAndIv
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ProtectionKeyMap::GetKeyAndIv(AP4_UI32               track_id, 
                                  const AP4_DataBuffer*& key, 
                                  const AP4_DataBuffer*& iv)
{
    KeyEntry* entry = GetEntry(track_id);
    if (entry) {
        key = &entry->m_Key;
        iv = &entry->m_IV;
        return AP4_SUCCESS;
    } else {
        key = NULL;
        iv = NULL;
        return AP4_ERROR_NO_SUCH_ITEM;
    }
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::GetKeyAndIvByKid
+---------------------------------------------------------------------*/
AP4_Result 
AP4_ProtectionKeyMap::GetKeyAndIvByKid(const AP4_UI08*        kid,
                                       const AP4_DataBuffer*& key,
                                       const AP4_DataBuffer*& iv)
{
    KeyEntry* entry = GetEntryByKid(kid);
    if (entry) {
        key = &entry->m_Key;
        iv = &entry->m_IV;
        return AP4_SUCCESS;
    } else {
        key = NULL;
        iv = NULL;
        return AP4_ERROR_NO_SUCH_ITEM;
    }
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::GetKey
+---------------------------------------------------------------------*/
const AP4_DataBuffer* 
AP4_ProtectionKeyMap::GetKey(AP4_UI32 track_id) const
{
    KeyEntry* entry = GetEntry(track_id);
    if (entry) {
        return &entry->m_Key;
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::GetKeyByKid
+---------------------------------------------------------------------*/
const AP4_DataBuffer* 
AP4_ProtectionKeyMap::GetKeyByKid(const AP4_UI08* kid) const
{
    KeyEntry* entry = GetEntryByKid(kid);
    if (entry) {
        return &entry->m_Key;
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
        KeyEntry* entry = item->GetData();
        if (entry->m_TrackId == track_id) return entry;
        item = item->GetNext();
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::GetEntryByKid
+---------------------------------------------------------------------*/
AP4_ProtectionKeyMap::KeyEntry*
AP4_ProtectionKeyMap::GetEntryByKid(const AP4_UI08* kid) const
{
    AP4_List<KeyEntry>::Item* item = m_KeyEntries.FirstItem();
    while (item) {
        KeyEntry* entry = item->GetData();
        if (AP4_CompareMemory(entry->m_KID, kid, 16) == 0) return entry;
        item = item->GetNext();
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::KeyEntry::KeyEntry
+---------------------------------------------------------------------*/
AP4_ProtectionKeyMap::KeyEntry::KeyEntry(AP4_UI32        track_id, 
                                         const AP4_UI08* key, 
                                         AP4_Size        key_size,
                                         const AP4_UI08* iv,
                                         AP4_Size        iv_size) :
    m_TrackId(track_id)
{
    AP4_SetMemory(m_KID, 0, 16);
    SetKey(key, key_size, iv, iv_size);
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::KeyEntry::KeyEntry
+---------------------------------------------------------------------*/
AP4_ProtectionKeyMap::KeyEntry::KeyEntry(const AP4_UI08* kid,
                                         const AP4_UI08* key, 
                                         AP4_Size        key_size,
                                         const AP4_UI08* iv,
                                         AP4_Size        iv_size) :
    m_TrackId(0)
{
    AP4_CopyMemory(m_KID, kid, 16);
    SetKey(key, key_size, iv, iv_size);
}

/*----------------------------------------------------------------------
|   AP4_ProtectionKeyMap::KeyEntry::SetKey
+---------------------------------------------------------------------*/
void
AP4_ProtectionKeyMap::KeyEntry::SetKey(const AP4_UI08* key, AP4_Size key_size, 
                                       const AP4_UI08* iv,  AP4_Size iv_size)
{
    if (key) {
        m_Key.SetData(key, key_size);
    }
    if (iv) {
        m_IV.SetData(iv, iv_size);
    } else {
        m_IV.SetDataSize(16);
        AP4_SetMemory(m_IV.UseData(), 0, 16);
    }
}

/*----------------------------------------------------------------------
|   AP4_TrackPropertyMap::~AP4_TrackPropertyMap
+---------------------------------------------------------------------*/
AP4_TrackPropertyMap::~AP4_TrackPropertyMap()
{
    m_Entries.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_TrackPropertyMap::SetProperty
+---------------------------------------------------------------------*/
AP4_Result
AP4_TrackPropertyMap::SetProperty(AP4_UI32    track_id, 
                                  const char* name, 
                                  const char* value)
{
    return m_Entries.Add(new Entry(track_id, name, value));
}

/*----------------------------------------------------------------------
|   AP4_TrackPropertyMap::SetProperties
+---------------------------------------------------------------------*/
AP4_Result 
AP4_TrackPropertyMap::SetProperties(const AP4_TrackPropertyMap& properties)
{
    AP4_List<Entry>::Item* item = properties.m_Entries.FirstItem();
    while (item) {
        Entry* entry = item->GetData();
        m_Entries.Add(new Entry(entry->m_TrackId,
                                entry->m_Name.GetChars(),
                                entry->m_Value.GetChars()));
        item = item->GetNext();
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_TrackPropertyMap::GetProperty
+---------------------------------------------------------------------*/
const char*
AP4_TrackPropertyMap::GetProperty(AP4_UI32 track_id, const char* name)
{
    AP4_List<Entry>::Item* item = m_Entries.FirstItem();
    while (item) {
        Entry* entry = item->GetData();
        if (entry->m_TrackId == track_id &&
            AP4_CompareStrings(entry->m_Name.GetChars(), name) == 0) {
            return entry->m_Value.GetChars();
        }
        item = item->GetNext();
    }

    // not found
    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_TrackPropertyMap::GetTextualHeaders
+---------------------------------------------------------------------*/
AP4_Result 
AP4_TrackPropertyMap::GetTextualHeaders(AP4_UI32 track_id, AP4_DataBuffer& textual_headers)
{
    AP4_Size    buffer_size = 0;
    AP4_Result  result      = AP4_SUCCESS;
    AP4_Byte*   data_buffer;
    
    // get the size needed for the textual headers
    AP4_List<Entry>::Item* item = m_Entries.FirstItem();
    while (item) {
        Entry* entry = item->GetData();
        if (entry->m_TrackId == track_id) {
            const char* name = entry->m_Name.GetChars();
            if (AP4_CompareStrings(name, "ContentId")       != 0 &&
                AP4_CompareStrings(name, "RightsIssuerUrl") != 0 &&   
                AP4_CompareStrings(name, "KID")             != 0) {   
                buffer_size += (entry->m_Name.GetLength()  + 
                                entry->m_Value.GetLength() +
                                2); // colon + nul 
                             
            }                           
        }
        item = item->GetNext(); 
    }
    
    result = textual_headers.SetDataSize(buffer_size);
    AP4_CHECK(result);
    
    data_buffer = textual_headers.UseData();
    
    // set the textual headers
    item = m_Entries.FirstItem();
    while (item) {
        Entry* entry = item->GetData();
        if (entry->m_TrackId == track_id) {
            const char* name       = entry->m_Name.GetChars();
            const char* value      = NULL;
            AP4_Size    name_len   = 0;
            AP4_Size    value_len  = 0;
            
            if (AP4_CompareStrings(name, "ContentId")       != 0 &&
                AP4_CompareStrings(name, "RightsIssuerUrl") != 0 &&
                AP4_CompareStrings(name, "KID")             != 0) {   
                name_len  = entry->m_Name.GetLength();
                value     = entry->m_Value.GetChars();
                value_len = entry->m_Value.GetLength();                
        
                // format is name:value\0
                if (name && value) {
                    AP4_CopyMemory(data_buffer, name, name_len);
                    data_buffer[name_len] = ':';
                    data_buffer += (1+name_len);
                    AP4_CopyMemory(data_buffer, value, value_len);
                    data_buffer[value_len] = '\0';
                    data_buffer += (1+value_len);                
                } 
            }                                                                                     
        }           
        item = item->GetNext();   
    }
    
    // success path
    return AP4_SUCCESS;        
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
    AP4_ContainerAtom*     schi,
    bool                   transfer_ownership_of_original /* = true */) :
    AP4_SampleDescription(TYPE_PROTECTED, format, NULL),
    m_OriginalSampleDescription(original_sample_description),
    m_OriginalSampleDescriptionIsOwned(transfer_ownership_of_original),
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
    if (m_OriginalSampleDescriptionIsOwned) delete m_OriginalSampleDescription;
}

/*----------------------------------------------------------------------
|   AP4_ProtectedSampleDescription::ToAtom
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_ProtectedSampleDescription::ToAtom() const
{
    // construct the atom for the original sample description
    if (m_OriginalSampleDescription == NULL) return NULL;
    AP4_Atom* atom = m_OriginalSampleDescription->ToAtom();
    
    // switch the atom type
    atom->SetType(m_Format);

    // check that the constructed atom is a container
    AP4_ContainerAtom* container = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
    if (container == NULL) return atom; // not a container ?? return now.
        
    // create the sinf atom
    AP4_ContainerAtom* sinf = new AP4_ContainerAtom(AP4_ATOM_TYPE_SINF);
    
    // create and add a frma atom
    AP4_FrmaAtom* frma = new AP4_FrmaAtom(m_OriginalFormat);
    sinf->AddChild(frma);
    
    // create and add a schm atom
    AP4_SchmAtom* schm = new AP4_SchmAtom(m_SchemeType, m_SchemeVersion, m_SchemeUri.GetChars());
    sinf->AddChild(schm);

    // add the schi atom
    if (m_SchemeInfo && m_SchemeInfo->GetSchiAtom()) {
        sinf->AddChild(m_SchemeInfo->GetSchiAtom()->Clone());
    }
    
    // add the sinf to the returned atom
    container->AddChild(sinf);

    return atom;
}

/*----------------------------------------------------------------------
|   AP4_SampleDecrypter:Create
+---------------------------------------------------------------------*/
AP4_SampleDecrypter* 
AP4_SampleDecrypter::Create(AP4_ProtectedSampleDescription* sample_description,
                            const AP4_UI08*                 key,
                            AP4_Size                        key_size,
                            AP4_BlockCipherFactory*         block_cipher_factory)
{
    if (sample_description == NULL || key == NULL) return NULL;

    // select the block cipher factory
    if (block_cipher_factory == NULL) {
        block_cipher_factory = &AP4_DefaultBlockCipherFactory::Instance;
    }

    switch(sample_description->GetSchemeType()) {
        case AP4_PROTECTION_SCHEME_TYPE_OMA: {
            AP4_OmaDcfSampleDecrypter* decrypter = NULL;
            AP4_Result result = AP4_OmaDcfSampleDecrypter::Create(sample_description, 
                                                                  key, 
                                                                  key_size, 
                                                                  block_cipher_factory, 
                                                                  decrypter);
            if (AP4_FAILED(result)) return NULL;
            return decrypter;
        }

        case AP4_PROTECTION_SCHEME_TYPE_IAEC: {
            AP4_IsmaCipher* decrypter = NULL;
            AP4_Result result = AP4_IsmaCipher::CreateSampleDecrypter(sample_description, 
                                                                      key, 
                                                                      key_size, 
                                                                      block_cipher_factory, 
                                                                      decrypter);
            if (AP4_FAILED(result)) return NULL;
            return decrypter;
        }
        
        default:
            return NULL;
    }

    // unreachable - return NULL;
}

/*----------------------------------------------------------------------
|   AP4_SampleDecrypter:Create
+---------------------------------------------------------------------*/
AP4_SampleDecrypter* 
AP4_SampleDecrypter::Create(AP4_ProtectedSampleDescription* sample_description,
                            AP4_ContainerAtom*              traf,
                            AP4_ByteStream&                 aux_info_data,
                            AP4_Position                    aux_info_data_offset,
                            const AP4_UI08*                 key,
                            AP4_Size                        key_size,
                            AP4_BlockCipherFactory*         block_cipher_factory)
{
    if (sample_description == NULL || traf == NULL || key == NULL) return NULL;

    // select the block cipher factory
    if (block_cipher_factory == NULL) {
        block_cipher_factory = &AP4_DefaultBlockCipherFactory::Instance;
    }

    switch(sample_description->GetSchemeType()) {
        case AP4_PROTECTION_SCHEME_TYPE_PIFF: 
        case AP4_PROTECTION_SCHEME_TYPE_CENC: {
            AP4_CencSampleDecrypter* decrypter = NULL;
            AP4_Result result = AP4_CencSampleDecrypter::Create(sample_description, 
                                                                traf,
                                                                aux_info_data,
                                                                aux_info_data_offset,
                                                                key, 
                                                                key_size, 
                                                                block_cipher_factory, 
                                                                decrypter);
            if (AP4_FAILED(result)) return NULL;
            return decrypter;
        }

        default:
            return NULL;
    }

    // unreachable - return NULL;
}

/*----------------------------------------------------------------------
|   AP4_StandardDecryptingProcessor:AP4_StandardDecryptingProcessor
+---------------------------------------------------------------------*/
AP4_StandardDecryptingProcessor::AP4_StandardDecryptingProcessor(
    const AP4_ProtectionKeyMap* key_map              /* = NULL */,
    AP4_BlockCipherFactory*     block_cipher_factory /* = NULL */)
{
    if (key_map) {
        // copy the keys
        m_KeyMap.SetKeys(*key_map);
    }
    
    if (block_cipher_factory == NULL) {
        m_BlockCipherFactory = &AP4_DefaultBlockCipherFactory::Instance;
    } else {
        m_BlockCipherFactory = block_cipher_factory;
    }
}

/*----------------------------------------------------------------------
 |   AP4_StandardDecryptingProcessor:Initialize
 +---------------------------------------------------------------------*/
AP4_Result 
AP4_StandardDecryptingProcessor::Initialize(AP4_AtomParent&   top_level,
                                            AP4_ByteStream&   /*stream*/,
                                            ProgressListener* /*listener*/)
{
    AP4_FtypAtom* ftyp = AP4_DYNAMIC_CAST(AP4_FtypAtom, top_level.GetChild(AP4_ATOM_TYPE_FTYP));
    if (ftyp) {
        // remove the atom, it will be replaced with a new one
        top_level.RemoveChild(ftyp);
        
        // keep the existing brand and compatible brands except for the ones we want to remove
        AP4_Array<AP4_UI32> compatible_brands;
        compatible_brands.EnsureCapacity(ftyp->GetCompatibleBrands().ItemCount());
        for (unsigned int i=0; i<ftyp->GetCompatibleBrands().ItemCount(); i++) {
            if (ftyp->GetCompatibleBrands()[i] != AP4_OMA_DCF_BRAND_OPF2) {
                compatible_brands.Append(ftyp->GetCompatibleBrands()[i]);
            }
        }
        
        // create a replacement for the major brand
        top_level.AddChild(new AP4_FtypAtom(ftyp->GetMajorBrand(),
                                            ftyp->GetMinorVersion(),
                                            &compatible_brands[0],
                                            compatible_brands.ItemCount()), 0);
        delete ftyp;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_StandardDecryptingProcessor:CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_StandardDecryptingProcessor::CreateTrackHandler(AP4_TrakAtom* trak)
{
    // find the stsd atom
    AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));

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
            const AP4_DataBuffer* key = m_KeyMap.GetKey(trak->GetId());
            if (key) {
                AP4_OmaDcfTrackDecrypter* handler = NULL;
                AP4_Result result = AP4_OmaDcfTrackDecrypter::Create(key->GetData(), 
                                                                     key->GetDataSize(), 
                                                                     protected_desc, 
                                                                     entry, 
                                                                     m_BlockCipherFactory, 
                                                                     handler);
                if (AP4_FAILED(result)) return NULL;
                return handler;
            }
        } else if (protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_IAEC) {
            const AP4_DataBuffer* key = m_KeyMap.GetKey(trak->GetId());
            if (key) {
                AP4_IsmaTrackDecrypter* handler = NULL;
                AP4_Result result = AP4_IsmaTrackDecrypter::Create(key->GetData(), 
                                                                   key->GetDataSize(), 
                                                                   protected_desc, 
                                                                   entry, 
                                                                   m_BlockCipherFactory, 
                                                                   handler);
                if (AP4_FAILED(result)) return NULL;
                return handler;
            }
        } 
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::AP4_DecryptingStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_DecryptingStream::Create(AP4_BlockCipher::CipherMode mode,
                             AP4_ByteStream&             encrypted_stream,
                             AP4_LargeSize               cleartext_size,
                             const AP4_UI08*             iv,
                             AP4_Size                    iv_size,
                             const AP4_UI08*             key,
                             AP4_Size                    key_size,
                             AP4_BlockCipherFactory*     block_cipher_factory,
                             AP4_ByteStream*&            stream)
{
    // default return value
    stream = NULL;

    // default cipher settings
    if (block_cipher_factory == NULL) {
        block_cipher_factory = &AP4_DefaultBlockCipherFactory::Instance;
    }
    
    // get the encrypted size (includes padding)
    AP4_LargeSize encrypted_size = 0;
    AP4_Result result = encrypted_stream.GetSize(encrypted_size);
    if (AP4_FAILED(result)) return result;
    
    // check IV
    if (iv == NULL || iv_size != 16) return AP4_ERROR_INVALID_PARAMETERS;
    
    // check that the encrypted size is consistent with the cipher mode
    AP4_BlockCipher::CtrParams ctr_params;
    const void*                mode_params = NULL;
    if (mode == AP4_BlockCipher::CBC) {
        // we need at least 16 bytes of data+padding
        // we also need a multiple of the block size
        if (encrypted_size < 16 || ((encrypted_size % 16) != 0)) {
            return AP4_ERROR_INVALID_FORMAT;
        }
    } else if (mode == AP4_BlockCipher::CTR) {
        ctr_params.counter_size = 16;
        mode_params = &ctr_params;
    } else {
        return AP4_ERROR_NOT_SUPPORTED;
    }
        
    // create the stream cipher
    AP4_BlockCipher* block_cipher;
    result = block_cipher_factory->CreateCipher(AP4_BlockCipher::AES_128,
                                                AP4_BlockCipher::DECRYPT,
                                                mode,
                                                mode_params,
                                                key, 
                                                key_size, 
                                                block_cipher);
    if (AP4_FAILED(result)) return result;
    
    // keep a reference to the source stream
    encrypted_stream.AddReference();

    // create the cipher according to the mode
    AP4_StreamCipher* stream_cipher = NULL;
    switch (mode) {
        case AP4_BlockCipher::CBC:
            stream_cipher = new AP4_CbcStreamCipher(block_cipher);
            break;
        case AP4_BlockCipher::CTR:
            stream_cipher = new AP4_CtrStreamCipher(block_cipher, AP4_CIPHER_BLOCK_SIZE);
            break;
        default:
            // should never occur
            return AP4_ERROR_NOT_SUPPORTED;
    }
    
    // set the IV
    stream_cipher->SetIV(iv);

    // create the stream
    stream = new AP4_DecryptingStream(cleartext_size, 
                                      &encrypted_stream,
                                      encrypted_size,
                                      stream_cipher);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::~AP4_DecryptingStream
+---------------------------------------------------------------------*/
AP4_DecryptingStream::~AP4_DecryptingStream()
{
    delete m_StreamCipher;
    m_EncryptedStream->Release();
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::AddReference
+---------------------------------------------------------------------*/
void 
AP4_DecryptingStream::AddReference()
{
    ++m_ReferenceCount;
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::Release
+---------------------------------------------------------------------*/
void 
AP4_DecryptingStream::Release()
{
    if (--m_ReferenceCount == 0) delete this;
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::ReadPartial
+---------------------------------------------------------------------*/
AP4_Result 
AP4_DecryptingStream::ReadPartial(void*     buffer, 
                                  AP4_Size  bytes_to_read, 
                                  AP4_Size& bytes_read)
{
    bytes_read = 0;

    // never read more than what's available
    AP4_LargeSize available = m_CleartextSize-m_CleartextPosition;
    if (available < bytes_to_read) {
        if (available == 0) {
            return AP4_ERROR_EOS;
        }
        bytes_to_read = (AP4_Size)available;
    }
    
    if (m_BufferFullness) {
        // we have some leftovers
        AP4_Size chunk = bytes_to_read;
        if (chunk > m_BufferFullness) chunk = m_BufferFullness;
        AP4_CopyMemory(buffer, &m_Buffer[m_BufferOffset], chunk);
        buffer = (char*)buffer+chunk;
        m_CleartextPosition += chunk;
        available           -= chunk;
        bytes_to_read       -= chunk;
        m_BufferFullness    -= chunk;
        m_BufferOffset      += chunk;
        bytes_read          += chunk;
    }
    if (bytes_to_read == 0) return AP4_SUCCESS;
    
    // seek to the right place in the input
    m_EncryptedStream->Seek(m_EncryptedPosition);

    while (bytes_to_read) {
        // read from the source
        AP4_UI08 encrypted[1024];
        AP4_Size encrypted_read = 0;
        AP4_Result result = m_EncryptedStream->ReadPartial(encrypted, sizeof(encrypted), encrypted_read);
        if (result == AP4_ERROR_EOS) {
            if (bytes_read == 0) {
                return AP4_ERROR_EOS;
            } else {
                return AP4_SUCCESS;
            }
        } else if (result != AP4_SUCCESS) {
            return result;
        } else {
            m_EncryptedPosition += encrypted_read;
        }
        bool is_last_buffer = (m_EncryptedPosition >= m_EncryptedSize);
        AP4_Size buffer_size = sizeof(m_Buffer);
        result = m_StreamCipher->ProcessBuffer(encrypted, 
                                               encrypted_read, 
                                               m_Buffer, 
                                               &buffer_size,
                                               is_last_buffer);
        if (AP4_FAILED(result)) return result;
        m_BufferOffset   = 0;
        m_BufferFullness = buffer_size;

        AP4_Size chunk = bytes_to_read;
        if (chunk > m_BufferFullness) chunk = m_BufferFullness;
        if (chunk) AP4_CopyMemory(buffer, &m_Buffer[m_BufferOffset], chunk);
        buffer = (char*)buffer+chunk;
        m_CleartextPosition += chunk;
        available           -= chunk;
        bytes_to_read       -= chunk;
        m_BufferFullness    -= chunk;
        m_BufferOffset      += chunk;
        bytes_read          += chunk;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::WritePartial
+---------------------------------------------------------------------*/
AP4_Result 
AP4_DecryptingStream::WritePartial(const void* /* buffer         */, 
                                   AP4_Size    /* bytes_to_write */, 
                                   AP4_Size&   /* bytes_written  */)
{
    return AP4_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::Seek
+---------------------------------------------------------------------*/
AP4_Result 
AP4_DecryptingStream::Seek(AP4_Position position)
{
    AP4_Cardinal preroll = 0;
    
    // check for no-op seek requests
    if (position == m_CleartextPosition) {
        return AP4_SUCCESS;
    }
    
    // check bounds
    if (position > m_CleartextSize) {
        return AP4_ERROR_INVALID_PARAMETERS;
    }
    
    // try to put the stream cipher at the right offset
    AP4_CHECK(m_StreamCipher->SetStreamOffset(position, &preroll));

    // seek in the source stream
    AP4_CHECK(m_EncryptedStream->Seek(position-preroll));
    
    // if we need to, process the preroll bytes
    if (preroll > 0) {
        AP4_Size out_size = 0;
        AP4_UI08 buffer[2*AP4_CIPHER_BLOCK_SIZE]; // bigger than preroll
        AP4_CHECK(m_EncryptedStream->Read(buffer, preroll));
        AP4_CHECK(m_StreamCipher->ProcessBuffer(buffer, preroll, buffer, &out_size));
        AP4_ASSERT(out_size == 0); // we're just feeding prerolled bytes, 
                                   // there can be no output
    }
    
    // update the counters
    m_CleartextPosition = position;
    m_EncryptedPosition = position;
    m_BufferFullness    = 0;
    m_BufferOffset      = 0;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::Tell
+---------------------------------------------------------------------*/
AP4_Result 
AP4_DecryptingStream::Tell(AP4_Position& position)
{
    position = m_CleartextPosition;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_DecryptingStream::GetSize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_DecryptingStream::GetSize(AP4_LargeSize& size)
{
    size = m_CleartextSize;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::AP4_EncryptingStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_EncryptingStream::Create(AP4_BlockCipher::CipherMode mode,
                             AP4_ByteStream&             cleartext_stream,
                             const AP4_UI08*             iv,
                             AP4_Size                    iv_size,
                             const AP4_UI08*             key,
                             AP4_Size                    key_size,
                             bool                        prepend_iv,
                             AP4_BlockCipherFactory*     block_cipher_factory,
                             AP4_ByteStream*&            stream)
{
    // default return value
    stream = NULL;

    // get the cleartext size
    AP4_LargeSize cleartext_size = 0;
    AP4_Result result = cleartext_stream.GetSize(cleartext_size);
    if (AP4_FAILED(result)) return result;
    
    // check IV
    if (iv == NULL || iv_size != 16) return AP4_ERROR_INVALID_PARAMETERS;
    
    // compute the encrypted size and set cipher params
    AP4_LargeSize              encrypted_size = cleartext_size;
    const void*                mode_params = NULL;
    AP4_BlockCipher::CtrParams ctr_params;
    if (mode == AP4_BlockCipher::CBC) {
        encrypted_size += (16-(cleartext_size%16)); // with padding
    } else {
        ctr_params.counter_size = 16;
        mode_params = &ctr_params;
    }
    
    // create the stream cipher
    AP4_BlockCipher* block_cipher;
    result = block_cipher_factory->CreateCipher(AP4_BlockCipher::AES_128,
                                                AP4_BlockCipher::ENCRYPT,
                                                mode,
                                                mode_params,
                                                key, 
                                                key_size, 
                                                block_cipher);
    if (AP4_FAILED(result)) return result;
    
    // keep a reference to the source stream
    cleartext_stream.AddReference();

    // create the cipher according to the mode
    AP4_StreamCipher* stream_cipher = NULL;
    switch (mode) {
        case AP4_BlockCipher::CBC:
            stream_cipher = new AP4_CbcStreamCipher(block_cipher);
            break;
        case AP4_BlockCipher::CTR:
            stream_cipher = new AP4_CtrStreamCipher(block_cipher, AP4_CIPHER_BLOCK_SIZE);
            break;
        default:
            // should never occur
            AP4_ASSERT(0);
    }
    
    // set the IV
    stream_cipher->SetIV(iv);

    // create the stream
    AP4_EncryptingStream* enc_stream = new AP4_EncryptingStream(cleartext_size,
                                                                &cleartext_stream,
                                                                encrypted_size, 
                                                                stream_cipher);
    stream = enc_stream;

    // deal with the prepended IV if required
    if (prepend_iv) {
        enc_stream->m_EncryptedSize += 16;
        enc_stream->m_BufferFullness = 16;
        AP4_CopyMemory(enc_stream->m_Buffer, iv, 16);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::~AP4_EncryptingStream
+---------------------------------------------------------------------*/
AP4_EncryptingStream::~AP4_EncryptingStream()
{
    delete m_StreamCipher;
    m_CleartextStream->Release();
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::AddReference
+---------------------------------------------------------------------*/
void 
AP4_EncryptingStream::AddReference()
{
    ++m_ReferenceCount;
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::Release
+---------------------------------------------------------------------*/
void 
AP4_EncryptingStream::Release()
{
    if (--m_ReferenceCount == 0) delete this;
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::ReadPartial
+---------------------------------------------------------------------*/
AP4_Result 
AP4_EncryptingStream::ReadPartial(void*     buffer, 
                                  AP4_Size  bytes_to_read, 
                                  AP4_Size& bytes_read)
{
    bytes_read = 0;

    // never read more than what's available
    AP4_LargeSize available = m_EncryptedSize-m_EncryptedPosition;
    if (available < bytes_to_read) {
        if (available == 0) return AP4_ERROR_EOS;
        bytes_to_read = (AP4_Size)available;
    }
    
    if (m_BufferFullness) {
        // we have some leftovers
        AP4_Size chunk = bytes_to_read;
        if (chunk > m_BufferFullness) chunk = m_BufferFullness;
        AP4_CopyMemory(buffer, &m_Buffer[m_BufferOffset], chunk);
        buffer = (char*)buffer+chunk;
        m_EncryptedPosition += chunk;
        available           -= chunk;
        bytes_to_read       -= chunk;
        m_BufferFullness    -= chunk;
        m_BufferOffset      += chunk;
        bytes_read          += chunk;
    }

    // seek to the right place in the input
    m_CleartextStream->Seek(m_CleartextPosition);

    while (bytes_to_read) {
        // read from the source
        AP4_UI08 cleartext[1024];
        AP4_Size cleartext_read = 0;
        AP4_Result result = m_CleartextStream->ReadPartial(cleartext, sizeof(cleartext), cleartext_read);
        if (result == AP4_ERROR_EOS) {
            if (bytes_read == 0) {
                return AP4_ERROR_EOS;
            } else {
                return AP4_SUCCESS;
            }
        } else if (result != AP4_SUCCESS) {
            return result;
        } else {
            m_CleartextPosition += cleartext_read;
        }
        bool is_last_buffer = (m_CleartextPosition >= m_CleartextSize);
        AP4_Size buffer_size = sizeof(m_Buffer);
        result = m_StreamCipher->ProcessBuffer(cleartext, 
                                               cleartext_read, 
                                               m_Buffer, 
                                               &buffer_size,
                                               is_last_buffer);
        if (AP4_FAILED(result)) return result;
        m_BufferOffset   = 0;
        m_BufferFullness = buffer_size;

        AP4_Size chunk = bytes_to_read;
        if (chunk > m_BufferFullness) chunk = m_BufferFullness;
        if (chunk) {
            AP4_CopyMemory(buffer, &m_Buffer[m_BufferOffset], chunk);
            buffer = (char*)buffer+chunk;
            m_EncryptedPosition += chunk;
            available           -= chunk;
            bytes_to_read       -= chunk;
            m_BufferFullness    -= chunk;
            m_BufferOffset      += chunk;
            bytes_read          += chunk;
        }
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::WritePartial
+---------------------------------------------------------------------*/
AP4_Result 
AP4_EncryptingStream::WritePartial(const void* /* buffer         */, 
                                   AP4_Size    /* bytes_to_write */, 
                                   AP4_Size&   /* bytes_written  */)
{
    return AP4_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::Seek
+---------------------------------------------------------------------*/
AP4_Result 
AP4_EncryptingStream::Seek(AP4_Position position)
{
    if (position == m_EncryptedPosition) {
        return AP4_SUCCESS;
    } else {
        return AP4_ERROR_NOT_SUPPORTED;
    }
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::Tell
+---------------------------------------------------------------------*/
AP4_Result 
AP4_EncryptingStream::Tell(AP4_Position& position)
{
    position = m_EncryptedPosition;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_EncryptingStream::GetSize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_EncryptingStream::GetSize(AP4_LargeSize& size)
{
    size = m_EncryptedSize;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_DefaultBlockCipherFactory::Instance
+---------------------------------------------------------------------*/
AP4_DefaultBlockCipherFactory AP4_DefaultBlockCipherFactory::Instance;

/*----------------------------------------------------------------------
|   AP4_DefaultBlockCipherFactory
+---------------------------------------------------------------------*/
AP4_Result
AP4_DefaultBlockCipherFactory::CreateCipher(AP4_BlockCipher::CipherType      type,
                                            AP4_BlockCipher::CipherDirection direction,
                                            AP4_BlockCipher::CipherMode      mode,
                                            const void*                      mode_params,
                                            const AP4_UI08*                  key,
                                            AP4_Size                         key_size,
                                            AP4_BlockCipher*&                cipher)
{
    // setup default return vaule
    cipher = NULL;

    switch (type) {
        case AP4_BlockCipher::AES_128:
            // check cipher parameters
            if (key == NULL || key_size != AP4_AES_BLOCK_SIZE) {
                return AP4_ERROR_INVALID_PARAMETERS;
            }

            // create the cipher
            {
                AP4_AesBlockCipher* aes_cipher = NULL;
                AP4_Result result =  AP4_AesBlockCipher::Create(key, direction, mode, mode_params, aes_cipher);
                if (AP4_FAILED(result)) return result;
                cipher = aes_cipher;
                return AP4_SUCCESS;
            }

        default:
            // not supported
            return AP4_ERROR_NOT_SUPPORTED;
    }
}

/*----------------------------------------------------------------------
|   AP4_DefaultBlockCipherFactory::AP4_DefaultBlockCipherFactory
+---------------------------------------------------------------------*/
AP4_DefaultBlockCipherFactory::AP4_DefaultBlockCipherFactory() : 
    m_Initialized(true)
{
}

/*----------------------------------------------------------------------
|   AP4_DefaultBlockCipherFactory::Initialize
+---------------------------------------------------------------------*/
AP4_Result
AP4_DefaultBlockCipherFactory::Initialize()
{
    m_Initialized = true;
    return AP4_SUCCESS;
}
