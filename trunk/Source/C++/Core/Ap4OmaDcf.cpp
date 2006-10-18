/*****************************************************************
|
|    AP4 - OMA DCF Support
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
#include "Ap4OdafAtom.h"
#include "Ap4OmaDcf.h"
#include "Ap4OhdrAtom.h"

/*----------------------------------------------------------------------
|   AP4_OmaDcfCrypCipher::AP4_OmaDcfCipher
+---------------------------------------------------------------------*/
AP4_OmaDcfCipher::AP4_OmaDcfCipher(const AP4_UI08* key,
                                   const AP4_UI08* salt,
                                   AP4_Size        counter_size)
{
    // left-align the salt
    unsigned char salt_128[AP4_AES_KEY_LENGTH];
    unsigned int i=0;
    if (salt) {
        for (; i<8; i++) {
            salt_128[i] = salt[i];
        }
    }
    for (; i<AP4_AES_KEY_LENGTH; i++) {
        salt_128[i] = 0;
    }
    
    // create a cipher
    m_Cipher = new AP4_CtrStreamCipher(key, salt_128, counter_size);
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCipher::~AP4_OmaDcfCipher
+---------------------------------------------------------------------*/
AP4_OmaDcfCipher::~AP4_OmaDcfCipher()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCipher::DecryptSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfCipher::DecryptSample(AP4_DataBuffer& data_in,
                                AP4_DataBuffer& data_out)
{
    bool                 is_encrypted = true;
    const unsigned char* in = data_in.GetData();
    AP4_Size             in_size = data_in.GetDataSize();
    
    // default to 0 output 
    data_out.SetDataSize(0);

    // check the selective encryption flag
    if (in_size < 1) return AP4_ERROR_INVALID_FORMAT;
    is_encrypted = ((in[0]&0x80)!=0);
    in++;

    // check the size
    unsigned int header_size = 1+(is_encrypted?AP4_AES_BLOCK_SIZE:0);
    if (header_size > in_size) return AP4_ERROR_INVALID_FORMAT;

    // process the sample data
    unsigned int payload_size = in_size-header_size;
    data_out.SetDataSize(payload_size);
    unsigned char* out = data_out.UseData();
    if (is_encrypted) {
        // get the IV
        const AP4_UI08* iv = (const AP4_UI08*)in;
        in += AP4_AES_BLOCK_SIZE;

        m_Cipher->SetBaseCounter(iv);
        m_Cipher->ProcessBuffer(in, out, payload_size);
    } else {
        AP4_CopyMemory(out, in, payload_size);
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCipher::EncryptSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfCipher::EncryptSample(AP4_DataBuffer& data_in,
                                AP4_DataBuffer& data_out,
                                AP4_UI32        counter,
                                bool            /*skip_encryption*/)
{
    // setup the buffers
    const unsigned char* in = data_in.GetData();
    data_out.SetDataSize(data_in.GetDataSize()+AP4_AES_BLOCK_SIZE+1);
    unsigned char* out = data_out.UseData();

    // selective encryption flag
    *out++ = 0x80;

    // IV on 16 bytes: [SSSSSSSS0000XXXX]
    // where SSSSSSSS is the 64-bit salt and
    // XXXX is the 32-bit base counter
    AP4_CopyMemory(out, m_Cipher->GetBaseCounter(), 8);
    out[8] = out[9] = out[10] = out[11] = 0;
    AP4_BytesFromUInt32BE(&out[12], counter);

    // encrypt the payload
    m_Cipher->SetBaseCounter(out+8);
    m_Cipher->ProcessBuffer(in, out+AP4_AES_BLOCK_SIZE, data_in.GetDataSize());

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackDecrypter::Create
+---------------------------------------------------------------------*/
AP4_OmaDcfTrackDecrypter*
AP4_OmaDcfTrackDecrypter::Create(
    const AP4_UI08*                 key,
    AP4_ProtectedSampleDescription* sample_description,
    AP4_SampleEntry*                sample_entry)
{
    // check the scheme details
    AP4_OhdrAtom* ohdr = (AP4_OhdrAtom*)sample_description->GetSchemeInfo()->GetSchiAtom().FindChild("odkm/ohdr");
    if (ohdr == NULL) return NULL;
    if (ohdr->GetEncryptionMethod() == AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC) {
        if (ohdr->GetPaddingScheme() != AP4_OMA_DCF_PADDING_SCHEME_RFC_2630) {
            return NULL;
        }
        // FIXME: not yet supported
        return NULL;
     } else if (ohdr->GetEncryptionMethod() == AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR) {
        if (ohdr->GetPaddingScheme() != AP4_OMA_DCF_PADDING_SCHEME_NONE) {
            return NULL;
        }
    } else {
        return NULL;
    }

    // get the cipher params
    AP4_OdafAtom* odaf = (AP4_OdafAtom*)sample_description->GetSchemeInfo()->GetSchiAtom().FindChild("odkm/odaf");
    if (odaf == NULL                              || 
        odaf->GetIvLength() != AP4_AES_BLOCK_SIZE ||
        odaf->GetSelectiveEncryption() != true) return NULL;
    
    // create the cipher
    AP4_OmaDcfCipher* cipher = new AP4_OmaDcfCipher(key, NULL, AP4_AES_BLOCK_SIZE);

    // instantiate the object
    return new AP4_OmaDcfTrackDecrypter(odaf,
                                        cipher, 
                                        sample_entry, 
                                        sample_description->GetOriginalFormat());
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackDecrypter::AP4_OmaDcfTrackDecrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfTrackDecrypter::AP4_OmaDcfTrackDecrypter(AP4_OdafAtom*     cipher_params,
                                                   AP4_OmaDcfCipher* cipher,
                                                   AP4_SampleEntry*  sample_entry,
                                                   AP4_UI32          original_format) :
    m_CipherParams(cipher_params),
    m_Cipher(cipher),
    m_SampleEntry(sample_entry),
    m_OriginalFormat(original_format)
{
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackDecrypter::~AP4_OmaDcfTrackDecrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfTrackDecrypter::~AP4_OmaDcfTrackDecrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackDecrypter::GetProcessedSampleSize
+---------------------------------------------------------------------*/
AP4_Size   
AP4_OmaDcfTrackDecrypter::GetProcessedSampleSize(AP4_Sample& sample)
{
    if (m_Cipher == NULL) return 0;

    // read the first byte to see if the sample is encrypted or not
    sample.ReadData(m_PeekBuffer, 1);
    bool is_encrypted = ((m_PeekBuffer.GetData()[0]&0x80)!=0);

    AP4_Size crypto_header_size = 1+(is_encrypted?m_CipherParams->GetIvLength():0);
    return sample.GetSize()-crypto_header_size;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackDecrypter::ProcessTrack
+---------------------------------------------------------------------*/
AP4_Result   
AP4_OmaDcfTrackDecrypter::ProcessTrack()
{
    m_SampleEntry->SetType(m_OriginalFormat);
    m_SampleEntry->DeleteChild(AP4_ATOM_TYPE_SINF);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfDecrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfTrackDecrypter::ProcessSample(AP4_DataBuffer& data_in,
                                        AP4_DataBuffer& data_out)
{
    return m_Cipher->DecryptSample(data_in, data_out);
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackEncrypter
+---------------------------------------------------------------------*/
class AP4_OmaDcfTrackEncrypter : public AP4_Processor::TrackHandler {
public:
    // constructor
    AP4_OmaDcfTrackEncrypter(const AP4_UI08*  key,
                             const AP4_UI08*  iv,
                             AP4_SampleEntry* sample_entry,
                             AP4_UI32         format,
                             const char*      content_id,
                             const char*      rights_issuer_url);
    virtual ~AP4_OmaDcfTrackEncrypter();

    // methods
    virtual AP4_Size   GetProcessedSampleSize(AP4_Sample& sample);
    virtual AP4_Result ProcessTrack();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);

private:
    // members
    AP4_OmaDcfCipher* m_Cipher;
    AP4_SampleEntry*  m_SampleEntry;
    AP4_UI32          m_Format;
    AP4_String        m_ContentId;
    AP4_String        m_RightsIssuerUrl;
    AP4_UI32          m_Counter;
};

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackEncrypter::AP4_OmaDcfTrackEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfTrackEncrypter::AP4_OmaDcfTrackEncrypter(
    const AP4_UI08*  key,
    const AP4_UI08*  salt,
    AP4_SampleEntry* sample_entry,
    AP4_UI32         format,
    const char*      content_id,
    const char*      rights_issuer_url) :
    m_SampleEntry(sample_entry),
    m_Format(format),
    m_ContentId(content_id),
    m_RightsIssuerUrl(rights_issuer_url),
    m_Counter(0)
{
    // instantiate the cipher (fixed params for now)
    m_Cipher = new AP4_OmaDcfCipher(key, salt, 8);
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackEncrypter::~AP4_OmaDcfTrackEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfTrackEncrypter::~AP4_OmaDcfTrackEncrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackEncrypter::GetProcessedSampleSize
+---------------------------------------------------------------------*/
AP4_Size   
AP4_OmaDcfTrackEncrypter::GetProcessedSampleSize(AP4_Sample& sample)
{
    return sample.GetSize()+17; //fixed header size for now
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackEncrypter::ProcessTrack
+---------------------------------------------------------------------*/
AP4_Result   
AP4_OmaDcfTrackEncrypter::ProcessTrack()
{
    // sinf container
    AP4_ContainerAtom* sinf = new AP4_ContainerAtom(AP4_ATOM_TYPE_SINF);

    // original format
    AP4_FrmaAtom* frma = new AP4_FrmaAtom(m_SampleEntry->GetType());
    
    // scheme info
    AP4_ContainerAtom* schi = new AP4_ContainerAtom(AP4_ATOM_TYPE_SCHI);
    AP4_OdafAtom*      odaf = new AP4_OdafAtom(true, 0, AP4_AES_BLOCK_SIZE);
    AP4_LargeInt       plaintext_length = {0,0};
    AP4_OhdrAtom*      ohdr = new AP4_OhdrAtom(AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR,
                                               AP4_OMA_DCF_PADDING_SCHEME_NONE,
                                               plaintext_length,
                                               m_ContentId.GetChars(),
                                               m_RightsIssuerUrl.GetChars(),
                                               NULL);
    AP4_ContainerAtom* odkm = new AP4_ContainerAtom(AP4_ATOM_TYPE_ODKM, AP4_FULL_ATOM_HEADER_SIZE, 0, 0);
    odkm->AddChild(odaf);
    odkm->AddChild(ohdr);

    // populate the schi container
    schi->AddChild(odkm);

    // populate the sinf container
    sinf->AddChild(frma);
    sinf->AddChild(schi);

    // add the sinf atom to the sample description
    m_SampleEntry->AddChild(sinf);

    // change the atom type of the sample description
    m_SampleEntry->SetType(m_Format);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackEncrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfTrackEncrypter::ProcessSample(AP4_DataBuffer& data_in,
                                        AP4_DataBuffer& data_out)
{
    AP4_Result result = m_Cipher->EncryptSample(data_in, 
                                                data_out, 
                                                m_Counter, 
                                                false);
    if (AP4_FAILED(result)) return result;

    m_Counter += (data_in.GetDataSize()+AP4_AES_BLOCK_SIZE-1)/AP4_AES_BLOCK_SIZE;
    return AP4_SUCCESS;
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
|   AP4_OmaDcfEncryptingProcessor:CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_OmaDcfEncryptingProcessor::CreateTrackHandler(AP4_TrakAtom* trak)
{
    // find the stsd atom
    AP4_StsdAtom* stsd = dynamic_cast<AP4_StsdAtom*>(
        trak->FindChild("mdia/minf/stbl/stsd"));

    // avoid tracks with no stsd atom (should not happen)
    if (stsd == NULL) return NULL;

    // only look at the first sample description
    AP4_SampleEntry* entry = stsd->GetSampleEntry(0);
    if (entry == NULL) return NULL;
        
    // create a handler for this track if we have a key for it and we know
    // how to map the type
    const AP4_UI08* key;
    const AP4_UI08* iv;
    AP4_UI32        format = 0;
    if (AP4_SUCCEEDED(m_KeyMap.GetKeyAndIv(trak->GetId(), key, iv))) {
        switch (entry->GetType()) {
            case AP4_ATOM_TYPE_MP4A:
                format = AP4_ATOM_TYPE_ENCA;
                break;

            case AP4_ATOM_TYPE_MP4V:
            case AP4_ATOM_TYPE_AVC1:
                format = AP4_ATOM_TYPE_ENCV;
                break;
        }
        if (format) {
            const char* content_id = m_PropertyMap.GetProperty(trak->GetId(), "ContentId");
            const char* rights_issuer_url = m_PropertyMap.GetProperty(trak->GetId(), "RightsIssuerUrl");
            return new AP4_OmaDcfTrackEncrypter(key, iv, entry, format, content_id, rights_issuer_url);
        }
    }

    return NULL;
}
