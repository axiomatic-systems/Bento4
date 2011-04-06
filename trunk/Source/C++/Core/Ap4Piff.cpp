/*****************************************************************
|
|    AP4 - PIFF Support
|
|    Copyright 2002-2009 Axiomatic Systems, LLC
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
#include "Ap4IsfmAtom.h"
#include "Ap4IsltAtom.h"
#include "Ap4Utils.h"
#include "Ap4TrakAtom.h"
#include "Ap4Piff.h"
#include "Ap4FtypAtom.h"
#include "Ap4HdlrAtom.h"
#include "Ap4TrunAtom.h"
#include "Ap4TfhdAtom.h"
#include "Ap4Marlin.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
AP4_UI08 const AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM[16] = {
    0x89, 0x74, 0xdb, 0xce, 0x7b, 0xe7, 0x4c, 0x51, 0x84, 0xf9, 0x71, 0x48, 0xf9, 0x88, 0x25, 0x54
};
AP4_UI08 const AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM[16] = {
    0xA2, 0x39, 0x4F, 0x52, 0x5A, 0x9B, 0x4f, 0x14, 0xA2, 0x44, 0x6C, 0x42, 0x7C, 0x64, 0x8D, 0xF4
};

/*----------------------------------------------------------------------
|   AP4_PiffCtrSampleEncrypter::AP4_PiffCtrSampleEncrypter
+---------------------------------------------------------------------*/
AP4_PiffCtrSampleEncrypter::AP4_PiffCtrSampleEncrypter(AP4_BlockCipher* block_cipher)
{
    m_Cipher = new AP4_CtrStreamCipher(block_cipher, 16);
}

/*----------------------------------------------------------------------
|   AP4_PiffCtrSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffCtrSampleEncrypter::EncryptSampleData(AP4_DataBuffer& /*data_in */,
                                              AP4_DataBuffer& /*data_out*/)
{
    return AP4_ERROR_NOT_SUPPORTED; // FIXME: not implemented yet
}

/*----------------------------------------------------------------------
|   AP4_PiffCtrSampleEncrypter::~AP4_PiffCtrSampleEncrypter
+---------------------------------------------------------------------*/
AP4_PiffCtrSampleEncrypter::~AP4_PiffCtrSampleEncrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_PiffSubSampleCtrSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSubSampleCtrSampleEncrypter::EncryptSampleData(AP4_DataBuffer& /*data_in */,
                                                       AP4_DataBuffer& /*data_out*/)
{
    return AP4_ERROR_NOT_SUPPORTED; // FIXME: not implemented yet
}

/*----------------------------------------------------------------------
|   AP4_PiffCbcSampleEncrypter::AP4_PiffCbcSampleEncrypter
+---------------------------------------------------------------------*/
AP4_PiffCbcSampleEncrypter::AP4_PiffCbcSampleEncrypter(AP4_BlockCipher* block_cipher)
{
    m_Cipher = new AP4_CbcStreamCipher(block_cipher);
}

