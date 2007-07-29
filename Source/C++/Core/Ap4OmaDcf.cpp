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
#include "Ap4OddaAtom.h"
#include "Ap4OdheAtom.h"

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::AP4_OmaDecryptingStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_OmaDecryptingStream::Create(AP4_OmaDcfCipherMode         mode,
                                AP4_ByteStream*              source_stream,
                                AP4_Position                 source_position,
                                const AP4_UI08*              key,
                                AP4_Size                     key_size,
                                AP4_BlockCipherFactory*      block_cipher_factory,
                                AP4_LargeSize                cleartext_size,
                                AP4_OmaDecryptingStream*& stream)
{
    // default return value
    stream = NULL;

    // create the stream cipher
    AP4_BlockCipher* block_cipher;
    AP4_Result result = block_cipher_factory->Create(AP4_BlockCipher::AES_128,
                                                     AP4_BlockCipher::DECRYPT,
                                                     key, key_size, block_cipher);
    if (AP4_FAILED(result)) return result;

    // keep a reference to the source stream
    source_stream->AddReference();

    // create the stream
    stream = new AP4_OmaDecryptingStream();
    stream->m_Size           = cleartext_size;
    stream->m_Position       = 0;
    stream->m_SourceStream   = source_stream;
    stream->m_SourceStart    = source_position+16;
    stream->m_SourcePosition = 0;
    stream->m_BufferFullness = 0;
    stream->m_BufferOffset   = 0;
    stream->m_ReferenceCount = 1;

    // create the cipher according to the mode
    switch (mode) {
        case AP4_OMA_DCF_CIPHER_MODE_CBC:
            stream->m_StreamCipher = new AP4_CbcStreamCipher(block_cipher, 
                                                             AP4_StreamCipher::DECRYPT);
            break;
        case AP4_OMA_DCF_CIPHER_MODE_CTR:
            stream->m_StreamCipher = new AP4_CtrStreamCipher(block_cipher,
                                                             NULL,
                                                             AP4_CIPHER_BLOCK_SIZE);
            break;
        default:
            // should never occur
            AP4_ASSERT(0);
    }
    
    // set the IV
    AP4_UI08 iv[16];
    AP4_Size bytes_read = 0;
    result = stream->m_SourceStream->Seek(source_position);
    if (AP4_FAILED(result)) return result;
    result = stream->m_SourceStream->Read(iv, 16, &bytes_read);
    if (AP4_FAILED(result)) return result;
    if (bytes_read != 16) return AP4_ERROR_INVALID_FORMAT;
    stream->m_StreamCipher->SetIV(iv);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::~AP4_OmaDecryptingStream
+---------------------------------------------------------------------*/
AP4_OmaDecryptingStream::~AP4_OmaDecryptingStream()
{
    delete m_StreamCipher;
    m_SourceStream->Release();
}

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::Read
+---------------------------------------------------------------------*/
void 
AP4_OmaDecryptingStream::AddReference()
{
    ++m_ReferenceCount;
}

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::Read
+---------------------------------------------------------------------*/
void 
AP4_OmaDecryptingStream::Release()
{
    if (--m_ReferenceCount == 0) delete this;
}

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::Read
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDecryptingStream::Read(void*     buffer, 
                              AP4_Size  bytes_to_read, 
                              AP4_Size* bytes_read)
{
    AP4_Size total_read = 0;
    if (bytes_read) *bytes_read = 0;

    // never read more than what's available
    AP4_LargeSize available = m_Size-m_Position;
    if (available < bytes_to_read) bytes_to_read = (AP4_Size)available;

    if (m_BufferFullness) {
        // we have some leftovers
        AP4_Size chunk = bytes_to_read;
        if (chunk > m_BufferFullness) chunk = m_BufferFullness;
        AP4_CopyMemory(buffer, &m_Buffer[m_BufferOffset], chunk);
        buffer = (char*)buffer+chunk;
        m_Position += chunk;
        available -= chunk;
        bytes_to_read -= chunk;
        m_BufferFullness -= chunk;
        m_BufferOffset += chunk;
        total_read += chunk;
        if (bytes_read) *bytes_read = total_read;
    }

    while (bytes_to_read) {
        // seek to the right place in the input
        m_SourceStream->Seek(m_SourceStart+m_SourcePosition);

        // read from the source
        AP4_UI08 encrypted[16];
        AP4_Size encrypted_read = 0;
        AP4_Result result = m_SourceStream->Read(encrypted, 16, &encrypted_read);
        if (result == AP4_ERROR_EOS) {
            if (total_read == 0) return AP4_ERROR_EOS;
        } else if (result != AP4_SUCCESS) {
            return result;
        } else {
            m_SourcePosition += encrypted_read;
        }
        bool is_last_buffer = (available <= 16);
        AP4_Size buffer_size = 16;
        result = m_StreamCipher->ProcessBuffer(encrypted, 
                                               16, 
                                               m_Buffer, 
                                               &buffer_size,
                                               is_last_buffer,
                                               NULL);
        m_BufferOffset = 0;
        m_BufferFullness = buffer_size;

        AP4_Size chunk = bytes_to_read;
        if (chunk > m_BufferFullness) chunk = m_BufferFullness;
        AP4_CopyMemory(buffer, &m_Buffer[m_BufferOffset], chunk);
        buffer = (char*)buffer+chunk;
        m_Position += chunk;
        available -= chunk;
        bytes_to_read -= chunk;
        m_BufferFullness -= chunk;
        m_BufferOffset += chunk;
        total_read += chunk;
        if (bytes_read) *bytes_read = total_read;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::Write
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDecryptingStream::Write(const void* /* buffer */, 
                                  AP4_Size    /* bytes_to_write */, 
                                  AP4_Size*   /* bytes_written */)
{
    return AP4_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::Seek
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDecryptingStream::Seek(AP4_Position position)
{
    AP4_Cardinal preroll = 0;
    
    // try to put the stream cipher at the right offset
    AP4_CHECK(m_StreamCipher->SetStreamOffset(position, &preroll));

    // seek in the source stream
    AP4_CHECK(m_SourceStream->Seek(position-preroll));
    
    // if we need to, process the preroll bytes
    if (preroll > 0) {
        AP4_Size out_size = 0;
        AP4_UI08 buffer[2*AP4_CIPHER_BLOCK_SIZE]; // bigger than preroll
        AP4_CHECK(m_SourceStream->ReadFully(buffer, preroll));
        AP4_CHECK(m_StreamCipher->ProcessBuffer(buffer, preroll, buffer, &out_size));
        AP4_ASSERT(out_size == 0); // we're just feeding prerolled bytes, 
                                   // there can be no output
    }
    
    // update the counters
    m_Position = position;
    m_SourcePosition = position;
    m_BufferFullness = 0;
    m_BufferOffset = 0;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::Tell
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDecryptingStream::Tell(AP4_Position& position)
{
    position = m_Position;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDecryptingStream::GetSize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDecryptingStream::GetSize(AP4_LargeSize& size)
{
    size = m_Size;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfAtomDecrypter::DecryptAtoms
+---------------------------------------------------------------------*/
AP4_Result
AP4_OmaDcfAtomDecrypter::DecryptAtoms(AP4_AtomParent&                  atoms, 
                                      AP4_Processor::ProgressListener* /*listener*/,
                                      AP4_BlockCipherFactory*          block_cipher_factory,
                                      AP4_ProtectionKeyMap&            key_map)
{
    AP4_List<AP4_Atom>::Item* item = atoms.GetChildren().FirstItem();
    unsigned int index = 1;
    while (item) {
        AP4_Atom* atom = item->GetData();
        if (atom->GetType() == AP4_ATOM_TYPE_ODRM) {
            // check that we have the key
            const AP4_UI08* key = key_map.GetKey(index++);
            if (key == NULL) return AP4_ERROR_INVALID_PARAMETERS;

            // check that we have the atoms
            AP4_ContainerAtom* odrm = dynamic_cast<AP4_ContainerAtom*>(atom);
            if (odrm) {
                AP4_OdheAtom* odhe = dynamic_cast<AP4_OdheAtom*>(odrm->FindChild("odhe"));
                AP4_OddaAtom* odda = dynamic_cast<AP4_OddaAtom*>(odrm->FindChild("odda"));;
                if (odhe && odda) {
                    AP4_OhdrAtom* ohdr = dynamic_cast<AP4_OhdrAtom*>(odhe->FindChild("ohdr"));
                    if (ohdr) {
                        // create the byte stream
                        AP4_ByteStream* cipher_stream = NULL;
                        AP4_Result result = CreateDecryptingStream(*odrm, 
                                                                   key, 
                                                                   16, 
                                                                   block_cipher_factory, 
                                                                   cipher_stream);
                        if (AP4_SUCCEEDED(result)) {
                            odda->SetSourceStream(cipher_stream, 0);
                            cipher_stream->Release();
                        }

                        // the atom will now be in the clear
                        ohdr->SetEncryptionMethod(AP4_OMA_DCF_ENCRYPTION_METHOD_NULL);
                        ohdr->SetPaddingScheme(AP4_OMA_DCF_PADDING_SCHEME_NONE);

                        // the new encrypted size is the cleartext size
                        odda->SetEncryptedDataLength(ohdr->GetPlaintextLength());
                    }
                }
            }
        }
        item = item->GetNext();
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfAtomDecrypter::CreateDecryptingStream
+---------------------------------------------------------------------*/
AP4_Result 
AP4_OmaDcfAtomDecrypter::CreateDecryptingStream(AP4_ContainerAtom&      odrm,
                                                const AP4_UI08*         key,
                                                AP4_Size                key_size,
                                                AP4_BlockCipherFactory* block_cipher_factory,
                                                AP4_ByteStream*&        stream)
{
    // default return value
    stream = NULL;

    AP4_OdheAtom* odhe = dynamic_cast<AP4_OdheAtom*>(odrm.FindChild("odhe"));
    AP4_OddaAtom* odda = dynamic_cast<AP4_OddaAtom*>(odrm.FindChild("odda"));;
    if (odhe == NULL || odda == NULL) return AP4_ERROR_INVALID_FORMAT;
    AP4_OhdrAtom* ohdr = dynamic_cast<AP4_OhdrAtom*>(odhe->FindChild("ohdr"));
    if (ohdr == NULL) return AP4_ERROR_INVALID_FORMAT;
    
    AP4_OmaDcfCipherMode mode;
    switch (ohdr->GetEncryptionMethod()) {
        case AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC:
            mode = AP4_OMA_DCF_CIPHER_MODE_CBC;
            break;
        case AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR:
            mode = AP4_OMA_DCF_CIPHER_MODE_CTR;
            break;
        default:
            return AP4_ERROR_NOT_SUPPORTED;
    }
        
    AP4_OmaDecryptingStream* dec_stream = NULL;
    AP4_Result result = AP4_OmaDecryptingStream::Create(mode,
                                                        odda->GetSourceStream(), 
                                                        odda->GetSourcePosition(), 
                                                        key, key_size, 
                                                        block_cipher_factory,
                                                        ohdr->GetPlaintextLength(),
                                                        dec_stream);
    if (AP4_FAILED(result)) return result;
    stream = dec_stream;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_OmaDcfSampleDecrypter::Create(AP4_ProtectedSampleDescription* sample_description, 
                                  const AP4_UI08*                 key, 
                                  AP4_Size                        key_size,
                                  AP4_BlockCipherFactory*         block_cipher_factory,
                                  AP4_OmaDcfSampleDecrypter*&     cipher)
{
    // check the parameters
    if (key == NULL || block_cipher_factory == NULL) {
        return AP4_ERROR_INVALID_PARAMETERS;
    }
    
    // default return value
    cipher = NULL;

    // get the scheme info atom
    AP4_ContainerAtom* schi = sample_description->GetSchemeInfo()->GetSchiAtom();
    if (schi == NULL) return AP4_ERROR_INVALID_FORMAT;

    // get and check the cipher params
    // NOTE: we only support an IV Length less than or equal to the cipher block size, 
    // and we don't know how to deal with a key indicator length != 0
    AP4_OdafAtom* odaf = dynamic_cast<AP4_OdafAtom*>(schi->FindChild("odkm/odaf"));
    if (odaf) {
        if (odaf->GetIvLength() > AP4_CIPHER_BLOCK_SIZE) return AP4_ERROR_INVALID_FORMAT;
        if (odaf->GetKeyIndicatorLength() != 0) return AP4_ERROR_INVALID_FORMAT;
    }

    // check the scheme details and create the cipher
    AP4_OhdrAtom* ohdr = dynamic_cast<AP4_OhdrAtom*>(schi->FindChild("odkm/ohdr"));
    if (ohdr == NULL) return AP4_ERROR_INVALID_FORMAT;
    AP4_UI08 encryption_method = ohdr->GetEncryptionMethod();
    if (encryption_method == AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC) {
        // in CBC mode, we only support IVs of the same size as the cipher block size
        if (odaf->GetIvLength() != AP4_CIPHER_BLOCK_SIZE) return AP4_ERROR_INVALID_FORMAT;

        // require RFC_2630 padding
        if (ohdr->GetPaddingScheme() != AP4_OMA_DCF_PADDING_SCHEME_RFC_2630) {
            return AP4_ERROR_NOT_SUPPORTED;
        }

        // create the block cipher
        AP4_BlockCipher* block_cipher = NULL;
        AP4_Result result = block_cipher_factory->Create(AP4_BlockCipher::AES_128, 
                                                         AP4_BlockCipher::DECRYPT, 
                                                         key, 
                                                         key_size, 
                                                         block_cipher);
        if (AP4_FAILED(result)) return result;

        // create the cipher
        cipher = new AP4_OmaDcfCbcSampleDecrypter(block_cipher, 
                                                  odaf->GetSelectiveEncryption());
        return AP4_SUCCESS;
    } else if (encryption_method == AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CTR) {
        // require NONE padding
        if (ohdr->GetPaddingScheme() != AP4_OMA_DCF_PADDING_SCHEME_NONE) {
            return AP4_ERROR_INVALID_FORMAT;
        }

        // create the block cipher
        AP4_BlockCipher* block_cipher = NULL;
        AP4_Result result = block_cipher_factory->Create(AP4_BlockCipher::AES_128, 
                                                         AP4_BlockCipher::ENCRYPT, 
                                                         key, 
                                                         key_size, 
                                                         block_cipher);
        if (AP4_FAILED(result)) return result;

        // create the cipher
        cipher = new AP4_OmaDcfCtrSampleDecrypter(block_cipher, 
                                                  odaf->GetIvLength(), 
                                                  odaf->GetSelectiveEncryption());
        return AP4_SUCCESS;
    } else {
        return AP4_ERROR_NOT_SUPPORTED;
    }
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleDecrypter::AP4_OmaDcfCtrSampleDecrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCtrSampleDecrypter::AP4_OmaDcfCtrSampleDecrypter(
    AP4_BlockCipher* block_cipher,
    AP4_Size         iv_length,
    bool             selective_encryption) :
    AP4_OmaDcfSampleDecrypter(iv_length, selective_encryption)
{
    m_Cipher = new AP4_CtrStreamCipher(block_cipher, NULL, iv_length);
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
    AP4_Size payload_size = in_size-header_size;
    data_out.Reserve(payload_size);
    unsigned char* out = data_out.UseData();
    if (is_encrypted) {
        // set the IV
        m_Cipher->SetIV(in);
        AP4_CHECK(m_Cipher->ProcessBuffer(in+m_IvLength, 
                                          payload_size, 
                                          out));
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
    AP4_BlockCipher* block_cipher,
    bool             selective_encryption) :
    AP4_OmaDcfSampleDecrypter(AP4_CIPHER_BLOCK_SIZE, selective_encryption)
{
    m_Cipher = new AP4_CbcStreamCipher(block_cipher, AP4_CbcStreamCipher::DECRYPT);
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
        in += AP4_CIPHER_BLOCK_SIZE;

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
        AP4_Size       decrypted_size = AP4_CIPHER_BLOCK_SIZE;
        if (sample.GetSize() < crypto_header_size+AP4_CIPHER_BLOCK_SIZE) {
            return 0;
        }
        AP4_Size offset = sample.GetSize()-2*AP4_CIPHER_BLOCK_SIZE;
        if (AP4_FAILED(sample.ReadData(encrypted, 2*AP4_CIPHER_BLOCK_SIZE, offset))) {
            return 0;
        }
        decrypted.Reserve(decrypted_size);
        m_Cipher->SetIV(encrypted.GetData());
        if (AP4_FAILED(m_Cipher->ProcessBuffer(encrypted.GetData()+AP4_CIPHER_BLOCK_SIZE, 
                                               AP4_CIPHER_BLOCK_SIZE,
                                               decrypted.UseData(), 
                                               &decrypted_size, 
                                               true))) {
            return 0;
        }
        unsigned int padding_size = AP4_CIPHER_BLOCK_SIZE-decrypted_size;
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
    for (; i<sizeof(m_Salt)/sizeof(m_Salt[0]); i++) {
        m_Salt[i] = 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleEncrypter::AP4_OmaDcfCtrSampleEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCtrSampleEncrypter::AP4_OmaDcfCtrSampleEncrypter(AP4_BlockCipher* block_cipher,
                                                           const AP4_UI08*  salt) :
    AP4_OmaDcfSampleEncrypter(salt)    
{
    m_Cipher = new AP4_CtrStreamCipher(block_cipher, m_Salt, 8);
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
    data_out.SetDataSize(data_in.GetDataSize()+AP4_CIPHER_BLOCK_SIZE+1);
    unsigned char* out = data_out.UseData();

    // selective encryption flag
    *out++ = 0x80;

    // IV on 16 bytes: [SSSSSSSSXXXXXXXX]
    // where SSSSSSSS is the 64-bit salt and
    // XXXXXXXX is the 64-bit base counter
    AP4_CopyMemory(out, m_Salt, 8);
    AP4_BytesFromUInt64BE(&out[8], counter);

    // encrypt the payload
    AP4_Size data_size = data_in.GetDataSize();
    m_Cipher->SetIV(out+8);
    m_Cipher->ProcessBuffer(in, data_size, out+AP4_CIPHER_BLOCK_SIZE);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCtrSampleEncrypter::GetEncryptedSampleSize
+---------------------------------------------------------------------*/
AP4_Size 
AP4_OmaDcfCtrSampleEncrypter::GetEncryptedSampleSize(AP4_Sample& sample)
{
    return sample.GetSize()+AP4_CIPHER_BLOCK_SIZE+1;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleEncrypter::AP4_OmaDcfCbcSampleEncrypter
+---------------------------------------------------------------------*/
AP4_OmaDcfCbcSampleEncrypter::AP4_OmaDcfCbcSampleEncrypter(AP4_BlockCipher* block_cipher,
                                                           const AP4_UI08* salt) :
    AP4_OmaDcfSampleEncrypter(salt)    
{
    m_Cipher = new AP4_CbcStreamCipher(block_cipher, AP4_CbcStreamCipher::ENCRYPT);
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
    data_out.Reserve(data_in.GetDataSize()+2*AP4_CIPHER_BLOCK_SIZE+1);

    // setup the buffers
    AP4_Size out_size = data_in.GetDataSize()+AP4_CIPHER_BLOCK_SIZE;
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
                            out+AP4_CIPHER_BLOCK_SIZE, 
                            &out_size,
                            true);
    data_out.SetDataSize(out_size+AP4_CIPHER_BLOCK_SIZE+1);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfCbcSampleEncrypter::GetEncryptedSampleSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_OmaDcfCbcSampleEncrypter::GetEncryptedSampleSize(AP4_Sample& sample)
{
    AP4_Size sample_size = sample.GetSize();
    AP4_Size padding_size = AP4_CIPHER_BLOCK_SIZE-(sample_size%AP4_CIPHER_BLOCK_SIZE);
    return sample_size+padding_size+AP4_CIPHER_BLOCK_SIZE+1;
}

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_OmaDcfTrackDecrypter::Create(
    const AP4_UI08*                 key,
    AP4_Size                        key_size,
    AP4_ProtectedSampleDescription* sample_description,
    AP4_SampleEntry*                sample_entry,
    AP4_BlockCipherFactory*         block_cipher_factory,
    AP4_OmaDcfTrackDecrypter*&      decrypter)
{
    // check and set defaults
    if (key == NULL) {
        return AP4_ERROR_INVALID_PARAMETERS;
    }
    if (block_cipher_factory == NULL) {
        block_cipher_factory = &AP4_DefaultBlockCipherFactory::Instance;
    }
    decrypter = NULL;

    // create the cipher
    AP4_OmaDcfSampleDecrypter* cipher = NULL;
    AP4_Result result = AP4_OmaDcfSampleDecrypter::Create(sample_description, 
                                                          key, 
                                                          key_size, 
                                                          block_cipher_factory,
                                                          cipher);
    if (AP4_FAILED(result)) return result;

    // instantiate the object
    decrypter = new AP4_OmaDcfTrackDecrypter(cipher, 
                                            sample_entry, 
                                            sample_description->GetOriginalFormat());
    return AP4_SUCCESS;
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
                             AP4_BlockCipher*     block_cipher,
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
    AP4_BlockCipher*     block_cipher,
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
        m_Cipher        = new AP4_OmaDcfCbcSampleEncrypter(block_cipher, salt);
        m_CipherMode    = AP4_OMA_DCF_ENCRYPTION_METHOD_AES_CBC;
        m_CipherPadding = AP4_OMA_DCF_PADDING_SCHEME_RFC_2630;
    } else {
        m_Cipher        = new AP4_OmaDcfCtrSampleEncrypter(block_cipher, salt);
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
    AP4_OdafAtom*      odaf = new AP4_OdafAtom(true, 0, AP4_CIPHER_BLOCK_SIZE);
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

    m_Counter += (data_in.GetDataSize()+AP4_CIPHER_BLOCK_SIZE-1)/AP4_CIPHER_BLOCK_SIZE;
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
AP4_OmaDcfEncryptingProcessor::AP4_OmaDcfEncryptingProcessor(AP4_OmaDcfCipherMode    cipher_mode,
                                                             AP4_BlockCipherFactory* block_cipher_factory) :
    m_CipherMode(cipher_mode)
{
    if (block_cipher_factory == NULL) {
        m_BlockCipherFactory = &AP4_DefaultBlockCipherFactory::Instance;
    } else {
        m_BlockCipherFactory = block_cipher_factory;
    }
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

            // create the block cipher
            AP4_BlockCipher* block_cipher = NULL;
            AP4_Result result = m_BlockCipherFactory->Create(AP4_BlockCipher::AES_128, 
                                                             AP4_BlockCipher::ENCRYPT, 
                                                             key, 
                                                             AP4_CIPHER_BLOCK_SIZE, 
                                                             block_cipher);
            if (AP4_FAILED(result)) return NULL;
            return new AP4_OmaDcfTrackEncrypter(m_CipherMode, block_cipher, iv, entry, format, content_id, rights_issuer_url);
        }
    }

    return NULL;
}
