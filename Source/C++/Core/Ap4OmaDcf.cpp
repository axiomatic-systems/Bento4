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
|   AP4_OmaDcfSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_OmaDcfSampleDecrypter*
AP4_OmaDcfSampleDecrypter::Create(AP4_ProtectedSampleDescription* sample_description, 
                                  const AP4_UI08* key, AP4_Size key_size)
{
    // check the key and key size
    if (key == NULL || key_size != AP4_AES_BLOCK_SIZE) return NULL;

    // get the scheme info atom
    AP4_ContainerAtom* schi = sample_description->GetSchemeInfo()->GetSchiAtom();
    if (schi == NULL) return NULL;

    // get and check the cipher params
    // NOTE: we only support an IV Length less than or equal to the cipher block size, 
    // and we don't know how to deal with a key indicator length != 0
    AP4_OdafAtom* odaf = dynamic_cast<AP4_OdafAtom*>(schi->FindChild("odkm/odaf"));
    if (odaf) {
        if (odaf->GetIvLength() > AP4_AES_BLOCK_SIZE) return NULL;
        if (odaf->GetKeyIndicatorLength() != 0) return NULL;
    }

    // check the scheme details and create the cipher
    AP4_OhdrAtom* ohdr = dynamic_cast<AP4_OhdrAtom*>(schi->FindChild("odkm/ohdr"));
    if (ohdr == NULL) return NULL;
    AP4_UI08 encryption_method = ohdr->GetEncryptionMethod();
    if (encryption_method == AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC) {
        // in CBC mode, we only support IVs of the same size as the cipher block size
        if (odaf->GetIvLength() != AP4_AES_BLOCK_SIZE) return NULL;

        // require RFC_2630 padding
        if (ohdr->GetPaddingScheme() != AP4_OMA_DCF_PADDING_SCHEME_RFC_2630) {
            return NULL;
        }
        return new AP4_OmaDcfCbcSampleDecrypter(key, odaf->GetSelectiveEncryption());
    } else if (encryption_method == AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR) {
        // require NONE padding
        if (ohdr->GetPaddingScheme() != AP4_OMA_DCF_PADDING_SCHEME_NONE) {
            return NULL;
        }
        return new AP4_OmaDcfCtrSampleDecrypter(key, odaf->GetIvLength(), odaf->GetSelectiveEncryption());
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleDecrypter::AP4_OmaDcfCtrSampleDecrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCtrSampleDecrypter::AP4_OmaDcfCtrSampleDecrypter(
    const AP4_UI08* key,
    AP4_Size        iv_length,
    bool            selective_encryption) :
    AP4_OmaDcfSampleDecrypter(iv_length, selective_encryption)
{
    m_Cipher = new AP4_CtrStreamCipher(key, NULL, iv_length);
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleDecrypter::~AP4_OmaDcfCtrSampleDecrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCtrSampleDecrypter::~AP4_OmaDcfCtrSampleDecrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfCtrSampleDecrypter::DecryptSampleData(AP4_DataBuffer& data_in,
                                                AP4_DataBuffer& data_out)
{   
    bool                 is_encrypted = true;
    const unsigned char* in = data_in.GetData();
    AP4_Size             in_size = data_in.GetDataSize();

    // default to 0 output 
    data_out.SetDataSize(0);

    // check the selective encryption flag
    if (m_SelectiveEncryption) {
        if (in_size < 1) return AP4_ERROR_INVALID_FORMAT;
        is_encrypted = ((in[0]&0x80)!=0);
        in++;
    }

    // check the size
    unsigned int header_size = (m_SelectiveEncryption?1:0)+(is_encrypted?m_IvLength:0);
    if (header_size > in_size) return AP4_ERROR_INVALID_FORMAT;

    // process the sample data
    unsigned int payload_size = in_size-header_size;
    data_out.Reserve(payload_size);
    unsigned char* out = data_out.UseData();
    if (is_encrypted) {
        // set the IV
        m_Cipher->SetBaseCounter(in);
        AP4_CHECK(m_Cipher->ProcessBuffer(in+m_IvLength, out, payload_size));
    } else {
        AP4_CopyMemory(out, in, payload_size);
    }
    data_out.SetDataSize(payload_size);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleDecrypter::GetDecryptedSampleSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_OmaDcfCtrSampleDecrypter::GetDecryptedSampleSize(AP4_Sample& sample)
{
    if (m_Cipher == NULL) return 0;

    // decide if this sample is encrypted or not
    bool is_encrypted;
    if (m_SelectiveEncryption) {
        // read the first byte to see if the sample is encrypted or not
        AP4_Byte h;
        AP4_DataBuffer peek_buffer;
        peek_buffer.SetBuffer(&h, 1);
        sample.ReadData(peek_buffer, 1);
        is_encrypted = ((h&0x80)!=0);
    } else {
        is_encrypted = true;
    }

    AP4_Size crypto_header_size = (m_SelectiveEncryption?1:0)+(is_encrypted?m_IvLength:0);
    return sample.GetSize()-crypto_header_size;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleDecrypter::AP4_OmaDcfCbcSampleDecrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCbcSampleDecrypter::AP4_OmaDcfCbcSampleDecrypter(
    const AP4_UI08* key,
    bool            selective_encryption) :
    AP4_OmaDcfSampleDecrypter(AP4_AES_BLOCK_SIZE, selective_encryption)
{
    m_Cipher = new AP4_CbcStreamCipher(key, AP4_CbcStreamCipher::DECRYPT);
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleDecrypter::~AP4_OmaDcfCbcSampleDecrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCbcSampleDecrypter::~AP4_OmaDcfCbcSampleDecrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_OmaDbcCbcSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfCbcSampleDecrypter::DecryptSampleData(AP4_DataBuffer& data_in,
                                                AP4_DataBuffer& data_out)
{   
    bool                 is_encrypted = true;
    const unsigned char* in = data_in.GetData();
    AP4_Size             in_size = data_in.GetDataSize();
    AP4_Size             out_size;

    // default to 0 output 
    data_out.SetDataSize(0);

    // check the selective encryption flag
    if (m_SelectiveEncryption) {
        if (in_size < 1) return AP4_ERROR_INVALID_FORMAT;
        is_encrypted = ((in[0]&0x80)!=0);
        in++;
    }

    // check the size
    unsigned int header_size = (m_SelectiveEncryption?1:0)+(is_encrypted?m_IvLength:0);
    if (header_size > in_size) return AP4_ERROR_INVALID_FORMAT;

    // process the sample data
    unsigned int payload_size = in_size-header_size;
    data_out.Reserve(payload_size);
    unsigned char* out = data_out.UseData();
    if (is_encrypted) {
        // get the IV
        const AP4_UI08* iv = (const AP4_UI08*)in;
        in += AP4_AES_BLOCK_SIZE;

        m_Cipher->SetIV(iv);
        out_size = payload_size;
        AP4_CHECK(m_Cipher->ProcessBuffer(in, payload_size, out, &out_size, true));
    } else {
        AP4_CopyMemory(out, in, payload_size);
        out_size = payload_size;
    }

    data_out.SetDataSize(out_size);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleDecrypter::GetDecryptedSampleSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_OmaDcfCbcSampleDecrypter::GetDecryptedSampleSize(AP4_Sample& sample)
{
    if (m_Cipher == NULL) return 0;

    // decide if this sample is encrypted or not
    bool is_encrypted;
    if (m_SelectiveEncryption) {
        // read the first byte to see if the sample is encrypted or not
        AP4_Byte h;
        AP4_DataBuffer peek_buffer;
        peek_buffer.SetBuffer(&h, 1);
        sample.ReadData(peek_buffer, 1);
        is_encrypted = ((h&0x80)!=0);
    } else {
        is_encrypted = true;
    }

    if (is_encrypted) {
        // with CBC, we need to decrypt the last block to know what the padding was
        AP4_Size crypto_header_size = (m_SelectiveEncryption?1:0)+m_IvLength;
        AP4_Size encrypted_size = sample.GetSize()-crypto_header_size;
        AP4_DataBuffer encrypted;
        AP4_DataBuffer decrypted;
        AP4_Size       decrypted_size = AP4_AES_BLOCK_SIZE;
        if (sample.GetSize() < crypto_header_size+AP4_AES_BLOCK_SIZE) {
            return 0;
        }
        AP4_Size offset = sample.GetSize()-2*AP4_AES_BLOCK_SIZE;
        if (AP4_FAILED(sample.ReadData(encrypted, 2*AP4_AES_BLOCK_SIZE, offset))) {
            return 0;
        }
        decrypted.Reserve(decrypted_size);
        m_Cipher->SetIV(encrypted.GetData());
        if (AP4_FAILED(m_Cipher->ProcessBuffer(encrypted.GetData()+AP4_AES_BLOCK_SIZE, AP4_AES_BLOCK_SIZE,
                                               decrypted.UseData(), &decrypted_size, true))) {
            return 0;
        }
        unsigned int padding_size = AP4_AES_BLOCK_SIZE-decrypted_size;
        return encrypted_size-padding_size;
    } else {
        return sample.GetSize()-(m_SelectiveEncryption?1:0);
    }
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfSampleEncrypter::AP4_OmaDcfSampleEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfSampleEncrypter::AP4_OmaDcfSampleEncrypter(const AP4_UI08* salt)
{
    // left-align the salt
    unsigned int i=0;
    if (salt) {
        for (; i<8; i++) {
            m_Salt[i] = salt[i];
        }
    }
    for (; i<AP4_AES_KEY_LENGTH; i++) {
        m_Salt[i] = 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleEncrypter::AP4_OmaDcfCtrSampleEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCtrSampleEncrypter::AP4_OmaDcfCtrSampleEncrypter(const AP4_UI08* key,
                                                           const AP4_UI08* salt) :
    AP4_OmaDcfSampleEncrypter(salt)    
{
    m_Cipher = new AP4_CtrStreamCipher(key, m_Salt, 8);
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleEncrypter::~AP4_OmaDcfCtrSampleEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCtrSampleEncrypter::~AP4_OmaDcfCtrSampleEncrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfCtrSampleEncrypter::EncryptSampleData(AP4_DataBuffer& data_in,
                                                AP4_DataBuffer& data_out,
                                                AP4_UI64        counter,
                                                bool            /*skip_encryption*/)
{
    // setup the buffers
    const unsigned char* in = data_in.GetData();
    data_out.SetDataSize(data_in.GetDataSize()+AP4_AES_BLOCK_SIZE+1);
    unsigned char* out = data_out.UseData();

    // selective encryption flag
    *out++ = 0x80;

    // IV on 16 bytes: [SSSSSSSSXXXXXXXX]
    // where SSSSSSSS is the 64-bit salt and
    // XXXXXXXX is the 64-bit base counter
    AP4_CopyMemory(out, m_Salt, 8);
    AP4_BytesFromUInt64BE(&out[8], counter);

    // encrypt the payload
    m_Cipher->SetBaseCounter(out+8);
    m_Cipher->ProcessBuffer(in, out+AP4_AES_BLOCK_SIZE, data_in.GetDataSize());

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleEncrypter::GetEncryptedSampleSize
+---------------------------------------------------------------------*/
AP4_Size 
AP4_OmaDcfCtrSampleEncrypter::GetEncryptedSampleSize(AP4_Sample& sample)
{
    return sample.GetSize()+AP4_AES_BLOCK_SIZE+1;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleEncrypter::AP4_OmaDcfCbcSampleEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCbcSampleEncrypter::AP4_OmaDcfCbcSampleEncrypter(const AP4_UI08* key,
                                                           const AP4_UI08* salt) :
    AP4_OmaDcfSampleEncrypter(salt)    
{
    m_Cipher = new AP4_CbcStreamCipher(key, AP4_CbcStreamCipher::ENCRYPT);
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleEncrypter::~AP4_OmaDcfCbcSampleEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCbcSampleEncrypter::~AP4_OmaDcfCbcSampleEncrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfCbcSampleEncrypter::EncryptSampleData(AP4_DataBuffer& data_in,
                                                AP4_DataBuffer& data_out,
                                                AP4_UI64        counter,
                                                bool            /*skip_encryption*/)
{
    // make sure there is enough space in the output buffer
    data_out.Reserve(data_in.GetDataSize()+2*AP4_AES_BLOCK_SIZE+1);

    // setup the buffers
    AP4_Size out_size = data_in.GetDataSize()+AP4_AES_BLOCK_SIZE;
    unsigned char* out = data_out.UseData();

    // selective encryption flag
    *out++ = 0x80;

    // IV on 16 bytes: [SSSSSSSSXXXXXXXX]
    // where SSSSSSSS is the 64-bit salt and
    // XXXXXXXX is the 64-bit base counter
    AP4_CopyMemory(out, m_Salt, 8);
    AP4_BytesFromUInt64BE(&out[8], counter);

    // encrypt the payload
    m_Cipher->SetIV(out);
    m_Cipher->ProcessBuffer(data_in.GetData(), 
                            data_in.GetDataSize(),
                            out+AP4_AES_BLOCK_SIZE, 
                            &out_size,
                            true);
    data_out.SetDataSize(out_size+AP4_AES_BLOCK_SIZE+1);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleEncrypter::GetEncryptedSampleSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_OmaDcfCbcSampleEncrypter::GetEncryptedSampleSize(AP4_Sample& sample)
{
    AP4_Size sample_size = sample.GetSize();
    AP4_Size padding_size = AP4_AES_BLOCK_SIZE-(sample_size%AP4_AES_BLOCK_SIZE);
    return sample_size+padding_size+AP4_AES_BLOCK_SIZE+1;
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
    // create the cipher
    AP4_OmaDcfSampleDecrypter* cipher = AP4_OmaDcfSampleDecrypter::Create(sample_description, key, AP4_AES_BLOCK_SIZE);
    if (cipher == NULL) return NULL;

    // instantiate the object
    return new AP4_OmaDcfTrackDecrypter(cipher, 
                                        sample_entry, 
                                        sample_description->GetOriginalFormat());
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackDecrypter::AP4_OmaDcfTrackDecrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfTrackDecrypter::AP4_OmaDcfTrackDecrypter(AP4_OmaDcfSampleDecrypter* cipher,
                                                   AP4_SampleEntry*           sample_entry,
                                                   AP4_UI32                   original_format) :
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
    return m_Cipher->GetDecryptedSampleSize(sample);
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
    return m_Cipher->DecryptSampleData(data_in, data_out);
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackEncrypter
+---------------------------------------------------------------------*/
class AP4_OmaDcfTrackEncrypter : public AP4_Processor::TrackHandler {
public:
    // constructor
    AP4_OmaDcfTrackEncrypter(AP4_OmaDcfCipherMode cipher_mode,
                             const AP4_UI08*      key,
                             const AP4_UI08*      iv,
                             AP4_SampleEntry*     sample_entry,
                             AP4_UI32             format,
                             const char*          content_id,
                             const char*          rights_issuer_url);
    virtual ~AP4_OmaDcfTrackEncrypter();

    // methods
    virtual AP4_Size   GetProcessedSampleSize(AP4_Sample& sample);
    virtual AP4_Result ProcessTrack();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);

private:
    // members
    AP4_OmaDcfSampleEncrypter* m_Cipher;
    AP4_UI08                   m_CipherMode;
    AP4_UI08                   m_CipherPadding;
    AP4_SampleEntry*           m_SampleEntry;
    AP4_UI32                   m_Format;
    AP4_String                 m_ContentId;
    AP4_String                 m_RightsIssuerUrl;
    AP4_UI64                   m_Counter;
};

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackEncrypter::AP4_OmaDcfTrackEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfTrackEncrypter::AP4_OmaDcfTrackEncrypter(
    AP4_OmaDcfCipherMode cipher_mode,
    const AP4_UI08*      key,
    const AP4_UI08*      salt,
    AP4_SampleEntry*     sample_entry,
    AP4_UI32             format,
    const char*          content_id,
    const char*          rights_issuer_url) :
    m_SampleEntry(sample_entry),
    m_Format(format),
    m_ContentId(content_id),
    m_RightsIssuerUrl(rights_issuer_url),
    m_Counter(0)
{
    // instantiate the cipher (fixed params for now)
    if (cipher_mode == AP4_OMA_DCF_CIPHER_MODE_CBC) {
        m_Cipher        = new AP4_OmaDcfCbcSampleEncrypter(key, salt);
        m_CipherMode    = AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC;
        m_CipherPadding = AP4_OMA_DCF_PADDING_SCHEME_RFC_2630;
    } else {
        m_Cipher        = new AP4_OmaDcfCtrSampleEncrypter(key, salt);
        m_CipherMode    = AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR;
        m_CipherPadding = AP4_OMA_DCF_PADDING_SCHEME_NONE;
    }
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
    return m_Cipher->GetEncryptedSampleSize(sample);
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
    AP4_OhdrAtom*      ohdr = new AP4_OhdrAtom(m_CipherMode,
                                               m_CipherPadding,
                                               0,
                                               m_ContentId.GetChars(),
                                               m_RightsIssuerUrl.GetChars(),
                                               NULL);
    AP4_ContainerAtom* odkm = new AP4_ContainerAtom(AP4_ATOM_TYPE_ODKM, AP4_FULL_ATOM_HEADER_SIZE, 0, 0);
    AP4_SchmAtom*      schm = new AP4_SchmAtom(AP4_PROTECTION_SCHEME_TYPE_OMA, 
                                               AP4_PROTECTION_SCHEME_VERSION_OMA_20);
    odkm->AddChild(odaf);
    odkm->AddChild(ohdr);

    // populate the schi container
    schi->AddChild(odkm);

    // populate the sinf container
    sinf->AddChild(frma);
    sinf->AddChild(schm);
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
    AP4_Result result = m_Cipher->EncryptSampleData(data_in, 
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
|   AP4_OmaDcfEncryptingProcessor:AP4_OmaDcfEncryptingProcessor
+---------------------------------------------------------------------*/
AP4_OmaDcfEncryptingProcessor::AP4_OmaDcfEncryptingProcessor(AP4_OmaDcfCipherMode cipher_mode) :
    m_CipherMode(cipher_mode)
{
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
            return new AP4_OmaDcfTrackEncrypter(m_CipherMode, key, iv, entry, format, content_id, rights_issuer_url);
        }
    }

    return NULL;
}