/*----------------------------------------------------------------------
|   AP4_PiffCbcSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffCbcSampleEncrypter::EncryptSampleData(AP4_DataBuffer& data_in,
                                              AP4_DataBuffer& data_out)
{
    // the output has the same size as the input
    data_out.SetDataSize(data_in.GetDataSize());

    // setup direct pointers to the buffers
    const AP4_UI08* in  = data_in.GetData();
    AP4_UI08*       out = data_out.UseData();
    
    // setup the IV
    m_Cipher->SetIV(m_Iv);

    // process the sample data
    unsigned int block_count = data_in.GetDataSize()/16;
    if (block_count) {
        AP4_Size out_size = data_out.GetDataSize();
        AP4_Result result = m_Cipher->ProcessBuffer(in, block_count*16, out, &out_size, false);
        if (AP4_FAILED(result)) return result;
        in  += block_count*16;
        out += block_count*16;
        
        // update the IV (last cipherblock emitted)
        AP4_CopyMemory(m_Iv, out-16, 16);
    }
    
    // any partial block at the end remains in the clear
    unsigned int partial = data_in.GetDataSize()%16;
    if (partial) {
        AP4_CopyMemory(out, in, partial);
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffCbcSampleEncrypter::~AP4_PiffCbcSampleEncrypter
+---------------------------------------------------------------------*/
AP4_PiffCbcSampleEncrypter::~AP4_PiffCbcSampleEncrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_PiffSubSampleCbcSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSubSampleCbcSampleEncrypter::EncryptSampleData(AP4_DataBuffer& data_in,
                                                       AP4_DataBuffer& data_out)
{  
    // the output has the same size as the input
    data_out.SetDataSize(data_in.GetDataSize());

    // check some basics
    if (data_in.GetDataSize() == 0) return AP4_SUCCESS;

    // setup direct pointers to the buffers
    const AP4_UI08* in  = data_in.GetData();
    AP4_UI08*       out = data_out.UseData();
    
    // setup the IV
    m_Cipher->SetIV(m_Iv);

    // process the sample data, one NALU at a time
    const AP4_UI08* in_end = data_in.GetData()+data_in.GetDataSize();
    while ((AP4_Size)(in_end-in) > 1+m_NaluLengthSize) {
        unsigned int nalu_length;
        switch (m_NaluLengthSize) {
            case 1:
                nalu_length = *in;
                break;
                
            case 2:
                nalu_length = AP4_BytesToUInt16BE(in);
                break;
                
            case 4:
                nalu_length = AP4_BytesToUInt32BE(in);
                break;
                
            default:
                return AP4_ERROR_INVALID_FORMAT;
        }

        unsigned int chunk_size     = m_NaluLengthSize+nalu_length;
        unsigned int cleartext_size = chunk_size%16;
        unsigned int block_count    = chunk_size/16;
        if (cleartext_size < m_NaluLengthSize+1) {
            AP4_ASSERT(block_count);
            --block_count;
            cleartext_size += 16;
        }
        
        // copy the cleartext portion
        AP4_CopyMemory(out, in, cleartext_size);
        
        // encrypt the rest
        if (block_count) {
            AP4_Size out_size = block_count*16;
            m_Cipher->ProcessBuffer(in+cleartext_size, block_count*16, out+cleartext_size, &out_size, false);
        }
        
        // move the pointers
        in += chunk_size;
        out += chunk_size;
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncrypter
+---------------------------------------------------------------------*/
class AP4_PiffTrackEncrypter : public AP4_Processor::TrackHandler {
public:
    // constructor
    AP4_PiffTrackEncrypter(AP4_UI32           default_algorithm_id,
                           AP4_UI08           default_iv_size,
                           const AP4_UI08*    default_kid,
                           AP4_SampleEntry*   sample_entry,
                           AP4_UI32           format);

    // methods
    virtual AP4_Result ProcessTrack();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);

private:
    // members
    AP4_SampleEntry* m_SampleEntry;
    AP4_UI32         m_Format;
    AP4_UI32         m_DefaultAlgorithmId;
    AP4_UI08         m_DefaultIvSize;
    AP4_UI08         m_DefaultKid[16];
};

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncrypter::AP4_PiffTrackEncrypter
+---------------------------------------------------------------------*/
AP4_PiffTrackEncrypter::AP4_PiffTrackEncrypter(
    AP4_UI32           default_algorithm_id,
    AP4_UI08           default_iv_size,
    const AP4_UI08*    default_kid,
    AP4_SampleEntry*   sample_entry,
    AP4_UI32           format) :
    m_SampleEntry(sample_entry),
    m_Format(format),
    m_DefaultAlgorithmId(default_algorithm_id),
    m_DefaultIvSize(default_iv_size)
{
    // copy the KID
    AP4_CopyMemory(m_DefaultKid, default_kid, 16);
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncrypter::ProcessTrack
+---------------------------------------------------------------------*/
AP4_Result   
AP4_PiffTrackEncrypter::ProcessTrack()
{
    // original format
    AP4_FrmaAtom* frma = new AP4_FrmaAtom(m_SampleEntry->GetType());
    
    // scheme info
    AP4_SchmAtom* schm = new AP4_SchmAtom(AP4_PROTECTION_SCHEME_TYPE_PIFF, 
                                          AP4_PROTECTION_SCHEME_VERSION_PIFF_11);

    // populate the schi container
    AP4_PiffTrackEncryptionAtom* piff_enc = 
        new AP4_PiffTrackEncryptionAtom(m_DefaultAlgorithmId, 
                                        m_DefaultIvSize, 
                                        m_DefaultKid);
    AP4_ContainerAtom* schi = new AP4_ContainerAtom(AP4_ATOM_TYPE_SCHI);
    schi->AddChild(piff_enc);
        
    // populate the sinf container
    AP4_ContainerAtom* sinf = new AP4_ContainerAtom(AP4_ATOM_TYPE_SINF);
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
|   AP4_PiffTrackEncrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffTrackEncrypter::ProcessSample(AP4_DataBuffer& data_in,
                                      AP4_DataBuffer& data_out)
{
    return data_out.SetData(data_in.GetData(), data_in.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_PiffFragmentEncrypter
+---------------------------------------------------------------------*/
class AP4_PiffFragmentEncrypter : public AP4_Processor::FragmentHandler {
public:
    // constructor
    AP4_PiffFragmentEncrypter(AP4_ContainerAtom*                      traf,
                              AP4_PiffEncryptingProcessor::Encrypter* encrypter);

    // methods
    virtual AP4_Result ProcessFragment();
    virtual AP4_Result FinishFragment();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);

private:
    // members
    AP4_ContainerAtom*                      m_Traf;
    AP4_PiffSampleEncryptionAtom*           m_SampleEncryptionAtom;
    AP4_PiffEncryptingProcessor::Encrypter* m_Encrypter;
    AP4_Ordinal                             m_IvIndex;
};

/*----------------------------------------------------------------------
|   AP4_PiffFragmentEncrypter::AP4_PiffFragmentEncrypter
+---------------------------------------------------------------------*/
AP4_PiffFragmentEncrypter::AP4_PiffFragmentEncrypter(AP4_ContainerAtom*                      traf,
                                                     AP4_PiffEncryptingProcessor::Encrypter* encrypter) :
    m_Traf(traf),
    m_SampleEncryptionAtom(NULL),
    m_Encrypter(encrypter),
    m_IvIndex(0)
{
}

/*----------------------------------------------------------------------
|   AP4_PiffFragmentEncrypter::ProcessFragment
+---------------------------------------------------------------------*/
AP4_Result   
AP4_PiffFragmentEncrypter::ProcessFragment()
{
    // count the number of samples and create IVs
    unsigned int sample_count = 0;
    for (AP4_List<AP4_Atom>::Item* item = m_Traf->GetChildren().FirstItem();
                                   item;
                                   item = item->GetNext()) {
        AP4_Atom* child = item->GetData();
        if (child->GetType() == AP4_ATOM_TYPE_TRUN) {
            AP4_TrunAtom* trun = AP4_DYNAMIC_CAST(AP4_TrunAtom, child);
            if (trun) {
                sample_count += trun->GetEntries().ItemCount();
            }
        }
    }
    
    // create a sample encryption atom
    m_SampleEncryptionAtom = new AP4_PiffSampleEncryptionAtom(sample_count);
    if (m_Encrypter->m_SampleEncrypter->UseSubSamples()) {
        m_SampleEncryptionAtom->SetFlags(
            m_SampleEncryptionAtom->GetFlags() |
            AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION);
    }
    
    // add the atom to the traf container
    m_Traf->AddChild(m_SampleEncryptionAtom);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffFragmentEncrypter::FinishFragment
+---------------------------------------------------------------------*/
AP4_Result   
AP4_PiffFragmentEncrypter::FinishFragment()
{
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffFragmentEncrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffFragmentEncrypter::ProcessSample(AP4_DataBuffer& data_in,
                                         AP4_DataBuffer& data_out)
{
    // store the IV for this sample
    m_SampleEncryptionAtom->SetIv(m_IvIndex++, m_Encrypter->m_SampleEncrypter->GetIv());
    
    // encrypt the sample
    AP4_Result result = m_Encrypter->m_SampleEncrypter->EncryptSampleData(data_in, data_out);
    return result;
}

/*----------------------------------------------------------------------
|   AP4_PiffEncryptingProcessor:AP4_PiffEncryptingProcessor
+---------------------------------------------------------------------*/
AP4_PiffEncryptingProcessor::AP4_PiffEncryptingProcessor(AP4_PiffCipherMode      cipher_mode,
                                                         AP4_BlockCipherFactory* block_cipher_factory) :
    m_CipherMode(cipher_mode)
{
    // create a block cipher factory if none is given
    if (block_cipher_factory == NULL) {
        m_BlockCipherFactory = &AP4_DefaultBlockCipherFactory::Instance;
    } else {
        m_BlockCipherFactory = block_cipher_factory;
    }
}

/*----------------------------------------------------------------------
|   AP4_PiffEncryptingProcessor:~AP4_PiffEncryptingProcessor
+---------------------------------------------------------------------*/
AP4_PiffEncryptingProcessor::~AP4_PiffEncryptingProcessor()
{
    m_Encrypters.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_PiffEncryptingProcessor::Initialize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffEncryptingProcessor::Initialize(AP4_AtomParent&                  top_level,
                                        AP4_ByteStream&                  /*stream*/,
                                        AP4_Processor::ProgressListener* /*listener*/)
{
    AP4_FtypAtom* ftyp = AP4_DYNAMIC_CAST(AP4_FtypAtom, top_level.GetChild(AP4_ATOM_TYPE_FTYP));
    if (ftyp) {
        // remove the atom, it will be replaced with a new one
        top_level.RemoveChild(ftyp);
        
        // keep the existing brand and compatible brands
        AP4_Array<AP4_UI32> compatible_brands;
        compatible_brands.EnsureCapacity(ftyp->GetCompatibleBrands().ItemCount()+1);
        for (unsigned int i=0; i<ftyp->GetCompatibleBrands().ItemCount(); i++) {
            compatible_brands.Append(ftyp->GetCompatibleBrands()[i]);
        }
        
        // add the PIFF compatible brand if it is not already there
        if (!ftyp->HasCompatibleBrand(AP4_PIFF_BRAND)) {
            compatible_brands.Append(AP4_PIFF_BRAND);
        }

        // create a replacement
        AP4_FtypAtom* new_ftyp = new AP4_FtypAtom(ftyp->GetMajorBrand(),
                                                  ftyp->GetMinorVersion(),
                                                  &compatible_brands[0],
                                                  compatible_brands.ItemCount());
        delete ftyp;
        ftyp = new_ftyp;
    } else {
        AP4_UI32 piff = AP4_PIFF_BRAND;
        ftyp = new AP4_FtypAtom(AP4_FTYP_BRAND_ISOM, 0, &piff, 1);
    }
    
    // insert the ftyp atom as the first child
    return top_level.AddChild(ftyp, 0);
}

/*----------------------------------------------------------------------
|   AP4_PiffEncryptingProcessor:CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_PiffEncryptingProcessor::CreateTrackHandler(AP4_TrakAtom* trak)
{
    // find the stsd atom
    AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));

    // avoid tracks with no stsd atom (should not happen)
    if (stsd == NULL) return NULL;

    // only look at the first sample description
    AP4_SampleEntry* entry = stsd->GetSampleEntry(0);
    if (entry == NULL) return NULL;
        
    // create a handler for this track if we have a key for it and we know
    // how to map the type
    const AP4_DataBuffer* key;
    const AP4_DataBuffer* iv;
    if (AP4_FAILED(m_KeyMap.GetKeyAndIv(trak->GetId(), key, iv))) {
        return NULL;
    }
    
    AP4_UI32 format = 0;
    switch (entry->GetType()) {
        case AP4_ATOM_TYPE_MP4A:
            format = AP4_ATOM_TYPE_ENCA;
            break;

        case AP4_ATOM_TYPE_MP4V:
        case AP4_ATOM_TYPE_AVC1:
            format = AP4_ATOM_TYPE_ENCV;
            break;
            
        default: {
            // try to find if this is audio or video
            AP4_HdlrAtom* hdlr = AP4_DYNAMIC_CAST(AP4_HdlrAtom, trak->FindChild("mdia/hdlr"));
            if (hdlr) {
                switch (hdlr->GetHandlerType()) {
                    case AP4_HANDLER_TYPE_SOUN:
                        format = AP4_ATOM_TYPE_ENCA;
                        break;

                    case AP4_HANDLER_TYPE_VIDE:
                        format = AP4_ATOM_TYPE_ENCV;
                        break;
                }
            }
            break;
        }
    }
    if (format == 0) return NULL;
         
    // get the track properties
    AP4_UI08 kid[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    const char* kid_hex = m_PropertyMap.GetProperty(trak->GetId(), "KID");
    if (kid_hex && AP4_StringLength(kid_hex) == 32) {
        AP4_ParseHex(kid_hex, kid, 16);
    }
        
    // create the encrypter
    AP4_Processor::TrackHandler* track_encrypter;
    track_encrypter = new AP4_PiffTrackEncrypter(m_CipherMode == AP4_PIFF_CIPHER_MODE_CBC?AP4_PIFF_ALGORITHM_ID_CBC:AP4_PIFF_ALGORITHM_ID_CTR, 
                                                 16,
                                                 kid,
                                                 entry, 
                                                 format);
        
    // create a block cipher
    AP4_BlockCipher*            block_cipher = NULL;
    AP4_BlockCipher::CipherMode mode;
    AP4_BlockCipher::CtrParams  ctr_params;
    const void*                 mode_params = NULL;
    switch (m_CipherMode) {
        case AP4_PIFF_CIPHER_MODE_CBC:
            mode = AP4_BlockCipher::CBC;
            break;
            
        case AP4_PIFF_CIPHER_MODE_CTR:
            mode = AP4_BlockCipher::CTR;
            ctr_params.counter_size = 8;
            mode_params = &ctr_params;
            break;
            
        default: return NULL;
    }
    AP4_Result result = m_BlockCipherFactory->CreateCipher(AP4_BlockCipher::AES_128, 
                                                           AP4_BlockCipher::ENCRYPT, 
                                                           mode,
                                                           mode_params,
                                                           key->GetData(), 
                                                           key->GetDataSize(), 
                                                           block_cipher);
    if (AP4_FAILED(result)) return NULL;
    
    // add a new cipher state for this track
    AP4_PiffSampleEncrypter* sample_encrypter = NULL;
    switch (m_CipherMode) {
        case AP4_PIFF_CIPHER_MODE_CBC:
            if (entry->GetType() == AP4_ATOM_TYPE_AVC1) {
                AP4_AvccAtom* avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, entry->GetChild(AP4_ATOM_TYPE_AVCC));
                if (avcc == NULL) return NULL;
                sample_encrypter = new AP4_PiffSubSampleCbcSampleEncrypter(block_cipher, avcc->GetNaluLengthSize());
            } else {
                sample_encrypter = new AP4_PiffCbcSampleEncrypter(block_cipher);
            }
            break;
            
        case AP4_PIFF_CIPHER_MODE_CTR:
            if (entry->GetType() == AP4_ATOM_TYPE_AVC1) {
                AP4_AvccAtom* avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, entry->GetChild(AP4_ATOM_TYPE_AVCC));
                if (avcc == NULL) return NULL;
                sample_encrypter = new AP4_PiffSubSampleCtrSampleEncrypter(block_cipher, avcc->GetNaluLengthSize());
            } else {
                sample_encrypter = new AP4_PiffCtrSampleEncrypter(block_cipher);
            }
            break;
    }
    sample_encrypter->SetIv(iv->GetData());
    m_Encrypters.Add(new Encrypter(trak->GetId(), sample_encrypter));

    return track_encrypter;
}

/*----------------------------------------------------------------------
|   AP4_PiffEncryptingProcessor:CreateFragmentHandler
+---------------------------------------------------------------------*/
AP4_Processor::FragmentHandler* 
AP4_PiffEncryptingProcessor::CreateFragmentHandler(AP4_ContainerAtom* traf)
{
    // get the traf header (tfhd) for this track fragment so we can get the track ID
    AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
    if (tfhd == NULL) return NULL;
    
    // lookup the encrypter for this track
    Encrypter* encrypter = NULL;
    for (AP4_List<Encrypter>::Item* item = m_Encrypters.FirstItem();
                                    item;
                                    item = item->GetNext()) {
        if (item->GetData()->m_TrackId == tfhd->GetTrackId()) {
            encrypter = item->GetData();
            break;
        }
    }
    if (encrypter == NULL) return NULL;
    return new AP4_PiffFragmentEncrypter(traf, encrypter);
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSampleDecrypter::Create(AP4_ProtectedSampleDescription* sample_description, 
                                AP4_ContainerAtom*              traf,
                                const AP4_UI08*                 key, 
                                AP4_Size                        key_size,
                                AP4_BlockCipherFactory*         block_cipher_factory,
                                AP4_PiffSampleDecrypter*&       decrypter)
{
    // check the parameters
    if (key == NULL) {
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    // use the default cipher factory if  none was passed
    if (block_cipher_factory == NULL) block_cipher_factory = &AP4_DefaultBlockCipherFactory::Instance;
    
    // default return value
    decrypter = NULL;
    
    // check the scheme
    if (sample_description->GetSchemeType() != AP4_PROTECTION_SCHEME_TYPE_PIFF) {
        return AP4_ERROR_INVALID_PARAMETERS;
    }
    if (sample_description->GetSchemeVersion() != AP4_PROTECTION_SCHEME_VERSION_PIFF_10 &&
        sample_description->GetSchemeVersion() != AP4_PROTECTION_SCHEME_VERSION_PIFF_11) {
        return AP4_ERROR_NOT_SUPPORTED;
    }
    
    // get the scheme info atom
    AP4_ContainerAtom* schi = sample_description->GetSchemeInfo()->GetSchiAtom();
    if (schi == NULL) return AP4_ERROR_INVALID_FORMAT;

    // look for a track encryption atom
    AP4_PiffTrackEncryptionAtom* track_encryption_atom = 
        AP4_DYNAMIC_CAST(AP4_PiffTrackEncryptionAtom, schi->GetChild(AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM));
    
    // look for a sample encryption atom
    AP4_PiffSampleEncryptionAtom* sample_encryption_atom = NULL;
    if (traf) {
        sample_encryption_atom = AP4_DYNAMIC_CAST(AP4_PiffSampleEncryptionAtom, traf->GetChild(AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM));
        if (sample_encryption_atom == NULL) return AP4_ERROR_INVALID_FORMAT;
    }

    // create the block cipher needed to decrypt the samples
    AP4_UI32     algorithm_id;
    unsigned int iv_size;
    if (sample_encryption_atom &&
        sample_encryption_atom->GetFlags() & AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        algorithm_id = sample_encryption_atom->GetAlgorithmId();
        iv_size      = sample_encryption_atom->GetIvSize();
    } else {
        if (track_encryption_atom == NULL) return AP4_ERROR_INVALID_FORMAT;
        algorithm_id = track_encryption_atom->GetDefaultAlgorithmId();
        iv_size      = track_encryption_atom->GetDefaultIvSize();
    }
    switch (algorithm_id) {
        case AP4_PIFF_ALGORITHM_ID_NONE:
            decrypter = new AP4_PiffNullSampleDecrypter();
            break;
            
        case AP4_PIFF_ALGORITHM_ID_CTR:
            if (iv_size != 8 && iv_size != 16) {
                return AP4_ERROR_INVALID_FORMAT;
            } else {
                // create the block cipher
                AP4_BlockCipher* block_cipher = NULL;
                AP4_BlockCipher::CtrParams ctr_params;
                ctr_params.counter_size = iv_size;
                AP4_Result result = block_cipher_factory->CreateCipher(AP4_BlockCipher::AES_128, 
                                                                       AP4_BlockCipher::DECRYPT, 
                                                                       AP4_BlockCipher::CTR,
                                                                       &ctr_params,
                                                                       key, 
                                                                       key_size, 
                                                                       block_cipher);
                if (AP4_FAILED(result)) return result;
                
                // create the decrypter
                if (sample_description->GetOriginalFormat() == AP4_ATOM_TYPE_AVC1) {
                    AP4_AvcSampleDescription* avc_desc = 
                        AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, 
                                         sample_description->GetOriginalSampleDescription());
                    if (avc_desc == NULL) {
                        return AP4_ERROR_INVALID_FORMAT;
                    }
                    decrypter = new AP4_PiffSubSampleCtrSampleDecrypter(block_cipher, 
                                                                        iv_size,
                                                                        sample_encryption_atom,
                                                                        avc_desc->GetNaluLengthSize());
                    
                } else {
                    decrypter = new AP4_PiffCtrSampleDecrypter(block_cipher, 
                                                               iv_size,
                                                               sample_encryption_atom);
                }
            }
            break;
            
        case AP4_PIFF_ALGORITHM_ID_CBC:
            if (iv_size != 16) {
                return AP4_ERROR_INVALID_FORMAT;
            } else {
                // create the block cipher
                AP4_BlockCipher* block_cipher = NULL;
                AP4_Result result = block_cipher_factory->CreateCipher(AP4_BlockCipher::AES_128, 
                                                                       AP4_BlockCipher::DECRYPT, 
                                                                       AP4_BlockCipher::CBC,
                                                                       NULL,
                                                                       key, 
                                                                       key_size, 
                                                                       block_cipher);
                if (AP4_FAILED(result)) return result;

                // create the decrypter
                if (sample_description->GetOriginalFormat() == AP4_ATOM_TYPE_AVC1) {
                    AP4_AvcSampleDescription* avc_desc = 
                        AP4_DYNAMIC_CAST(AP4_AvcSampleDescription, 
                                         sample_description->GetOriginalSampleDescription());
                    if (avc_desc == NULL) {
                        return AP4_ERROR_INVALID_FORMAT;
                    }
                    decrypter = new AP4_PiffSubSampleCbcSampleDecrypter(block_cipher, 
                                                                        sample_encryption_atom,
                                                                        avc_desc->GetNaluLengthSize());
                } else {
                    decrypter = new AP4_PiffCbcSampleDecrypter(block_cipher, 
                                                               sample_encryption_atom);
                }
            }
            break;
            
        default:
            return AP4_ERROR_NOT_SUPPORTED;
    }

    // setup the sample info walker
    if (sample_encryption_atom) sample_encryption_atom->GetSampleInfoWalker(iv_size, decrypter->m_SampleInfoWalker);    

    return AP4_SUCCESS;
}
       
/*----------------------------------------------------------------------
|   AP4_PiffSampleDecrypter::SetSampleIndex
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSampleDecrypter::SetSampleIndex(AP4_Ordinal sample_index)
{
    return m_SampleInfoWalker.SetSampleIndex(sample_index);
}

/*----------------------------------------------------------------------
|   AP4_PiffCtrSampleDecrypter::AP4_PiffCtrSampleDecrypter
+---------------------------------------------------------------------*/
AP4_PiffCtrSampleDecrypter::AP4_PiffCtrSampleDecrypter(
    AP4_BlockCipher*              block_cipher,
    AP4_Size                      iv_size,
    AP4_PiffSampleEncryptionAtom* sample_encryption_atom) :
    AP4_PiffSampleDecrypter(sample_encryption_atom),
    m_IvSize(iv_size)
{
    m_Cipher = new AP4_CtrStreamCipher(block_cipher, iv_size);
}

/*----------------------------------------------------------------------
|   AP4_PiffCtrSampleDecrypter::~AP4_PiffCtrSampleDecrypter
+---------------------------------------------------------------------*/
AP4_PiffCtrSampleDecrypter::~AP4_PiffCtrSampleDecrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_PiffCtrSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffCtrSampleDecrypter::DecryptSampleData(AP4_DataBuffer& data_in,
                                              AP4_DataBuffer& data_out,
                                              const AP4_UI08* iv)
{
    // setup the IV
    unsigned char iv_block[16];
    if (iv == NULL) {
        AP4_Result result = m_SampleInfoWalker.Next(iv_block);
        if (AP4_FAILED(result)) return result;
    } else {
        AP4_CopyMemory(iv_block, iv, m_IvSize);
        if (m_IvSize != 16) AP4_SetMemory(&iv_block[m_IvSize], 0, 16-m_IvSize);
    }
    m_Cipher->SetIV(iv_block);

    data_out.SetDataSize(data_in.GetDataSize());
    AP4_Size data_out_size = data_out.GetDataSize();
    return m_Cipher->ProcessBuffer(data_in.GetData(), 
                                   data_in.GetDataSize(), 
                                   data_out.UseData(), 
                                   &data_out_size, 
                                   true);
}

/*----------------------------------------------------------------------
|   AP4_PiffSubSampleCtrSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSubSampleCtrSampleDecrypter::DecryptSampleData(AP4_DataBuffer& data_in,
                                                       AP4_DataBuffer& data_out,
                                                       const AP4_UI08* iv)
{
    // the output has the same size as the input
    data_out.SetDataSize(data_in.GetDataSize());

    // setup direct pointers to the buffers
    const AP4_UI08* in  = data_in.GetData();
    AP4_UI08*       out = data_out.UseData();

    // setup the IV
    AP4_PiffSubSampleInfoWalker walker;
    unsigned char iv_block[16];
    AP4_Result result;
    if (m_SampleEncryptionAtom->GetFlags() & AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
        result = m_SampleInfoWalker.Next(iv_block, walker);
    } else {
        result = m_SampleInfoWalker.Next(iv_block);
    }
    if (AP4_FAILED(result)) return result;
    if (iv) {
        AP4_CopyMemory(iv_block, iv, m_IvSize);
        if (m_IvSize != 16) AP4_SetMemory(&iv_block[m_IvSize], 0, 16-m_IvSize);
    }
    m_Cipher->SetIV(iv_block);

    if (m_SampleEncryptionAtom->GetFlags() & AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
        // process the sample data, one sub-sample at a time
        const AP4_UI08* in_end = data_in.GetData()+data_in.GetDataSize();
        unsigned int cleartext_size, encrypted_size;
        while (AP4_SUCCEEDED(walker.Next(cleartext_size, encrypted_size)) &&
               (unsigned int)(in_end-in) >= cleartext_size+encrypted_size) {
            // copy the cleartext portion
            if (cleartext_size) {
                AP4_CopyMemory(out, in, cleartext_size);
            }
            
            // decrypt the rest
            if (encrypted_size) {
                m_Cipher->ProcessBuffer(in+cleartext_size, encrypted_size, out+cleartext_size, &encrypted_size, false);
            }
            
            // move the pointers
            in  += cleartext_size+encrypted_size;
            out += cleartext_size+encrypted_size;
        }
    } else {
        // process the sample data, one NALU at a time
        const AP4_UI08* in_end = data_in.GetData()+data_in.GetDataSize();
        while ((AP4_Size)(in_end-in) > 1+m_NaluLengthSize) {
            unsigned int nalu_length;
            switch (m_NaluLengthSize) {
                case 1:
                    nalu_length = *in;
                    break;
                    
                case 2:
                    nalu_length = AP4_BytesToUInt16BE(in);
                    break;
                    
                case 4:
                    nalu_length = AP4_BytesToUInt32BE(in);
                    break;
                    
                default:
                    return AP4_ERROR_INVALID_FORMAT;
            }

            unsigned int chunk_size     = m_NaluLengthSize+nalu_length;
            unsigned int cleartext_size = m_NaluLengthSize+1;
            
            // copy the cleartext portion
            AP4_CopyMemory(out, in, cleartext_size);
            
            // decrypt the rest
            if (chunk_size > cleartext_size) {
                AP4_Size encrypted_size = chunk_size-cleartext_size;
                m_Cipher->ProcessBuffer(in+cleartext_size, encrypted_size, out+cleartext_size, &encrypted_size, false);
            }
            
            // move the pointers
            in += chunk_size;
            out += chunk_size;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffCbcSampleDecrypter::AP4_PiffCbcSampleDecrypter
+---------------------------------------------------------------------*/
AP4_PiffCbcSampleDecrypter::AP4_PiffCbcSampleDecrypter(
    AP4_BlockCipher*              block_cipher,
    AP4_PiffSampleEncryptionAtom* sample_encryption_atom) :
    AP4_PiffSampleDecrypter(sample_encryption_atom)
{
    m_Cipher = new AP4_CbcStreamCipher(block_cipher);
}

/*----------------------------------------------------------------------
|   AP4_PiffCbcSampleDecrypter::~AP4_PiffCbcSampleDecrypter
+---------------------------------------------------------------------*/
AP4_PiffCbcSampleDecrypter::~AP4_PiffCbcSampleDecrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_PiffCbcSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffCbcSampleDecrypter::DecryptSampleData(AP4_DataBuffer& data_in,
                                              AP4_DataBuffer& data_out,
                                              const AP4_UI08* iv)
{
    // the output has the same size as the input
    data_out.SetDataSize(data_in.GetDataSize());

    // setup direct pointers to the buffers
    const AP4_UI08* in  = data_in.GetData();
    AP4_UI08*       out = data_out.UseData();

    // setup the IV
    unsigned char iv_block[16];
    if (iv == NULL) {
        AP4_Result result = m_SampleInfoWalker.Next(iv_block);
        if (AP4_FAILED(result)) return result;
        iv = iv_block;
    }
    m_Cipher->SetIV(iv);
    
    // process the sample data
    unsigned int block_count = data_in.GetDataSize()/16;
    if (block_count) {
        AP4_Size out_size = data_out.GetDataSize();
        AP4_Result result = m_Cipher->ProcessBuffer(in, block_count*16, out, &out_size, false);
        if (AP4_FAILED(result)) return result;
        AP4_ASSERT(out_size == block_count*16);
        in  += block_count*16;
        out += block_count*16;
    }
    
    // any partial block at the end remains in the clear
    unsigned int partial = data_in.GetDataSize()%16;
    if (partial) {
        AP4_CopyMemory(out, in, partial);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSubSampleCbcSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSubSampleCbcSampleDecrypter::DecryptSampleData(AP4_DataBuffer& data_in,
                                                       AP4_DataBuffer& data_out,
                                                       const AP4_UI08* iv)
{
    // the output has the same size as the input
    data_out.SetDataSize(data_in.GetDataSize());

    // setup direct pointers to the buffers
    const AP4_UI08* in  = data_in.GetData();
    AP4_UI08*       out = data_out.UseData();

    // setup the IV
    unsigned char iv_block[16];
    if (iv == NULL) {
        AP4_Result result = m_SampleInfoWalker.Next(iv_block);
        if (AP4_FAILED(result)) return result;
        iv = iv_block;
    }
    m_Cipher->SetIV(iv);

    // process the sample data, one NALU at a time
    const AP4_UI08* in_end = data_in.GetData()+data_in.GetDataSize();
    while ((AP4_Size)(in_end-in) > 1+m_NaluLengthSize) {
        unsigned int nalu_length;
        switch (m_NaluLengthSize) {
            case 1:
                nalu_length = *in;
                break;
                
            case 2:
                nalu_length = AP4_BytesToUInt16BE(in);
                break;
                
            case 4:
                nalu_length = AP4_BytesToUInt32BE(in);
                break;
                
            default:
                return AP4_ERROR_INVALID_FORMAT;
        }

        unsigned int chunk_size     = m_NaluLengthSize+nalu_length;
        unsigned int cleartext_size = chunk_size%16;
        unsigned int block_count    = chunk_size/16;
        if (cleartext_size < m_NaluLengthSize+1) {
            AP4_ASSERT(block_count);
            --block_count;
            cleartext_size += 16;
        }
        
        // copy the cleartext portion
        AP4_CopyMemory(out, in, cleartext_size);
        
        // encrypt the rest
        if (block_count) {
            AP4_Size out_size = block_count*16;
            m_Cipher->ProcessBuffer(in+cleartext_size, block_count*16, out+cleartext_size, &out_size, false);
        }
        
        // move the pointers
        in += chunk_size;
        out += chunk_size;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackDecrypter
+---------------------------------------------------------------------*/
class AP4_PiffTrackDecrypter : public AP4_Processor::TrackHandler {
public:
    AP4_IMPLEMENT_DYNAMIC_CAST(AP4_PiffTrackDecrypter)

    // constructor
    static AP4_Result Create(const unsigned char*            key,
                             AP4_Size                        key_size,
                             AP4_ProtectedSampleDescription* sample_description,
                             AP4_SampleEntry*                sample_entry,
                             AP4_PiffTrackDecrypter*&        decrypter);

    // methods
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);
    virtual AP4_Result ProcessTrack();

    // accessors
    AP4_ProtectedSampleDescription* GetSampleDescription() {
        return m_SampleDescription;
    }
    
private:
    // constructor
    AP4_PiffTrackDecrypter(AP4_ProtectedSampleDescription* sample_description,
                           AP4_SampleEntry*                sample_entry,
                           AP4_UI32                        original_format);

    // members
    AP4_ProtectedSampleDescription* m_SampleDescription;
    AP4_SampleEntry*                m_SampleEntry;
    AP4_UI32                        m_OriginalFormat;
};

/*----------------------------------------------------------------------
|   AP4_PiffTrackDecrypter Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PiffTrackDecrypter)

/*----------------------------------------------------------------------
|   AP4_PiffTrackDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_PiffTrackDecrypter::Create(const unsigned char*            key,
                               AP4_Size                        /* key_size */,
                               AP4_ProtectedSampleDescription* sample_description,
                               AP4_SampleEntry*                sample_entry,
                               AP4_PiffTrackDecrypter*&        decrypter)
{
    decrypter = NULL;

    // check and set defaults
    if (key == NULL) {
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    // instantiate the object
    decrypter = new AP4_PiffTrackDecrypter(sample_description,
                                           sample_entry, 
                                           sample_description->GetOriginalFormat());
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackDecrypter::AP4_PiffTrackDecrypter
+---------------------------------------------------------------------*/
AP4_PiffTrackDecrypter::AP4_PiffTrackDecrypter(AP4_ProtectedSampleDescription* sample_description,
                                               AP4_SampleEntry*                sample_entry,
                                               AP4_UI32                        original_format) :
    m_SampleDescription(sample_description),
    m_SampleEntry(sample_entry),
    m_OriginalFormat(original_format)
{
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackDecrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffTrackDecrypter::ProcessSample(AP4_DataBuffer& data_in,
                                      AP4_DataBuffer& data_out)
{
    data_out.SetData(data_in.GetData(), data_in.GetDataSize());
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackDecrypter::ProcessTrack
+---------------------------------------------------------------------*/
AP4_Result   
AP4_PiffTrackDecrypter::ProcessTrack()
{
    m_SampleEntry->SetType(m_OriginalFormat);
    m_SampleEntry->DeleteChild(AP4_ATOM_TYPE_SINF);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffFragmentDecrypter
+---------------------------------------------------------------------*/
class AP4_PiffFragmentDecrypter : public AP4_Processor::FragmentHandler {
public:
    // constructor
    AP4_PiffFragmentDecrypter(AP4_PiffSampleDecrypter* sample_decrypter);

    // methods
    virtual AP4_Result ProcessFragment();
    virtual AP4_Result FinishFragment();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);

private:
    // members
    AP4_PiffSampleDecrypter* m_SampleDecrypter;
    AP4_Ordinal              m_IvIndex;
};

/*----------------------------------------------------------------------
|   AP4_PiffFragmentDecrypter::AP4_PiffFragmentDecrypter
+---------------------------------------------------------------------*/
AP4_PiffFragmentDecrypter::AP4_PiffFragmentDecrypter(AP4_PiffSampleDecrypter* sample_decrypter) :
    m_SampleDecrypter(sample_decrypter),
    m_IvIndex(0)
{
}

/*----------------------------------------------------------------------
|   AP4_PiffFragmentDecrypter::ProcessFragment
+---------------------------------------------------------------------*/
AP4_Result   
AP4_PiffFragmentDecrypter::ProcessFragment()
{
    // detach the sample encryption atom
    if (m_SampleDecrypter) {
        AP4_PiffSampleEncryptionAtom* atom = m_SampleDecrypter->GetSampleEncryptionAtom();
        atom->Detach();
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffFragmentDecrypter::FinishFragment
+---------------------------------------------------------------------*/
AP4_Result   
AP4_PiffFragmentDecrypter::FinishFragment()
{
    AP4_PiffSampleEncryptionAtom* atom = m_SampleDecrypter->GetSampleEncryptionAtom();
    m_SampleDecrypter->SetSampleEncryptionAtom(NULL);
    delete atom;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffFragmentDecrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffFragmentDecrypter::ProcessSample(AP4_DataBuffer& data_in,
                                         AP4_DataBuffer& data_out)
{
    // decrypt the sample
    return m_SampleDecrypter->DecryptSampleData(data_in, data_out, NULL);
}

/*----------------------------------------------------------------------
|   AP4_PiffDecryptingProcessor::AP4_PiffDecryptingProcessor
+---------------------------------------------------------------------*/
AP4_PiffDecryptingProcessor::AP4_PiffDecryptingProcessor(const AP4_ProtectionKeyMap* key_map, 
                                                         AP4_BlockCipherFactory*     block_cipher_factory) :
    m_KeyMap(key_map)
{
    if (block_cipher_factory) {
        m_BlockCipherFactory = block_cipher_factory;
    } else {
        m_BlockCipherFactory = &AP4_DefaultBlockCipherFactory::Instance;
    }
}

/*----------------------------------------------------------------------
|   AP4_PiffDecryptingProcessor::~AP4_PiffDecryptingProcessor
+---------------------------------------------------------------------*/
AP4_PiffDecryptingProcessor::~AP4_PiffDecryptingProcessor()
{
}

/*----------------------------------------------------------------------
|   AP4_PiffDecryptingProcessor::Initialize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffDecryptingProcessor::Initialize(AP4_AtomParent&                  /*top_level*/,
                                        AP4_ByteStream&                  /*stream*/,
                                        AP4_Processor::ProgressListener* /*listener*/)
{
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffDecryptingProcessor:CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_PiffDecryptingProcessor::CreateTrackHandler(AP4_TrakAtom* trak)
{
    // find the stsd atom
    AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));

    // avoid tracks with no stsd atom (should not happen)
    if (stsd == NULL) return NULL;

    // we only look at the first sample description
    AP4_SampleDescription* desc = stsd->GetSampleDescription(0);
    AP4_SampleEntry* entry = stsd->GetSampleEntry(0);
    if (desc == NULL || entry == NULL) return NULL;
    if (m_KeyMap == NULL) return NULL;
    if (desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
        // create a handler for this track
        AP4_ProtectedSampleDescription* protected_desc = 
            static_cast<AP4_ProtectedSampleDescription*>(desc);
        if (protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_PIFF) {
            const AP4_DataBuffer* key = m_KeyMap->GetKey(trak->GetId());
            if (key) {
                AP4_PiffTrackDecrypter* handler = NULL;
                AP4_Result result = AP4_PiffTrackDecrypter::Create(key->GetData(), 
                                                                   key->GetDataSize(), 
                                                                   protected_desc, 
                                                                   entry, 
                                                                   handler);
                if (AP4_FAILED(result)) return NULL;
                return handler;
            }
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_PiffDecryptingProcessor::CreateFragmentHandler
+---------------------------------------------------------------------*/
AP4_Processor::FragmentHandler* 
AP4_PiffDecryptingProcessor::CreateFragmentHandler(AP4_ContainerAtom* traf)
{
    // find the matching track handler to get to the track sample description
    const AP4_DataBuffer* key = NULL;
    AP4_ProtectedSampleDescription* sample_description = NULL;
    for (unsigned int i=0; i<m_TrackIds.ItemCount(); i++) {
        AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
        if (tfhd && m_TrackIds[i] == tfhd->GetTrackId()) {
            // look for the Track Encryption Box
            AP4_PiffTrackDecrypter* track_decrypter = 
                AP4_DYNAMIC_CAST(AP4_PiffTrackDecrypter, m_TrackHandlers[i]);
            if (track_decrypter) {
                sample_description = track_decrypter->GetSampleDescription();
            }
            
            // get the matching key
            key = m_KeyMap->GetKey(tfhd->GetTrackId());
        }
    }

    // create the sample decrypter for the fragment
    AP4_PiffSampleDecrypter* sample_decrypter = NULL;
    AP4_Result result = AP4_PiffSampleDecrypter::Create(
        sample_description, 
        traf, 
        key->GetData(), 
        key->GetDataSize(), 
        NULL, 
        sample_decrypter);
    if (AP4_FAILED(result)) return NULL;
    
    return new AP4_PiffFragmentDecrypter(sample_decrypter);
}
    
/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PiffTrackEncryptionAtom)

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::Create
+---------------------------------------------------------------------*/
AP4_PiffTrackEncryptionAtom* 
AP4_PiffTrackEncryptionAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI32 version = 0;
    AP4_UI32 flags   = 0;
    AP4_Result result = ReadFullHeader(stream, version, flags);
    if (AP4_FAILED(result)) return NULL;
    if (version != 0) return NULL;
    return new AP4_PiffTrackEncryptionAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::AP4_PiffTrackEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffTrackEncryptionAtom::AP4_PiffTrackEncryptionAtom(AP4_UI32        size, 
                                                         AP4_UI32        version,
                                                         AP4_UI32        flags,
                                                         AP4_ByteStream& stream) :
    AP4_UuidAtom(size, AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM, version, flags),
    m_DefaultAlgorithmId(0),
    m_DefaultIvSize(0)
{
    stream.ReadUI24(m_DefaultAlgorithmId);
    stream.ReadUI08(m_DefaultIvSize);
    AP4_SetMemory(m_DefaultKid, 0, 16);
    stream.Read(m_DefaultKid, 16);
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::AP4_PiffTrackEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffTrackEncryptionAtom::AP4_PiffTrackEncryptionAtom(AP4_UI32        default_algorithm_id,
                                                         AP4_UI08        default_iv_size,
                                                         const AP4_UI08* default_kid) :
    AP4_UuidAtom(AP4_FULL_UUID_ATOM_HEADER_SIZE+20, AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM, 0, 0),
    m_DefaultAlgorithmId(default_algorithm_id),
    m_DefaultIvSize(default_iv_size)
{
    AP4_CopyMemory(m_DefaultKid, default_kid, 16);
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffTrackEncryptionAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("default_AlgorithmID", m_DefaultAlgorithmId);
    inspector.AddField("default_IV_size",     m_DefaultIvSize);
    inspector.AddField("default_KID",         m_DefaultKid, 16);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffTrackEncryptionAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // write the fields   
    result = stream.WriteUI24(m_DefaultAlgorithmId);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(m_DefaultIvSize);
    if (AP4_FAILED(result)) return result;
    result = stream.Write(m_DefaultKid, 16);
    if (AP4_FAILED(result)) return result;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PiffSampleEncryptionAtom)

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::Create
+---------------------------------------------------------------------*/
AP4_PiffSampleEncryptionAtom* 
AP4_PiffSampleEncryptionAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI32 version = 0;
    AP4_UI32 flags   = 0;
    AP4_Result result = ReadFullHeader(stream, version, flags);
    if (AP4_FAILED(result)) return NULL;
    if (version != 0) return NULL;
    return new AP4_PiffSampleEncryptionAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom(AP4_UI32        size, 
                                                           AP4_UI32        version,
                                                           AP4_UI32        flags,
                                                           AP4_ByteStream& stream) :
    AP4_UuidAtom(size, AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM, version, flags)
{
    if (flags & AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        stream.ReadUI24(m_AlgorithmId);
        stream.ReadUI08(m_IvSize);
        stream.Read    (m_Kid, 16);
    } else {
        m_AlgorithmId = 0;
        m_IvSize      = 0;
        AP4_SetMemory(m_Kid, 0, 16);
    }
    
    stream.ReadUI32(m_SampleInfoCount);

    AP4_Size payload_size = size-GetHeaderSize()-4;
    m_SampleInfos.SetDataSize(payload_size);
    stream.Read(m_SampleInfos.UseData(), payload_size);
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom(AP4_Cardinal sample_count) :
    AP4_UuidAtom(AP4_FULL_UUID_ATOM_HEADER_SIZE+4+sample_count*16, AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM, 0, 0),
    m_AlgorithmId(0),
    m_IvSize(16),
    m_SampleInfoCount(sample_count)
{
    AP4_SetMemory(m_Kid, 0, 16);

    // initialize the IVs to 0s
    m_SampleInfos.SetDataSize(sample_count*m_IvSize);
    AP4_SetMemory(m_SampleInfos.UseData(), 0, m_SampleInfos.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom(AP4_UI32        algorithm_id,
                                                           AP4_UI08        iv_size,
                                                           const AP4_UI08* kid,
                                                           AP4_Cardinal    sample_count) :
    AP4_UuidAtom(AP4_FULL_UUID_ATOM_HEADER_SIZE+20+4, AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM, 0, 
                 AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS),
    m_AlgorithmId(algorithm_id),
    m_IvSize(iv_size),
    m_SampleInfoCount(sample_count)
{
    AP4_CopyMemory(m_Kid, kid, 16);

    // initialize the IVs to 0s
    m_SampleInfos.SetDataSize(sample_count*m_IvSize);
    AP4_SetMemory(m_SampleInfos.UseData(), 0, m_SampleInfos.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::SetIv
+---------------------------------------------------------------------*/
AP4_Result      
AP4_PiffSampleEncryptionAtom::SetIv(AP4_Ordinal indx, const AP4_UI08* iv)
{
    if (m_IvSize == 0) return AP4_ERROR_INVALID_STATE;
    if (indx >= m_SampleInfoCount) {
        return AP4_ERROR_OUT_OF_RANGE;
    }
    AP4_CopyMemory(m_SampleInfos.UseData()+indx*m_IvSize, iv, m_IvSize);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSubSampleInfoWalker::Next
+---------------------------------------------------------------------*/
AP4_Result
AP4_PiffSubSampleInfoWalker::Next(unsigned int& cleartext_size, 
                                  unsigned int& encrypted_size)
{   
    cleartext_size = 0;
    encrypted_size = 0;
    if (m_SubSampleIndex >= m_EntryCount) return AP4_ERROR_OUT_OF_RANGE;
    cleartext_size = AP4_BytesToUInt16BE(m_Cursor);
    encrypted_size = AP4_BytesToUInt32BE(m_Cursor+2);
    m_Cursor += 6;
    ++m_SubSampleIndex;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleInfoWalker::Next
+---------------------------------------------------------------------*/
AP4_Result
AP4_PiffSampleInfoWalker::Next(unsigned char* iv)
{
    if (m_Data == NULL || m_IvSize == 0) return AP4_ERROR_INVALID_STATE;
    if (m_SampleIndex >= m_EntryCount || m_Cursor >= m_Data+m_DataSize) {
        return AP4_ERROR_OUT_OF_RANGE;
    }
    AP4_CopyMemory(iv, m_Cursor, m_IvSize);
    m_Cursor += m_IvSize;
    if (m_IvSize != 16) AP4_SetMemory(iv+m_IvSize, 0, 16-m_IvSize);
    if (m_HasSubSamples) {
        AP4_UI16 entry_count = AP4_BytesToInt16BE(m_Cursor);
        m_Cursor += 2+entry_count*6;
    }
    m_SampleIndex++;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleInfoWalker::Next
+---------------------------------------------------------------------*/
AP4_Result
AP4_PiffSampleInfoWalker::Next(unsigned char*               iv,
                               AP4_PiffSubSampleInfoWalker& walker)
{
    if (m_Data == NULL || m_IvSize == 0) return AP4_ERROR_INVALID_STATE;
    if (m_SampleIndex >= m_EntryCount || m_Cursor >= m_Data+m_DataSize) {
        return AP4_ERROR_OUT_OF_RANGE;
    }
    AP4_CopyMemory(iv, m_Cursor, m_IvSize);
    m_Cursor += m_IvSize;
    if (m_IvSize != 16) AP4_SetMemory(iv+m_IvSize, 0, 16-m_IvSize);
    if (m_HasSubSamples) {
        AP4_UI16 entry_count = AP4_BytesToInt16BE(m_Cursor);
        walker.Init(entry_count, m_Cursor+2);
        m_Cursor += 2+entry_count*6;
    }
    m_SampleIndex++;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleInfoWalker::SetSampleIndex
+---------------------------------------------------------------------*/
AP4_Result
AP4_PiffSampleInfoWalker::SetSampleIndex(AP4_Ordinal indx)
{
    if (m_Data == NULL || m_IvSize == 0) return AP4_ERROR_INVALID_STATE;
    if (indx >= m_EntryCount) return AP4_ERROR_OUT_OF_RANGE;
    if (m_HasSubSamples) {
        m_Cursor = m_Data;
        for (unsigned int i=0; i<indx; i++) {
            m_Cursor += m_IvSize;
            AP4_UI16 entry_count = AP4_BytesToInt16BE(m_Cursor);
            m_Cursor += 2+entry_count*6;
        }
    } else {
        m_Cursor = m_Data+m_IvSize*indx;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::GetSampleInfoWalker
+---------------------------------------------------------------------*/
AP4_Result
AP4_PiffSampleEncryptionAtom::GetSampleInfoWalker(AP4_Size                  default_iv_size,
                                                  AP4_PiffSampleInfoWalker& walker)
{
    AP4_Size iv_size = default_iv_size;
    if (m_Flags & AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        iv_size = m_IvSize;
    } 
    bool has_subsamples = false;
    if (m_Flags & AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
        has_subsamples = true;
    }
    walker.Init(m_SampleInfoCount, m_SampleInfos.GetData(), m_SampleInfos.GetDataSize(), iv_size, has_subsamples);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSampleEncryptionAtom::InspectFields(AP4_AtomInspector& inspector)
{
    if (m_Flags & AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        inspector.AddField("AlgorithmID", m_AlgorithmId);
        inspector.AddField("IV_size",     m_IvSize);
        inspector.AddField("KID",         m_Kid, 16);
    }
    
    // NOTE: we can't print the sample infos here because we don't know the IV size
    // (it can only be known in the context of a track)
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSampleEncryptionAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // optional fields
    if (m_Flags & AP4_PIFF_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {   
        result = stream.WriteUI24(m_AlgorithmId);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI08(m_IvSize);
        if (AP4_FAILED(result)) return result;
        result = stream.Write(m_Kid, 16);
        if (AP4_FAILED(result)) return result;
    }
    
    // IVs
    result = stream.WriteUI32(m_SampleInfoCount);
    if (AP4_FAILED(result)) return result;
    if (m_SampleInfos.GetDataSize()) {
        stream.Write(m_SampleInfos.GetData(), m_SampleInfos.GetDataSize());
        if (AP4_FAILED(result)) return result;
    }
    
    return AP4_SUCCESS;
}

