/*****************************************************************
|
|    AP4 - Common Encryption Support
|
|    Copyright 2002-2011 Axiomatic Systems, LLC
|
|
|    This file is part of Bento4/AP4 (MP4  Processing Library).
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
#include "Ap4CommonEncryption.h"
#include "Ap4Utils.h"
#include "Ap4FrmaAtom.h"
#include "Ap4SchmAtom.h"
#include "Ap4TencAtom.h"
#include "Ap4SencAtom.h"
#include "Ap4FragmentSampleTable.h"
#include "Ap4FtypAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4TrakAtom.h"
#include "Ap4HdlrAtom.h"
#include "Ap4TfhdAtom.h"
#include "Ap4TrexAtom.h"
#include "Ap4SaizAtom.h"
#include "Ap4SaioAtom.h"
#include "Ap4Piff.h"
#include "Ap4StreamCipher.h"
#include "Ap4SaioAtom.h"
#include "Ap4SaizAtom.h"
#include "Ap4TrunAtom.h"
#include "Ap4Marlin.h"
#include "Ap4PsshAtom.h"

/*----------------------------------------------------------------------
|   AP4_CencSampleEncrypter::~AP4_CencSampleEncrypter
+---------------------------------------------------------------------*/
AP4_CencSampleEncrypter::~AP4_CencSampleEncrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_CencCtrSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencCtrSampleEncrypter::EncryptSampleData(AP4_DataBuffer& data_in,
                                              AP4_DataBuffer& data_out,
                                              AP4_DataBuffer& /* sample_infos */)
{
    // the output has the same size as the input
    data_out.SetDataSize(data_in.GetDataSize());

    // setup direct pointers to the buffers
    const AP4_UI08* in  = data_in.GetData();
    AP4_UI08*       out = data_out.UseData();
    
    // setup the IV
    m_Cipher->SetIV(m_Iv);

    // process the sample data
    if (data_in.GetDataSize()) {
        AP4_Size out_size = data_out.GetDataSize();
        AP4_Result result = m_Cipher->ProcessBuffer(in, data_in.GetDataSize(), out, &out_size, false);
        if (AP4_FAILED(result)) return result;
    }
    
    // update the IV
    if (m_IvSize == 16) {
        unsigned int block_count = (data_in.GetDataSize()+15)/16;
        AP4_UI64 counter = AP4_BytesToUInt64BE(&m_Iv[8]);
        AP4_BytesFromUInt64BE(&m_Iv[8], counter+block_count);
    } else if (m_IvSize == 8){
        AP4_UI64 counter = AP4_BytesToUInt64BE(&m_Iv[0]);
        AP4_BytesFromUInt64BE(&m_Iv[0], counter+1);
    } else {
        return AP4_ERROR_INTERNAL;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencCtrSubSampleEncrypter::GetSubSampleMap
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencCtrSubSampleEncrypter::GetSubSampleMap(AP4_DataBuffer&      sample_data, 
                                               AP4_Array<AP4_UI16>& bytes_of_cleartext_data, 
                                               AP4_Array<AP4_UI32>& bytes_of_encrypted_data)
{
    // setup direct pointers to the buffers
    const AP4_UI08* in = sample_data.GetData();
    
    // process the sample data, one NALU at a time
    const AP4_UI08* in_end = sample_data.GetData()+sample_data.GetDataSize();
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
        if (in+chunk_size > in_end) {
            return AP4_ERROR_INVALID_FORMAT;
        }
        if (cleartext_size < m_NaluLengthSize+1) {
            AP4_ASSERT(block_count);
            --block_count;
            cleartext_size += 16;
        }
                
        // move the pointers
        in += chunk_size;
        
        // store the info
        bytes_of_cleartext_data.Append(cleartext_size);
        bytes_of_encrypted_data.Append(block_count*16);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencCtrSubSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencCtrSubSampleEncrypter::EncryptSampleData(AP4_DataBuffer& data_in,
                                                 AP4_DataBuffer& data_out,
                                                 AP4_DataBuffer& sample_infos)
{
    // the output has the same size as the input
    data_out.SetDataSize(data_in.GetDataSize());

    // check some basics
    if (data_in.GetDataSize() == 0) return AP4_SUCCESS;

    // setup direct pointers to the buffers
    const AP4_UI08* in  = data_in.GetData();
    AP4_UI08*       out = data_out.UseData();
    
    // setup the IV
    unsigned int total_encrypted = 0;
    m_Cipher->SetIV(m_Iv);

    // get the subsample map
    AP4_Array<AP4_UI16> bytes_of_cleartext_data;
    AP4_Array<AP4_UI32> bytes_of_encrypted_data;
    AP4_Result result = GetSubSampleMap(data_in, bytes_of_cleartext_data, bytes_of_encrypted_data);
    if (AP4_FAILED(result)) return result;

    // process the data
    for (unsigned int i=0; i<bytes_of_cleartext_data.ItemCount(); i++) {
        // copy the cleartext portion
        AP4_CopyMemory(out, in, bytes_of_cleartext_data[i]);
        
        // encrypt the rest
        if (bytes_of_encrypted_data[i]) {
            AP4_Size out_size = bytes_of_encrypted_data[i];
            m_Cipher->ProcessBuffer(in+bytes_of_cleartext_data[i], 
                                    bytes_of_encrypted_data[i], 
                                    out+bytes_of_cleartext_data[i], 
                                    &out_size);
            total_encrypted += bytes_of_encrypted_data[i];
        }
        
        // move the pointers
        in  += bytes_of_cleartext_data[i]+bytes_of_encrypted_data[i];
        out += bytes_of_cleartext_data[i]+bytes_of_encrypted_data[i];
    }
    
    // update the IV
    if (m_IvSize == 16) {
        AP4_UI64 counter = AP4_BytesToUInt64BE(&m_Iv[8]);
        AP4_BytesFromUInt64BE(&m_Iv[8], counter+(total_encrypted+15)/16);
    } else {
        AP4_UI64 counter = AP4_BytesToUInt64BE(&m_Iv[0]);
        AP4_BytesFromUInt64BE(&m_Iv[0], counter+1);
    }
    
    // encode the sample infos
    unsigned int sample_info_count = bytes_of_cleartext_data.ItemCount();
    sample_infos.SetDataSize(2+sample_info_count*6);
    AP4_UI08* infos = sample_infos.UseData();
    AP4_BytesFromUInt16BE(infos, sample_info_count);
    for (unsigned int i=0; i<sample_info_count; i++) {
        AP4_BytesFromUInt16BE(&infos[2+i*6],   bytes_of_cleartext_data[i]);
        AP4_BytesFromUInt32BE(&infos[2+i*6+2], bytes_of_encrypted_data[i]);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencCbcSubSampleEncrypter::GetSubSampleMap
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencCbcSubSampleEncrypter::GetSubSampleMap(AP4_DataBuffer&      sample_data, 
                                               AP4_Array<AP4_UI16>& bytes_of_cleartext_data, 
                                               AP4_Array<AP4_UI32>& bytes_of_encrypted_data)
{
    // setup direct pointers to the buffers
    const AP4_UI08* in = sample_data.GetData();
    
    // process the sample data, one NALU at a time
    const AP4_UI08* in_end = sample_data.GetData()+sample_data.GetDataSize();
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
                
        // move the pointers
        in += chunk_size;
        
        // store the info
        bytes_of_cleartext_data.Append(cleartext_size);
        bytes_of_encrypted_data.Append(block_count*16);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencCbcSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencCbcSampleEncrypter::EncryptSampleData(AP4_DataBuffer& data_in,
                                              AP4_DataBuffer& data_out,
                                              AP4_DataBuffer& /* sample_infos */)
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
|   AP4_CencCbcSubSampleEncrypter::EncryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencCbcSubSampleEncrypter::EncryptSampleData(AP4_DataBuffer& data_in,
                                                 AP4_DataBuffer& data_out,
                                                 AP4_DataBuffer& sample_infos)
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

    // get the subsample map
    AP4_Array<AP4_UI16> bytes_of_cleartext_data;
    AP4_Array<AP4_UI32> bytes_of_encrypted_data;
    AP4_Result result = GetSubSampleMap(data_in, bytes_of_cleartext_data, bytes_of_encrypted_data);
    if (AP4_FAILED(result)) return result;

    for (unsigned int i=0; i<bytes_of_cleartext_data.ItemCount(); i++) {
        // copy the cleartext portion
        AP4_CopyMemory(out, in, bytes_of_cleartext_data[i]);
        
        // encrypt the rest
        if (bytes_of_encrypted_data[i]) {
            AP4_Size out_size = bytes_of_encrypted_data[i];
            m_Cipher->ProcessBuffer(in+bytes_of_cleartext_data[i], 
                                    bytes_of_encrypted_data[i], 
                                    out+bytes_of_cleartext_data[i], 
                                    &out_size, false);
            
            // update the IV (last cipherblock emitted)
            AP4_CopyMemory(m_Iv, out+bytes_of_cleartext_data[i]+bytes_of_encrypted_data[i]-16, 16);            
        }
        
        // move the pointers
        in  += bytes_of_cleartext_data[i]+bytes_of_encrypted_data[i];
        out += bytes_of_cleartext_data[i]+bytes_of_encrypted_data[i];
    }
    
    // encode the sample infos
    unsigned int sample_info_count = bytes_of_cleartext_data.ItemCount();
    sample_infos.SetDataSize(2+sample_info_count*6);
    AP4_UI08* infos = sample_infos.UseData();
    AP4_BytesFromUInt16BE(infos, sample_info_count);
    for (unsigned int i=0; i<sample_info_count; i++) {
        AP4_BytesFromUInt16BE(&infos[2+i*6],   bytes_of_cleartext_data[i]);
        AP4_BytesFromUInt32BE(&infos[2+i*6+2], bytes_of_encrypted_data[i]);
    }
        
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncrypter
+---------------------------------------------------------------------*/
class AP4_CencTrackEncrypter : public AP4_Processor::TrackHandler {
public:
    // constructor
    AP4_CencTrackEncrypter(AP4_CencVariant              variant,
                           AP4_UI32                     default_algorithm_id,
                           AP4_UI08                     default_iv_size,
                           const AP4_UI08*              default_kid,
                           AP4_Array<AP4_SampleEntry*>& sample_entries,
                           AP4_UI32                     format);

    // methods
    virtual AP4_Result ProcessTrack();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);

private:
    // members
    AP4_CencVariant             m_Variant;
    AP4_Array<AP4_SampleEntry*> m_SampleEntries;
    AP4_UI32                    m_Format;
    AP4_UI32                    m_DefaultAlgorithmId;
    AP4_UI08                    m_DefaultIvSize;
    AP4_UI08                    m_DefaultKid[16];
};

/*----------------------------------------------------------------------
|   AP4_CencTrackEncrypter::AP4_CencTrackEncrypter
+---------------------------------------------------------------------*/
AP4_CencTrackEncrypter::AP4_CencTrackEncrypter(
    AP4_CencVariant              variant,
    AP4_UI32                     default_algorithm_id,
    AP4_UI08                     default_iv_size,
    const AP4_UI08*              default_kid,
    AP4_Array<AP4_SampleEntry*>& sample_entries,
    AP4_UI32                     format) :
    m_Variant(variant),
    m_Format(format),
    m_DefaultAlgorithmId(default_algorithm_id),
    m_DefaultIvSize(default_iv_size)
{
    // copy the KID
    AP4_CopyMemory(m_DefaultKid, default_kid, 16);

    // copy the sample entry list
    for (unsigned int i=0; i<sample_entries.ItemCount(); i++) {
        m_SampleEntries.Append(sample_entries[i]);
    }
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncrypter::ProcessTrack
+---------------------------------------------------------------------*/
AP4_Result   
AP4_CencTrackEncrypter::ProcessTrack()
{
    for (unsigned int i=0; i<m_SampleEntries.ItemCount(); i++) {
        // original format
        AP4_FrmaAtom* frma = new AP4_FrmaAtom(m_SampleEntries[i]->GetType());
        
        // scheme info
        AP4_SchmAtom* schm = NULL;
        AP4_Atom*     tenc = NULL;
        switch (m_Variant) {
            case AP4_CENC_VARIANT_PIFF_CBC:
            case AP4_CENC_VARIANT_PIFF_CTR:
                schm = new AP4_SchmAtom(AP4_PROTECTION_SCHEME_TYPE_PIFF, 
                                        AP4_PROTECTION_SCHEME_VERSION_PIFF_11);
                tenc = new AP4_PiffTrackEncryptionAtom(m_DefaultAlgorithmId, 
                                                       m_DefaultIvSize, 
                                                       m_DefaultKid);
                break;
                
            case AP4_CENC_VARIANT_MPEG:
                schm = new AP4_SchmAtom(AP4_PROTECTION_SCHEME_TYPE_CENC, 
                                        AP4_PROTECTION_SCHEME_VERSION_CENC_10);
                tenc = new AP4_TencAtom(m_DefaultAlgorithmId,
                                        m_DefaultIvSize,
                                        m_DefaultKid);
                break;
        }
        
        // populate the schi container
        AP4_ContainerAtom* schi = new AP4_ContainerAtom(AP4_ATOM_TYPE_SCHI);
        schi->AddChild(tenc);
            
        // populate the sinf container
        AP4_ContainerAtom* sinf = new AP4_ContainerAtom(AP4_ATOM_TYPE_SINF);
        sinf->AddChild(frma);
        sinf->AddChild(schm);
        sinf->AddChild(schi);

        // add the sinf atom to the sample description
        m_SampleEntries[i]->AddChild(sinf);

        // change the atom type of the sample description
        m_SampleEntries[i]->SetType(m_Format);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencTrackEncrypter::ProcessSample(AP4_DataBuffer& data_in,
                                      AP4_DataBuffer& data_out)
{
    return data_out.SetData(data_in.GetData(), data_in.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter
+---------------------------------------------------------------------*/
class AP4_CencFragmentEncrypter : public AP4_Processor::FragmentHandler {
public:
    // constructor
    AP4_CencFragmentEncrypter(AP4_CencVariant                         variant,
                              AP4_ContainerAtom*                      traf,
                              AP4_CencEncryptingProcessor::Encrypter* encrypter);

    // methods
    virtual AP4_Result ProcessFragment();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);
    virtual AP4_Result PrepareForSamples(AP4_FragmentSampleTable* sample_table);
    virtual AP4_Result FinishFragment();
    
private:
    // members
    AP4_CencVariant                         m_Variant;
    AP4_ContainerAtom*                      m_Traf;
    AP4_CencSampleEncryption*               m_SampleEncryptionAtom;
    AP4_CencSampleEncryption*               m_SampleEncryptionAtomShadow;
    AP4_SaizAtom*                           m_Saiz;
    AP4_SaioAtom*                           m_Saio;
    AP4_CencEncryptingProcessor::Encrypter* m_Encrypter;
};

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter::AP4_CencFragmentEncrypter
+---------------------------------------------------------------------*/
AP4_CencFragmentEncrypter::AP4_CencFragmentEncrypter(AP4_CencVariant                         variant,
                                                     AP4_ContainerAtom*                      traf,
                                                     AP4_CencEncryptingProcessor::Encrypter* encrypter) :
    m_Variant(variant),
    m_Traf(traf),
    m_SampleEncryptionAtom(NULL),
    m_SampleEncryptionAtomShadow(NULL),
    m_Saiz(NULL),
    m_Saio(NULL),
    m_Encrypter(encrypter)
{
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter::ProcessFragment
+---------------------------------------------------------------------*/
AP4_Result   
AP4_CencFragmentEncrypter::ProcessFragment()
{
    // create a sample encryption atom
    m_SampleEncryptionAtom       = NULL;
    m_SampleEncryptionAtomShadow = NULL;
    m_Saiz = NULL;
    m_Saio = NULL;
    switch (m_Variant) {
        case AP4_CENC_VARIANT_PIFF_CBC:
            m_SampleEncryptionAtom = new AP4_PiffSampleEncryptionAtom(16);
            break;
            
        case AP4_CENC_VARIANT_PIFF_CTR:
            m_SampleEncryptionAtom = new AP4_PiffSampleEncryptionAtom(8);
            break;
            
        case AP4_CENC_VARIANT_MPEG:
            if (AP4_GlobalOptions::GetBool("mpeg-cenc.piff-compatible")) {
                AP4_UI08 iv_size = 8;
                if (AP4_GlobalOptions::GetBool("mpeg-cenc.iv-size-16")) {
                    iv_size = 16;
                }
                m_SampleEncryptionAtom       = new AP4_SencAtom(iv_size);
                m_SampleEncryptionAtomShadow = new AP4_PiffSampleEncryptionAtom(iv_size);
            } else {
                AP4_UI08 iv_size = 16; // default
                if (AP4_GlobalOptions::GetBool("mpeg-cenc.iv-size-8")) {
                    iv_size = 8;
                }
                m_SampleEncryptionAtom = new AP4_SencAtom(iv_size);
            }
            m_Saiz = new AP4_SaizAtom();
            m_Saio = new AP4_SaioAtom();
            break;
            
        default:
            return AP4_ERROR_INTERNAL;
    }
    if (m_Encrypter->m_SampleEncrypter->UseSubSamples()) {
        m_SampleEncryptionAtom->GetOuter().SetFlags(
            m_SampleEncryptionAtom->GetOuter().GetFlags() |
            AP4_CENC_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION);
        if (m_SampleEncryptionAtomShadow) {
            m_SampleEncryptionAtomShadow->GetOuter().SetFlags(
                m_SampleEncryptionAtomShadow->GetOuter().GetFlags() |
                AP4_CENC_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION);
        }
    }
    
    // this is mostly for testing: forces the clients to parse saio/saiz instead
    // on relying on 'senc'
    if (AP4_GlobalOptions::GetBool("mpeg-cenc.no-senc")) {
        m_SampleEncryptionAtom->GetOuter().SetType(AP4_ATOM_TYPE('s', 'e', 'n', 'C'));
    }

    // add the child atoms to the traf container
    if (m_Saiz) m_Traf->AddChild(m_Saiz);
    if (m_Saio) m_Traf->AddChild(m_Saio);
    m_Traf->AddChild(&m_SampleEncryptionAtom->GetOuter());
    if (m_SampleEncryptionAtomShadow) {
        m_Traf->AddChild(&m_SampleEncryptionAtomShadow->GetOuter());
    }
    
    // set the default-base-is-moof flag
    AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, m_Traf->GetChild(AP4_ATOM_TYPE_TFHD));
    if (tfhd && m_Variant != AP4_CENC_VARIANT_PIFF_CBC && m_Variant != AP4_CENC_VARIANT_PIFF_CTR) {
        tfhd->SetFlags(tfhd->GetFlags() | AP4_TFHD_FLAG_DEFAULT_BASE_IS_MOOF);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter::PrepareForSamples
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencFragmentEncrypter::PrepareForSamples(AP4_FragmentSampleTable* sample_table) { 
    AP4_Cardinal sample_count = sample_table->GetSampleCount();

    // resize the saio atom if we have one
    if (m_Saio) {
        m_Saio->AddEntry(0); // we'll compute the offset later
    }
    
    if (!m_Encrypter->m_SampleEncrypter->UseSubSamples()) {
        m_SampleEncryptionAtom->SetSampleInfosSize(sample_count*m_SampleEncryptionAtom->GetIvSize());
        if (m_SampleEncryptionAtomShadow) {
            m_SampleEncryptionAtomShadow->SetSampleInfosSize(sample_count*m_SampleEncryptionAtomShadow->GetIvSize());
        }
        if (m_Saiz) {
            m_Saiz->SetDefaultSampleInfoSize(m_SampleEncryptionAtom->GetIvSize());
            m_Saiz->SetSampleCount(sample_count);
        }
        return AP4_SUCCESS;
    }
        
    // resize the saiz atom if we have one
    if (m_Saiz) {
        m_Saiz->SetSampleCount(sample_count);
    }

    // prepare for samples
    AP4_Sample          sample;
    AP4_DataBuffer      sample_data;
    AP4_Array<AP4_UI16> bytes_of_cleartext_data;
    AP4_Array<AP4_UI32> bytes_of_encrypted_data;
    AP4_Size            sample_info_size = sample_count*(m_SampleEncryptionAtom->GetIvSize());
    for (unsigned int i=0; i<sample_count; i++) {
        AP4_Result result = sample_table->GetSample(i, sample);
        if (AP4_FAILED(result)) return result;
        result = sample.ReadData(sample_data);
        if (AP4_FAILED(result)) return result;
        bytes_of_cleartext_data.SetItemCount(0);
        bytes_of_encrypted_data.SetItemCount(0);
        result = m_Encrypter->m_SampleEncrypter->GetSubSampleMap(sample_data, 
                                                                 bytes_of_cleartext_data,
                                                                 bytes_of_encrypted_data);
        if (AP4_FAILED(result)) return result;
        sample_info_size += 2+bytes_of_cleartext_data.ItemCount()*6;
        
        if (m_Saiz) {
            m_Saiz->SetSampleInfoSize(i, m_SampleEncryptionAtom->GetIvSize()+2+bytes_of_cleartext_data.ItemCount()*6);
        }
    }
    m_SampleEncryptionAtom->SetSampleInfosSize(sample_info_size);
    if (m_SampleEncryptionAtomShadow) {
        m_SampleEncryptionAtomShadow->SetSampleInfosSize(sample_info_size);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencFragmentEncrypter::ProcessSample(AP4_DataBuffer& data_in,
                                         AP4_DataBuffer& data_out)
{
    // copy the current IV
    AP4_UI08 iv[16];
    AP4_CopyMemory(iv, m_Encrypter->m_SampleEncrypter->GetIv(), 16);
    
    // encrypt the sample
    AP4_DataBuffer sample_infos;
    AP4_Result result = m_Encrypter->m_SampleEncrypter->EncryptSampleData(data_in, data_out, sample_infos);
    if (AP4_FAILED(result)) return result;

    // update the sample info
    m_SampleEncryptionAtom->AddSampleInfo(iv, sample_infos);
    if (m_SampleEncryptionAtomShadow) {
        m_SampleEncryptionAtomShadow->AddSampleInfo(iv, sample_infos);
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter::FinishFragment
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencFragmentEncrypter::FinishFragment()
{
    if (!m_Saio) return AP4_SUCCESS;

    // compute the saio offsets
    AP4_ContainerAtom* moof = AP4_DYNAMIC_CAST(AP4_ContainerAtom, m_Traf->GetParent());
    if (moof == NULL) return AP4_ERROR_INTERNAL;
    
    AP4_UI64 traf_offset = moof->GetHeaderSize();
    AP4_List<AP4_Atom>::Item* child = moof->GetChildren().FirstItem();
    while (child) {
        if (AP4_DYNAMIC_CAST(AP4_ContainerAtom, child->GetData()) == m_Traf) {
            AP4_UI64  senc_offset = m_Traf->GetHeaderSize();
            AP4_Atom* senc_atom = NULL;
            for (AP4_List<AP4_Atom>::Item* item = m_Traf->GetChildren().FirstItem();
                                           item && !senc_atom;
                                           item = item->GetNext()) {
                AP4_Atom* atom = item->GetData();
                if (atom->GetType() == AP4_ATOM_TYPE_SENC ||
                    atom->GetType() == AP4_ATOM_TYPE('s', 'e', 'n', 'C')) { // sometimes used instead of 'senc' for testing
                    senc_atom = atom;
                } else if (atom->GetType() == AP4_ATOM_TYPE_UUID) {
                    AP4_UuidAtom* uuid_atom = AP4_DYNAMIC_CAST(AP4_UuidAtom, atom);
                    if (AP4_CompareMemory(uuid_atom->GetUuid(), AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM, 16) == 0) {
                        senc_atom = atom;
                    }
                }
                if (senc_atom) break;
                senc_offset += atom->GetSize();
            }
        
            if (senc_atom) {
                AP4_UI64 saio_offset = traf_offset+senc_offset+senc_atom->GetHeaderSize()+4;
                m_Saio->SetEntry(0, saio_offset);
            }
        } else {
            traf_offset += child->GetData()->GetSize();
        }
        child = child->GetNext();
    }
    
    return AP4_SUCCESS;
}    

/*----------------------------------------------------------------------
|   AP4_CencEncryptingProcessor:AP4_CencEncryptingProcessor
+---------------------------------------------------------------------*/
AP4_CencEncryptingProcessor::AP4_CencEncryptingProcessor(AP4_CencVariant         variant,
                                                         AP4_BlockCipherFactory* block_cipher_factory) :
    m_Variant(variant)
{
    // create a block cipher factory if none is given
    if (block_cipher_factory == NULL) {
        m_BlockCipherFactory = &AP4_DefaultBlockCipherFactory::Instance;
    } else {
        m_BlockCipherFactory = block_cipher_factory;
    }
}

/*----------------------------------------------------------------------
|   AP4_CencEncryptingProcessor:~AP4_CencEncryptingProcessor
+---------------------------------------------------------------------*/
AP4_CencEncryptingProcessor::~AP4_CencEncryptingProcessor()
{
    m_Encrypters.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_CencEncryptingProcessor::Initialize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencEncryptingProcessor::Initialize(AP4_AtomParent&                  top_level,
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
        
        // add the compatible brand if it is not already there
        if (m_Variant == AP4_CENC_VARIANT_PIFF_CBC || m_Variant == AP4_CENC_VARIANT_PIFF_CTR) {
            if (!ftyp->HasCompatibleBrand(AP4_PIFF_BRAND)) {
                compatible_brands.Append(AP4_PIFF_BRAND);
            }
        } else if (m_Variant == AP4_CENC_VARIANT_MPEG) {
            if (!ftyp->HasCompatibleBrand(AP4_FILE_BRAND_ISO6)) {
                compatible_brands.Append(AP4_FILE_BRAND_ISO6);
            }
        }

        // create a replacement
        AP4_FtypAtom* new_ftyp = new AP4_FtypAtom(ftyp->GetMajorBrand(),
                                                  ftyp->GetMinorVersion(),
                                                  &compatible_brands[0],
                                                  compatible_brands.ItemCount());
        delete ftyp;
        ftyp = new_ftyp;
    } else {
        AP4_UI32 compat = AP4_FILE_BRAND_ISO6;
        if (m_Variant == AP4_CENC_VARIANT_PIFF_CBC || m_Variant == AP4_CENC_VARIANT_PIFF_CTR) {
            compat = AP4_PIFF_BRAND;
        }
        ftyp = new AP4_FtypAtom(AP4_FTYP_BRAND_MP42, 0, &compat, 1);
    }
    
    // insert the ftyp atom as the first child
    AP4_Result result = top_level.AddChild(ftyp, 0);
    if (AP4_FAILED(result)) return result;
    
    // check if we need to create a Marlin 'mkid' table
    AP4_MkidAtom* mkid = NULL;
    if (m_Variant == AP4_CENC_VARIANT_MPEG) {
        const AP4_List<AP4_TrackPropertyMap::Entry>& prop_entries = m_PropertyMap.GetEntries();
        for (unsigned int i=0; i<prop_entries.ItemCount(); i++) {
            AP4_TrackPropertyMap::Entry* entry = NULL;
            prop_entries.Get(i, entry);
            if (entry && entry->m_Name == "ContentId") {
                if (mkid == NULL) mkid = new AP4_MkidAtom();
                const char* kid_hex = m_PropertyMap.GetProperty(entry->m_TrackId, "KID");
                
                if (kid_hex && AP4_StringLength(kid_hex) == 32) {
                    AP4_UI08 kid[16];
                    AP4_ParseHex(kid_hex, kid, 16);
                    // only add this entry if there isn't already an entry for this KID
                    const AP4_Array<AP4_MkidAtom::Entry>& entries = mkid->GetEntries();
                    bool found = false;
                    for (unsigned int i=0; i<entries.ItemCount() && !found; i++) {
                        if (AP4_CompareMemory(entries[i].m_KID, kid, 16) == 0) {
                            found = true;
                        }
                    }
                    if (!found) {
                        mkid->AddEntry(kid, entry->m_Value.GetChars());
                    }
                }
            }
        }
    }
    if (mkid) {
        // find the moov container
        AP4_ContainerAtom* moov = AP4_DYNAMIC_CAST(AP4_ContainerAtom, top_level.GetChild(AP4_ATOM_TYPE_MOOV));
        if (moov) {
            // create a 'marl' container
            AP4_ContainerAtom* marl = new AP4_ContainerAtom(AP4_ATOM_TYPE_MARL);
            marl->AddChild(mkid);
            
            // see if we need to pad the 'pssh' container
            const char* padding_str = m_PropertyMap.GetProperty(0, "PsshPadding");
            AP4_UI32    padding = 0;
            if (padding_str) {
                padding = AP4_ParseIntegerU(padding_str);
            }
            
            // create a 'pssh' atom to contain the 'marl' atom
            AP4_PsshAtom* pssh = new AP4_PsshAtom(AP4_MARLIN_PSSH_SYSTEM_ID);
            pssh->SetData(*marl);
            if (padding > marl->GetSize()+32 && padding < 1024*1024) {
                padding -= (AP4_UI32)marl->GetSize()+32;
                AP4_UI08* data = new AP4_UI08[padding];
                AP4_SetMemory(data, 0, padding);
                pssh->SetPadding(data, padding);
                delete[] data;
            }
            
            // add the 'pssh' atom at the end of the 'moov' container
            // but before any 'free' atom, if any
            int position = -1;
            int current = 0;
            for (AP4_List<AP4_Atom>::Item* child = moov->GetChildren().FirstItem();
                                           child;
                                           child = child->GetNext(), ++current) {
                if (child->GetData()->GetType() == AP4_ATOM_TYPE_FREE) {
                    position = current;
                }
            }
            moov->AddChild(pssh, position);
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencEncryptingProcessor:CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_CencEncryptingProcessor::CreateTrackHandler(AP4_TrakAtom* trak)
{
    // find the stsd atom
    AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));

    // avoid tracks with no stsd atom (should not happen)
    if (stsd == NULL) return NULL;

    // get the list of sample entries
    AP4_Array<AP4_SampleEntry*> entries;
    for (unsigned int i=0; i<stsd->GetSampleDescriptionCount(); i++) {
        AP4_SampleEntry* entry = stsd->GetSampleEntry(i);
        if (entry == NULL) return NULL;
        entries.Append(entry);
    }
        
    // create a handler for this track if we have a key for it and we know
    // how to map the type
    const AP4_DataBuffer* key;
    const AP4_DataBuffer* iv;
    if (AP4_FAILED(m_KeyMap.GetKeyAndIv(trak->GetId(), key, iv))) {
        return NULL;
    }
    if (iv == NULL || iv->GetDataSize() != 16) {
        return NULL;
    }
    
    AP4_UI32 format = 0;
    switch (entries[0]->GetType()) { // only look at the type of the first entry
        case AP4_ATOM_TYPE_MP4A:
            format = AP4_ATOM_TYPE_ENCA;
            break;

        case AP4_ATOM_TYPE_MP4V:
        case AP4_ATOM_TYPE_AVC1:
        case AP4_ATOM_TYPE_AVC2:
        case AP4_ATOM_TYPE_AVC3:
        case AP4_ATOM_TYPE_AVC4:
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
    AP4_UI32 algorithm_id = 0;
    AP4_UI08 iv_size = 16;
    switch (m_Variant) {
        case AP4_CENC_VARIANT_PIFF_CTR:
            algorithm_id = AP4_CENC_ALGORITHM_ID_CTR;
            iv_size = 8;
            break;

        case AP4_CENC_VARIANT_PIFF_CBC:
            algorithm_id = AP4_CENC_ALGORITHM_ID_CBC;
            break;
        
        case AP4_CENC_VARIANT_MPEG:
            if ((AP4_GlobalOptions::GetBool("mpeg-cenc.piff-compatible") ||
                 AP4_GlobalOptions::GetBool("mpeg-cenc.iv-size-8")) &&
                 !AP4_GlobalOptions::GetBool("mpeg-cenc.iv-size-16")) {
                iv_size = 8;
            }
            algorithm_id = AP4_CENC_ALGORITHM_ID_CTR;
            break;
            
        default:
            return NULL;
    }
    track_encrypter = new AP4_CencTrackEncrypter(m_Variant,
                                                 algorithm_id, 
                                                 iv_size,
                                                 kid,
                                                 entries, 
                                                 format);
        
    // create a block cipher
    AP4_BlockCipher*            block_cipher = NULL;
    AP4_BlockCipher::CipherMode mode;
    AP4_BlockCipher::CtrParams  ctr_params;
    const void*                 mode_params = NULL;
    switch (algorithm_id) {
        case AP4_CENC_ALGORITHM_ID_CBC:
            mode = AP4_BlockCipher::CBC;
            break;
            
        case AP4_CENC_ALGORITHM_ID_CTR:
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
    AP4_CencSampleEncrypter* sample_encrypter = NULL;
    AP4_StreamCipher* stream_cipher = NULL;
    switch (algorithm_id) {
        case AP4_CENC_ALGORITHM_ID_CBC:
            stream_cipher = new AP4_CbcStreamCipher(block_cipher);
            if (entries[0]->GetType() == AP4_ATOM_TYPE_AVC1 ||
                entries[0]->GetType() == AP4_ATOM_TYPE_AVC2 ||
                entries[0]->GetType() == AP4_ATOM_TYPE_AVC3 ||
                entries[0]->GetType() == AP4_ATOM_TYPE_AVC4) {
                AP4_AvccAtom* avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, entries[0]->GetChild(AP4_ATOM_TYPE_AVCC));
                if (avcc == NULL) return NULL;
                sample_encrypter = new AP4_CencCbcSubSampleEncrypter(stream_cipher, avcc->GetNaluLengthSize());
            } else {
                sample_encrypter = new AP4_CencCbcSampleEncrypter(stream_cipher);
            }
            break;
            
        case AP4_CENC_ALGORITHM_ID_CTR:
            stream_cipher = new AP4_CtrStreamCipher(block_cipher, 16);
            if (entries[0]->GetType() == AP4_ATOM_TYPE_AVC1 ||
                entries[0]->GetType() == AP4_ATOM_TYPE_AVC2 ||
                entries[0]->GetType() == AP4_ATOM_TYPE_AVC3 ||
                entries[0]->GetType() == AP4_ATOM_TYPE_AVC4) {
                AP4_AvccAtom* avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, entries[0]->GetChild(AP4_ATOM_TYPE_AVCC));
                if (avcc == NULL) return NULL;
                sample_encrypter = new AP4_CencCtrSubSampleEncrypter(stream_cipher, avcc->GetNaluLengthSize(), iv_size);
            } else {
                sample_encrypter = new AP4_CencCtrSampleEncrypter(stream_cipher, iv_size);
            }
            break;
    }
    sample_encrypter->SetIv(iv->GetData());
    m_Encrypters.Add(new Encrypter(trak->GetId(), sample_encrypter));

    return track_encrypter;
}

/*----------------------------------------------------------------------
|   AP4_CencEncryptingProcessor:CreateFragmentHandler
+---------------------------------------------------------------------*/
AP4_Processor::FragmentHandler* 
AP4_CencEncryptingProcessor::CreateFragmentHandler(AP4_TrexAtom*      /* trex */,
                                                   AP4_ContainerAtom* traf,
                                                   AP4_ByteStream&    /* moof_data   */,
                                                   AP4_Position       /* moof_offset */)
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
    return new AP4_CencFragmentEncrypter(m_Variant, traf, encrypter);
}

/*----------------------------------------------------------------------
|   AP4_CencSingleSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencSingleSampleDecrypter::Create(AP4_UI32                        algorithm_id,
                                      const AP4_UI08*                 key,
                                      AP4_Size                        key_size,
                                      AP4_BlockCipherFactory*         block_cipher_factory,
                                      AP4_CencSingleSampleDecrypter*& decrypter)
{
    // check the parameters
    if (key == NULL) return AP4_ERROR_INVALID_PARAMETERS;
    
    // use the default cipher factory if  none was passed
    if (block_cipher_factory == NULL) {
        block_cipher_factory = &AP4_DefaultBlockCipherFactory::Instance;
    }
    
    // create the cipher
    AP4_StreamCipher* stream_cipher = NULL;
    bool              full_blocks_only = false;
    switch (algorithm_id) {
        case AP4_CENC_ALGORITHM_ID_NONE:
            break;
            
        case AP4_CENC_ALGORITHM_ID_CTR: {
            // create the block cipher
            AP4_BlockCipher* block_cipher = NULL;
            AP4_BlockCipher::CtrParams ctr_params;
            ctr_params.counter_size = 8;
            AP4_Result result = block_cipher_factory->CreateCipher(AP4_BlockCipher::AES_128, 
                                                                   AP4_BlockCipher::DECRYPT, 
                                                                   AP4_BlockCipher::CTR,
                                                                   &ctr_params,
                                                                   key, 
                                                                   key_size, 
                                                                   block_cipher);
            if (AP4_FAILED(result)) return result;
            
            // create the stream cipher
            stream_cipher = new AP4_CtrStreamCipher(block_cipher, 8);
            break;
        }
            
        case AP4_CENC_ALGORITHM_ID_CBC: {
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

            // create the stream cipher
            stream_cipher = new AP4_CbcStreamCipher(block_cipher);
            
            // with CBC, we only use full blocks
            full_blocks_only = true;
            break;
        }
        
        default:
            return AP4_ERROR_NOT_SUPPORTED;
    }

    // create the decrypter
    decrypter = new AP4_CencSingleSampleDecrypter(stream_cipher, full_blocks_only);                    

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSingleSampleDecrypter::~AP4_CencSingleSampleDecrypter
+---------------------------------------------------------------------*/
AP4_CencSingleSampleDecrypter::~AP4_CencSingleSampleDecrypter() {
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_CencSingleSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSingleSampleDecrypter::DecryptSampleData(AP4_DataBuffer& data_in,
                                                 AP4_DataBuffer& data_out,
                                                 const AP4_UI08* iv,
                                                 unsigned int    subsample_count,
                                                 const AP4_UI16* bytes_of_cleartext_data,
                                                 const AP4_UI32* bytes_of_encrypted_data)
{
    // the output has the same size as the input
    data_out.SetDataSize(data_in.GetDataSize());

    // check input parameters
    if (iv == NULL) return AP4_ERROR_INVALID_PARAMETERS;
    if (subsample_count) {
        if (bytes_of_cleartext_data == NULL || bytes_of_encrypted_data == NULL) {
            return AP4_ERROR_INVALID_PARAMETERS;
        }
    }
    
    // shortcut for NULL ciphers
    if (m_Cipher == NULL) {
        AP4_CopyMemory(data_out.UseData(), data_in.GetData(), data_in.GetDataSize());
        return AP4_SUCCESS;
    }
    
    // setup direct pointers to the buffers
    const AP4_UI08* in  = data_in.GetData();
    AP4_UI08*       out = data_out.UseData();

    // setup the IV
    m_Cipher->SetIV(iv);

    if (subsample_count) {
        // process the sample data, one sub-sample at a time
        const AP4_UI08* in_end = data_in.GetData()+data_in.GetDataSize();
        for (unsigned int i=0; i<subsample_count; i++) {
            AP4_UI16 cleartext_size = bytes_of_cleartext_data[i];
            AP4_UI32 encrypted_size = bytes_of_encrypted_data[i];

            // check bounds
            if ((unsigned int)(in_end-in) < cleartext_size+encrypted_size) {
                return AP4_ERROR_INVALID_FORMAT;
            }

            // copy the cleartext portion
            if (cleartext_size) {
                AP4_CopyMemory(out, in, cleartext_size);
            }
            
            // decrypt the rest
            if (encrypted_size) {
                AP4_Result result = m_Cipher->ProcessBuffer(in+cleartext_size, encrypted_size, out+cleartext_size, &encrypted_size, false);
                if (AP4_FAILED(result)) return result;
            }
            
            // move the pointers
            in  += cleartext_size+encrypted_size;
            out += cleartext_size+encrypted_size;
        }
    } else {
        if (m_FullBlocksOnly) {
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
        } else {
            // process the entire sample data at once
            AP4_Size encrypted_size = data_in.GetDataSize();
            AP4_Result result = m_Cipher->ProcessBuffer(in, encrypted_size, out, &encrypted_size, false);
            if (AP4_FAILED(result)) return result;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleDecrypter::Create(AP4_ProtectedSampleDescription* sample_description, 
                                AP4_ContainerAtom*              traf,
                                AP4_ByteStream&                 aux_info_data,
                                AP4_Position                    aux_info_data_offset,
                                const AP4_UI08*                 key, 
                                AP4_Size                        key_size,
                                AP4_BlockCipherFactory*         block_cipher_factory,
                                AP4_CencSampleDecrypter*&       decrypter)
{
    AP4_SaioAtom* saio = NULL;
    AP4_SaizAtom* saiz = NULL;
    AP4_CencSampleEncryption* sample_encryption_atom = NULL;
    return Create(sample_description, 
                  traf,
                  aux_info_data,
                  aux_info_data_offset,
                  key,
                  key_size,
                  block_cipher_factory,
                  saio,
                  saiz,
                  sample_encryption_atom,
                  decrypter);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleDecrypter::Create(AP4_ProtectedSampleDescription* sample_description, 
                                AP4_ContainerAtom*              traf,
                                AP4_ByteStream&                 aux_info_data,
                                AP4_Position                    aux_info_data_offset,
                                const AP4_UI08*                 key, 
                                AP4_Size                        key_size,
                                AP4_BlockCipherFactory*         block_cipher_factory,
                                AP4_SaioAtom*&                  saio,
                                AP4_SaizAtom*&                  saiz,
                                AP4_CencSampleEncryption*&      sample_encryption_atom,
                                AP4_CencSampleDecrypter*&       decrypter)
{
    // default return values
    saio                   = NULL;
    saiz                   = NULL;
    sample_encryption_atom = NULL;
    decrypter              = NULL;
    
    // create the sample info table
    AP4_CencSampleInfoTable* sample_info_table = NULL;
    AP4_UI32                 algorithm_id = 0;
    AP4_Result result = AP4_CencSampleInfoTable::Create(sample_description,
                                                        traf,
                                                        saio,
                                                        saiz,
                                                        sample_encryption_atom,
                                                        algorithm_id,
                                                        aux_info_data,
                                                        aux_info_data_offset,
                                                        sample_info_table);
    if (AP4_FAILED(result)) {
        return result;
    }

    // we now have all the info we need to create the decrypter
    return Create(sample_info_table, algorithm_id, key, key_size, block_cipher_factory, decrypter);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencSampleDecrypter::Create(AP4_CencSampleInfoTable*  sample_info_table,
                                AP4_UI32                  algorithm_id,
                                const AP4_UI08*           key, 
                                AP4_Size                  key_size,
                                AP4_BlockCipherFactory*   block_cipher_factory,
                                AP4_CencSampleDecrypter*& decrypter)
{
    // default return value
    decrypter = NULL;
    
    // check some basic paramaters
    unsigned int      iv_size = sample_info_table->GetIvSize();
    switch (algorithm_id) {
        case AP4_CENC_ALGORITHM_ID_NONE:
            break;
            
        case AP4_CENC_ALGORITHM_ID_CTR:
            if (iv_size != 8 && iv_size != 16) {
                return AP4_ERROR_INVALID_FORMAT;
            }
            break;
            
        case AP4_CENC_ALGORITHM_ID_CBC:
            if (iv_size != 16) {
                return AP4_ERROR_INVALID_FORMAT;
            }
            break;
            
        default:
            return AP4_ERROR_NOT_SUPPORTED;
    }

    // create a single-sample decrypter
    AP4_CencSingleSampleDecrypter* single_sample_decrypter = NULL;
    AP4_Result result = AP4_CencSingleSampleDecrypter::Create(algorithm_id, key, key_size, block_cipher_factory, single_sample_decrypter);
    if (AP4_FAILED(result)) return result;

    // create the decrypter
    decrypter = new AP4_CencSampleDecrypter(single_sample_decrypter, sample_info_table);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleDecrypter::~AP4_CencSampleDecrypter
+---------------------------------------------------------------------*/
AP4_CencSampleDecrypter::~AP4_CencSampleDecrypter()
{
	delete m_SampleInfoTable;
	delete m_SingleSampleDecrypter;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleDecrypter::SetSampleIndex
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleDecrypter::SetSampleIndex(AP4_Ordinal sample_index)
{
    m_SampleCursor = sample_index;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleDecrypter::DecryptSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleDecrypter::DecryptSampleData(AP4_DataBuffer& data_in,
                                           AP4_DataBuffer& data_out,
                                           const AP4_UI08* iv)
{
    // increment the sample cursor
    unsigned int sample_cursor = m_SampleCursor++;

    // setup the IV
    unsigned char iv_block[16];
    if (iv == NULL) {
        iv = m_SampleInfoTable->GetIv(sample_cursor);
    }
    if (iv == NULL) return AP4_ERROR_INVALID_FORMAT;
    unsigned int iv_size = m_SampleInfoTable->GetIvSize();
    AP4_CopyMemory(iv_block, iv, iv_size);
    if (iv_size != 16) AP4_SetMemory(&iv_block[iv_size], 0, 16-iv_size);

    // get the subsample info to this sample if needed
    unsigned int    subsample_count = 0;
    const AP4_UI16* bytes_of_cleartext_data = NULL;
    const AP4_UI32* bytes_of_encrypted_data = NULL;
    if (m_SampleInfoTable) {
        AP4_Result result = m_SampleInfoTable->GetSampleInfo(sample_cursor, subsample_count, bytes_of_cleartext_data, bytes_of_encrypted_data);
        if (AP4_FAILED(result)) return result;
    }
    
    // decrypt the sample
    return m_SingleSampleDecrypter->DecryptSampleData(data_in, data_out, iv_block, subsample_count, bytes_of_cleartext_data, bytes_of_encrypted_data);
}

/*----------------------------------------------------------------------
|   AP4_CencTrackDecrypter
+---------------------------------------------------------------------*/
class AP4_CencTrackDecrypter : public AP4_Processor::TrackHandler {
public:
    AP4_IMPLEMENT_DYNAMIC_CAST(AP4_CencTrackDecrypter)

    // constructor
    static AP4_Result Create(const unsigned char*                        key,
                             AP4_Size                                    key_size,
                             AP4_Array<AP4_ProtectedSampleDescription*>& sample_descriptions,
                             AP4_Array<AP4_SampleEntry*>&                sample_entries,
                             AP4_CencTrackDecrypter*&                    decrypter);

    // methods
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);
    virtual AP4_Result ProcessTrack();

    // accessors
    AP4_ProtectedSampleDescription* GetSampleDescription(unsigned int i) {
        if (i < m_SampleDescriptions.ItemCount()) {
            return m_SampleDescriptions[i];
        } else {
            return NULL;
        }
    }
    
private:
    // constructor
    AP4_CencTrackDecrypter(AP4_Array<AP4_ProtectedSampleDescription*>& sample_descriptions,
                           AP4_Array<AP4_SampleEntry*>&                sample_entries,
                           AP4_UI32                                    original_format);

    // members
    AP4_Array<AP4_ProtectedSampleDescription*> m_SampleDescriptions;
    AP4_Array<AP4_SampleEntry*>                m_SampleEntries;
    AP4_UI32                                   m_OriginalFormat;
};

/*----------------------------------------------------------------------
|   AP4_CencTrackDecrypter Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_CencTrackDecrypter)

/*----------------------------------------------------------------------
|   AP4_CencTrackDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencTrackDecrypter::Create(const unsigned char*                        key,
                               AP4_Size                                    /* key_size */,
                               AP4_Array<AP4_ProtectedSampleDescription*>& sample_descriptions,
                               AP4_Array<AP4_SampleEntry*>&                sample_entries,
                               AP4_CencTrackDecrypter*&                    decrypter)
{
    decrypter = NULL;

    // check and set defaults
    if (key == NULL) {
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    // instantiate the object
    decrypter = new AP4_CencTrackDecrypter(sample_descriptions,
                                           sample_entries, 
                                           sample_descriptions[0]->GetOriginalFormat());
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencTrackDecrypter::AP4_CencTrackDecrypter
+---------------------------------------------------------------------*/
AP4_CencTrackDecrypter::AP4_CencTrackDecrypter(AP4_Array<AP4_ProtectedSampleDescription*>& sample_descriptions,
                                               AP4_Array<AP4_SampleEntry*>&                sample_entries,
                                               AP4_UI32                                    original_format) :
    m_OriginalFormat(original_format)
{
    for (unsigned int i=0; i<sample_descriptions.ItemCount(); i++) {
        m_SampleDescriptions.Append(sample_descriptions[i]);
    }
    for (unsigned int i=0; i<sample_entries.ItemCount(); i++) {
        m_SampleEntries.Append(sample_entries[i]);
    }
}

/*----------------------------------------------------------------------
|   AP4_CencTrackDecrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencTrackDecrypter::ProcessSample(AP4_DataBuffer& data_in,
                                      AP4_DataBuffer& data_out)
{
    data_out.SetData(data_in.GetData(), data_in.GetDataSize());
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencTrackDecrypter::ProcessTrack
+---------------------------------------------------------------------*/
AP4_Result   
AP4_CencTrackDecrypter::ProcessTrack()
{
    for (unsigned int i=0; i<m_SampleEntries.ItemCount(); i++) {
        m_SampleEntries[i]->SetType(m_OriginalFormat);
        m_SampleEntries[i]->DeleteChild(AP4_ATOM_TYPE_SINF);
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentDecrypter
+---------------------------------------------------------------------*/
class AP4_CencFragmentDecrypter : public AP4_Processor::FragmentHandler {
public:
    // constructor
    AP4_CencFragmentDecrypter(AP4_CencSampleDecrypter*  sample_decrypter,
                              AP4_SaioAtom*             saio_atom,
                              AP4_SaizAtom*             saiz_atom,
                              AP4_CencSampleEncryption* sample_encryption_atom) :
    m_SampleDecrypter(sample_decrypter),
    m_SaioAtom(saio_atom),
    m_SaizAtom(saiz_atom),
    m_SampleEncryptionAtom(sample_encryption_atom) {}

    // methods
    virtual AP4_Result ProcessFragment();
    virtual AP4_Result FinishFragment();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);

private:
    // members
    AP4_CencSampleDecrypter*  m_SampleDecrypter;
    AP4_SaioAtom*             m_SaioAtom;
    AP4_SaizAtom*             m_SaizAtom;
    AP4_CencSampleEncryption* m_SampleEncryptionAtom;
};

/*----------------------------------------------------------------------
|   AP4_CencFragmentDecrypter::ProcessFragment
+---------------------------------------------------------------------*/
AP4_Result   
AP4_CencFragmentDecrypter::ProcessFragment()
{
    // detach the sample encryption atom
    if (m_SampleDecrypter) {
        if (m_SaioAtom) m_SaioAtom->Detach();
        if (m_SaizAtom) m_SaizAtom->Detach();
        if (m_SampleEncryptionAtom) m_SampleEncryptionAtom->GetOuter().Detach();
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentDecrypter::FinishFragment
+---------------------------------------------------------------------*/
AP4_Result   
AP4_CencFragmentDecrypter::FinishFragment()
{
    if (m_SampleDecrypter) {
        delete m_SaioAtom; 
        m_SaioAtom = NULL;
        delete m_SaizAtom;
        m_SaizAtom = NULL;
        delete m_SampleEncryptionAtom;
        m_SampleEncryptionAtom = NULL;
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentDecrypter::ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencFragmentDecrypter::ProcessSample(AP4_DataBuffer& data_in,
                                         AP4_DataBuffer& data_out)
{
    // decrypt the sample
    return m_SampleDecrypter->DecryptSampleData(data_in, data_out, NULL);
}

/*----------------------------------------------------------------------
|   AP4_CencDecryptingProcessor::AP4_CencDecryptingProcessor
+---------------------------------------------------------------------*/
AP4_CencDecryptingProcessor::AP4_CencDecryptingProcessor(const AP4_ProtectionKeyMap* key_map, 
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
|   AP4_CencDecryptingProcessor:CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_CencDecryptingProcessor::CreateTrackHandler(AP4_TrakAtom* trak)
{
    // find the stsd atom
    AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));

    // avoid tracks with no stsd atom (should not happen)
    if (stsd == NULL) return NULL;

    // if we don't have keys, don't bother
    if (m_KeyMap == NULL) return NULL;

    // build a list of sample descriptions
    AP4_Array<AP4_ProtectedSampleDescription*> sample_descs;
    AP4_Array<AP4_SampleEntry*>                sample_entries;
    for (unsigned int i=0; i<stsd->GetSampleDescriptionCount(); i++) {
        AP4_SampleDescription* sample_desc  = stsd->GetSampleDescription(i);
        AP4_SampleEntry*       sample_entry = stsd->GetSampleEntry(i);
        if (sample_desc == NULL || sample_entry == NULL) continue;
        if (sample_desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
            AP4_ProtectedSampleDescription* protected_desc = 
                static_cast<AP4_ProtectedSampleDescription*>(sample_desc);
            if (protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_PIFF ||
                protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CENC) {
                sample_descs.Append(protected_desc);
                sample_entries.Append(sample_entry);
            }
        }
    }
    if (sample_entries.ItemCount() == 0) return NULL;
    const AP4_DataBuffer* key = m_KeyMap->GetKey(trak->GetId());
    if (key) {
        AP4_CencTrackDecrypter* handler = NULL;
        AP4_Result result = AP4_CencTrackDecrypter::Create(key->GetData(), 
                                                           key->GetDataSize(), 
                                                           sample_descs, 
                                                           sample_entries, 
                                                           handler);
        if (AP4_FAILED(result)) return NULL;
        return handler;
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_CencDecryptingProcessor::CreateFragmentHandler
+---------------------------------------------------------------------*/
AP4_Processor::FragmentHandler* 
AP4_CencDecryptingProcessor::CreateFragmentHandler(AP4_TrexAtom*      trex,
                                                   AP4_ContainerAtom* traf,
                                                   AP4_ByteStream&    moof_data,
                                                   AP4_Position       moof_offset)
{
    // find the matching track handler to get to the track sample description
    const AP4_DataBuffer* key = NULL;
    AP4_ProtectedSampleDescription* sample_description = NULL;
    for (unsigned int i=0; i<m_TrackIds.ItemCount(); i++) {
        AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
        if (tfhd && m_TrackIds[i] == tfhd->GetTrackId()) {
            // look for the Track Encryption Box
            AP4_CencTrackDecrypter* track_decrypter = 
                AP4_DYNAMIC_CAST(AP4_CencTrackDecrypter, m_TrackHandlers[i]);
            if (track_decrypter) {
                unsigned int index = trex->GetDefaultSampleDescriptionIndex();
                unsigned int tfhd_flags = tfhd->GetFlags();
                if (tfhd_flags & AP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT) {
                    index = tfhd->GetSampleDescriptionIndex();
                }
                if (index >= 1) {
                    sample_description = track_decrypter->GetSampleDescription(index-1);
                }
                if (sample_description == NULL) return NULL;
            }
            
            // get the matching key
            key = m_KeyMap->GetKey(tfhd->GetTrackId());
            
            break;
        }
    }
    if (sample_description == NULL) return NULL;
    
    // create the sample decrypter for the fragment
    AP4_CencSampleDecrypter* sample_decrypter = NULL;
    AP4_SaioAtom* saio = NULL;
    AP4_SaizAtom* saiz = NULL;
    AP4_CencSampleEncryption* sample_encryption_atom = NULL;
    AP4_Result result = AP4_CencSampleDecrypter::Create(
        sample_description, 
        traf, 
        moof_data,
        moof_offset,
        key->GetData(), 
        key->GetDataSize(), 
        NULL,
        saio,
        saiz,
        sample_encryption_atom,
        sample_decrypter);
    if (AP4_FAILED(result)) return NULL;
    
    return new AP4_CencFragmentDecrypter(sample_decrypter, saio, saiz, sample_encryption_atom);
}
    
/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_CencTrackEncryption)

/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption::AP4_CencTrackEncryption
+---------------------------------------------------------------------*/
AP4_CencTrackEncryption::AP4_CencTrackEncryption() :
    m_DefaultAlgorithmId(0),
    m_DefaultIvSize(0)
{
    AP4_SetMemory(m_DefaultKid, 0, 16);
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption::AP4_CencTrackEncryption
+---------------------------------------------------------------------*/
AP4_CencTrackEncryption::AP4_CencTrackEncryption(AP4_UI32        default_algorithm_id,
                                                 AP4_UI08        default_iv_size,
                                                 const AP4_UI08* default_kid) :
    m_DefaultAlgorithmId(default_algorithm_id),
    m_DefaultIvSize(default_iv_size)
{
    AP4_CopyMemory(m_DefaultKid, default_kid, 16);
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption::AP4_CencTrackEncryption
+---------------------------------------------------------------------*/
AP4_CencTrackEncryption::AP4_CencTrackEncryption(AP4_ByteStream& stream)
{
    stream.ReadUI24(m_DefaultAlgorithmId);
    stream.ReadUI08(m_DefaultIvSize);
    AP4_SetMemory(m_DefaultKid, 0, 16);
    stream.Read(m_DefaultKid, 16);
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption::DoInspectFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencTrackEncryption::DoInspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("default_AlgorithmID", m_DefaultAlgorithmId);
    inspector.AddField("default_IV_size",     m_DefaultIvSize);
    inspector.AddField("default_KID",         m_DefaultKid, 16);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption::DoWriteFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencTrackEncryption::DoWriteFields(AP4_ByteStream& stream)
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
|   AP4_CencSampleEncryption Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_CencSampleEncryption)

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::Create
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleInfoTable::Create(AP4_ProtectedSampleDescription* sample_description,
                                AP4_ContainerAtom*              traf,
                                AP4_UI32&                       algorithm_id,
                                AP4_ByteStream&                 aux_info_data,
                                AP4_Position                    aux_info_data_offset,
                                AP4_CencSampleInfoTable*&       sample_info_table)
{
    AP4_SaioAtom* saio;
    AP4_SaizAtom* saiz;
    AP4_CencSampleEncryption* sample_encryption_atom;
    return Create(sample_description, 
                  traf, 
                  saio,
                  saiz,
                  sample_encryption_atom,
                  algorithm_id,
                  aux_info_data,
                  aux_info_data_offset,
                  sample_info_table);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::Create
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleInfoTable::Create(AP4_ProtectedSampleDescription* sample_description,
                                AP4_ContainerAtom*              traf,
                                AP4_SaioAtom*&                  saio,
                                AP4_SaizAtom*&                  saiz,
                                AP4_CencSampleEncryption*&      sample_encryption_atom,
                                AP4_UI32&                       algorithm_id,
                                AP4_ByteStream&                 aux_info_data,
                                AP4_Position                    aux_info_data_offset,
                                AP4_CencSampleInfoTable*&       sample_info_table)
{
    // default return values
    saio = NULL;
    saiz = NULL;
    sample_encryption_atom = NULL;
    sample_info_table = NULL;
    unsigned int iv_size = 0;
    
    // check the scheme
    if (sample_description->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_PIFF) {
        // we don't support PIFF 1.0 anymore!
        //if (sample_description->GetSchemeVersion() != AP4_PROTECTION_SCHEME_VERSION_PIFF_11) {
        //    return AP4_ERROR_NOT_SUPPORTED;
        //}
    } else if (sample_description->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CENC) {
        if (sample_description->GetSchemeVersion() != AP4_PROTECTION_SCHEME_VERSION_CENC_10) {
            return AP4_ERROR_NOT_SUPPORTED;
        }
    } else {
        return AP4_ERROR_NOT_SUPPORTED;
    }

    // get the scheme info atom
    AP4_ContainerAtom* schi = sample_description->GetSchemeInfo()->GetSchiAtom();
    if (schi == NULL) return AP4_ERROR_INVALID_FORMAT;

    // look for a track encryption atom
    AP4_CencTrackEncryption* track_encryption_atom = 
        AP4_DYNAMIC_CAST(AP4_CencTrackEncryption, schi->GetChild(AP4_ATOM_TYPE_TENC));
    if (track_encryption_atom == NULL) {
        track_encryption_atom = AP4_DYNAMIC_CAST(AP4_CencTrackEncryption, schi->GetChild(AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM));
    }
    
    // look for a sample encryption atom
    if (traf) {
        sample_encryption_atom = AP4_DYNAMIC_CAST(AP4_SencAtom, traf->GetChild(AP4_ATOM_TYPE_SENC));
        if (sample_encryption_atom == NULL) {
            sample_encryption_atom = AP4_DYNAMIC_CAST(AP4_PiffSampleEncryptionAtom, traf->GetChild(AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM));
        }
    }
    
    // parse the crypto params
    if (sample_encryption_atom &&
        sample_encryption_atom->GetOuter().GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        algorithm_id = sample_encryption_atom->GetAlgorithmId();
        iv_size      = sample_encryption_atom->GetIvSize();
    } else {
        if (track_encryption_atom == NULL) return AP4_ERROR_INVALID_FORMAT;
        algorithm_id = track_encryption_atom->GetDefaultAlgorithmId();
        iv_size      = track_encryption_atom->GetDefaultIvSize();
    }

    // try to create a sample info table from senc
    if (sample_info_table == NULL && sample_encryption_atom) {
        AP4_Result result = sample_encryption_atom->CreateSampleInfoTable(iv_size, sample_info_table);
        if (AP4_FAILED(result)) return result;
    }

    // try to create a sample info table from saio/saiz
    if (traf) {
        for (AP4_List<AP4_Atom>::Item* child = traf->GetChildren().FirstItem();
                                       child;
                                       child = child->GetNext()) {
            if (child->GetData()->GetType() == AP4_ATOM_TYPE_SAIO) {
                saio = AP4_DYNAMIC_CAST(AP4_SaioAtom, child->GetData());
                if (saio->GetAuxInfoType() != 0 && saio->GetAuxInfoType() != AP4_PROTECTION_SCHEME_TYPE_CENC) {
                    saio = NULL;
                }
            } else if (child->GetData()->GetType() == AP4_ATOM_TYPE_SAIZ) {
                saiz = AP4_DYNAMIC_CAST(AP4_SaizAtom, child->GetData());
                if (saiz->GetAuxInfoType() != 0 && saiz->GetAuxInfoType() != AP4_PROTECTION_SCHEME_TYPE_CENC) {
                    saiz = NULL;
                }
            }
        }
        if (sample_info_table == NULL && saio && saiz) {
            AP4_Result result = Create(iv_size, 
                                       *traf,
                                       *saio, 
                                       *saiz,
                                       aux_info_data,
                                       aux_info_data_offset, 
                                       sample_info_table);
            if (AP4_FAILED(result)) return result;
        }
    }
    
    if (sample_info_table == NULL) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::Create
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleInfoTable::Create(unsigned int              iv_size, 
                                AP4_ContainerAtom&        traf,
                                AP4_SaioAtom&             saio, 
                                AP4_SaizAtom&             saiz, 
                                AP4_ByteStream&           aux_info_data,
                                AP4_Position              aux_info_data_offset,
                                AP4_CencSampleInfoTable*& sample_info_table)
{
    AP4_Result result = AP4_SUCCESS;
    
    // remember where we are in the data stream so that we can return to that point
    // before returning
    AP4_Position position_before = 0;
    aux_info_data.Tell(position_before);
    
    // count the samples
    unsigned int sample_info_count = 0;
    for (AP4_List<AP4_Atom>::Item* item = traf.GetChildren().FirstItem();
                                   item;
                                   item = item->GetNext()) {
        AP4_Atom* atom = item->GetData();
        if (atom->GetType() == AP4_ATOM_TYPE_TRUN) {
            AP4_TrunAtom* trun = AP4_DYNAMIC_CAST(AP4_TrunAtom, atom);
            sample_info_count += trun->GetEntries().ItemCount();
        }
    }
    
    // create the table
    AP4_CencSampleInfoTable* table = new AP4_CencSampleInfoTable(sample_info_count, iv_size);
    
    // process each sample's auxiliary info
    AP4_Ordinal    saio_index  = 0;
    AP4_Ordinal    saiz_index  = 0;
    AP4_DataBuffer info;
    for (AP4_List<AP4_Atom>::Item* item = traf.GetChildren().FirstItem();
                                   item;
                                   item = item->GetNext()) {
        AP4_Atom* atom = item->GetData();
        if (atom->GetType() == AP4_ATOM_TYPE_TRUN) {
            AP4_TrunAtom* trun = AP4_DYNAMIC_CAST(AP4_TrunAtom, atom);

            if (saio_index == 0) {
                aux_info_data.Seek(aux_info_data_offset+saio.GetEntries()[0]);
            } else {
                if (saio.GetEntries().ItemCount() > 1) {
                    if (saio_index >= saio.GetEntries().ItemCount()) {
                        result = AP4_ERROR_INVALID_FORMAT;
                        goto end;
                    }
                    aux_info_data.Seek(aux_info_data_offset+saio.GetEntries()[saio_index]);
                }
            }
            ++saio_index;
            
            for (unsigned int i=0; i<trun->GetEntries().ItemCount(); i++) {
                AP4_UI08 info_size = 0;
                result = saiz.GetSampleInfoSize(saiz_index, info_size);
                if (AP4_FAILED(result)) goto end;

                info.SetDataSize(info_size);
                result = aux_info_data.Read(info.UseData(), info_size);
                if (AP4_FAILED(result)) goto end;

                const AP4_UI08* info_data = info.GetData();
                table->SetIv(saiz_index, info_data);
                if (info_size > iv_size+2) {
                    AP4_UI16 subsample_count = AP4_BytesToUInt16BE(info_data+iv_size);
                    if (info_size < iv_size+2+subsample_count*6) {
                        // not enough data
                        goto end;
                    }
                    table->AddSubSampleData(subsample_count, info_data+iv_size+2);
                }
                saiz_index++;
            }
        }
    }    
    result = AP4_SUCCESS;
    
end:
    if (AP4_SUCCEEDED(result)) {
        sample_info_table = table;
    } else {
        delete table;
        sample_info_table = NULL;
    }
    aux_info_data.Seek(position_before);    
    return result;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::Create
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleInfoTable::Create(const AP4_UI08*           serialized,
                                unsigned int              serialized_size,
                                AP4_CencSampleInfoTable*& sample_info_table)
{
    sample_info_table = NULL;
    
    if (serialized_size < 4+4) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    AP4_UI32 sample_count = AP4_BytesToUInt32BE(serialized); serialized += 4; serialized_size -= 4;
    AP4_UI32 iv_size      = AP4_BytesToUInt32BE(serialized); serialized += 4; serialized_size -= 4;
    
    if (serialized_size < sample_count*iv_size) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    AP4_CencSampleInfoTable* table = new AP4_CencSampleInfoTable(sample_count, iv_size);
    table->m_IvData.SetData(serialized, sample_count*iv_size);
    serialized      += sample_count*iv_size;
    serialized_size -= sample_count*iv_size;
    
    if (serialized_size < 4) {
        delete table;
        return AP4_ERROR_INVALID_FORMAT;
    }
    AP4_UI32 item_count = AP4_BytesToUInt32BE(serialized); serialized += 4; serialized_size -= 4;
    if (serialized_size < item_count*(2+4)) {
        delete table;
        return AP4_ERROR_INVALID_FORMAT;
    }
    table->m_BytesOfCleartextData.SetItemCount(item_count);
    table->m_BytesOfEncryptedData.SetItemCount(item_count);
    for (unsigned int i=0; i<item_count; i++) {
        table->m_BytesOfCleartextData[i] = AP4_BytesToUInt16BE(serialized);
        serialized      += 2;
        serialized_size -= 2;
    }
    for (unsigned int i=0; i<item_count; i++) {
        table->m_BytesOfEncryptedData[i] = AP4_BytesToUInt32BE(serialized);
        serialized      += 4;
        serialized_size -= 4;
    }
    
    if (serialized_size < 4) {
        delete table;
        return AP4_ERROR_INVALID_FORMAT;
    }
    AP4_UI32 use_subsamples = AP4_BytesToUInt32BE(serialized) != 0; serialized += 4; serialized_size -= 4;
    if (!use_subsamples) {
        sample_info_table = table;
        return AP4_SUCCESS;
    }
    
    if (serialized_size < sample_count*(4+4)) {
        delete table;
        return AP4_ERROR_INVALID_FORMAT;
    }
    table->m_SubSampleMapStarts.SetItemCount(sample_count);
    table->m_SubSampleMapLengths.SetItemCount(sample_count);
    for (unsigned int i=0; i<sample_count; i++) {
        table->m_SubSampleMapStarts[i] = AP4_BytesToUInt32BE(serialized);
        serialized      += 4;
        serialized_size -= 4;
    }
    for (unsigned int i=0; i<sample_count; i++) {
        table->m_SubSampleMapLengths[i] = AP4_BytesToUInt32BE(serialized);
        serialized      += 4;
        serialized_size -= 4;
    }
    
    sample_info_table = table;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::AP4_CencSampleInfoTable
+---------------------------------------------------------------------*/
AP4_CencSampleInfoTable::AP4_CencSampleInfoTable(AP4_UI32 sample_count,
                                                 AP4_UI08 iv_size) :
    m_SampleCount(sample_count),
    m_IvSize(iv_size)
{
    m_IvData.SetDataSize(m_IvSize*sample_count);
    AP4_SetMemory(m_IvData.UseData(), 0, m_IvSize*sample_count);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::Serialize
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencSampleInfoTable::Serialize(AP4_DataBuffer& buffer)
{
    unsigned int size = 4 +
                        4 +
                        m_SampleCount*m_IvSize +
                        4 +
                        m_BytesOfCleartextData.ItemCount()*2 +
                        m_BytesOfEncryptedData.ItemCount()*4 +
                        4;
    bool use_subsamples = m_SubSampleMapStarts.ItemCount() != 0;
    if (use_subsamples) {
        size += m_SampleCount*(4+4);
    }
    
    // sanity check
    if (m_IvData.GetDataSize()             != m_SampleCount*m_IvSize ||
        m_BytesOfCleartextData.ItemCount() != m_BytesOfEncryptedData.ItemCount() ||
        m_SubSampleMapStarts.ItemCount()   != m_SubSampleMapLengths.ItemCount()) {
        return AP4_ERROR_INTERNAL;
    }
    if (use_subsamples && m_SubSampleMapStarts.ItemCount() != m_SampleCount) {
        return AP4_ERROR_INTERNAL;
    }
    
    buffer.SetDataSize(size);
    AP4_UI08* data = buffer.UseData();
    
    AP4_BytesFromUInt32BE(data, m_SampleCount);                       data += 4;
    AP4_BytesFromUInt32BE(data, m_IvSize);                            data += 4;
    AP4_CopyMemory(data, m_IvData.GetData(), m_SampleCount*m_IvSize); data += m_SampleCount*m_IvSize;
    AP4_BytesFromUInt32BE(data, m_BytesOfCleartextData.ItemCount());  data += 4;
    for (unsigned int i=0; i<m_BytesOfCleartextData.ItemCount(); i++) {
        AP4_BytesFromUInt16BE(data, m_BytesOfCleartextData[i]); data += 2;
    }
    for (unsigned int i=0; i<m_BytesOfEncryptedData.ItemCount(); i++) {
        AP4_BytesFromUInt32BE(data, m_BytesOfEncryptedData[i]); data += 4;
    }
    AP4_BytesFromUInt32BE(data, use_subsamples?1:0); data += 4;
    if (use_subsamples) {
        for (unsigned int i=0; i<m_SampleCount; i++) {
            AP4_BytesFromUInt32BE(data, m_SubSampleMapStarts[i]); data += 4;
        }
        for (unsigned int i=0; i<m_SampleCount; i++) {
            AP4_BytesFromUInt32BE(data, m_SubSampleMapLengths[i]); data += 4;
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::SetIv
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleInfoTable::SetIv(AP4_Ordinal sample_index, const AP4_UI08* iv)
{
    if (sample_index >= m_SampleCount) return AP4_ERROR_OUT_OF_RANGE;
    AP4_UI08* dst = m_IvData.UseData()+(m_IvSize*sample_index);
    AP4_CopyMemory(dst, iv, m_IvSize);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::GetIv
+---------------------------------------------------------------------*/
const AP4_UI08* 
AP4_CencSampleInfoTable::GetIv(AP4_Ordinal sample_index)
{
    if (sample_index >= m_SampleCount) return NULL;
    return m_IvData.GetData()+(m_IvSize*sample_index);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::AddSubSampleData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleInfoTable::AddSubSampleData(AP4_Cardinal    subsample_count,
                                          const AP4_UI08* subsample_data)
{
    unsigned int current = m_SubSampleMapStarts.ItemCount();
    if (current == 0) {
        m_SubSampleMapStarts.Append(0);
    } else {
        m_SubSampleMapStarts.Append(m_SubSampleMapStarts[current-1]+
                                    m_SubSampleMapLengths[current-1]);
    }
    m_SubSampleMapLengths.Append(subsample_count);
    for (unsigned int i=0; i<subsample_count; i++) {
        m_BytesOfCleartextData.Append(AP4_BytesToUInt16BE(subsample_data));
        m_BytesOfEncryptedData.Append(AP4_BytesToUInt32BE(subsample_data+2));
        subsample_data += 6;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::GetSampleInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencSampleInfoTable::GetSampleInfo(AP4_Cardinal     sample_index,
                                       AP4_Cardinal&    subsample_count,
                                       const AP4_UI16*& bytes_of_cleartext_data,
                                       const AP4_UI32*& bytes_of_encrypted_data)
{
    
    if (sample_index >= m_SampleCount) {
        return AP4_ERROR_OUT_OF_RANGE;
    }
    
    if (m_SubSampleMapStarts.ItemCount() == 0) {
        // no subsamples
        subsample_count = 0;
        bytes_of_cleartext_data = NULL;
        bytes_of_encrypted_data = NULL;
        return AP4_SUCCESS;
    }
    
    subsample_count         = m_SubSampleMapLengths[sample_index];
    unsigned int target     = m_SubSampleMapStarts[sample_index];
    bytes_of_cleartext_data = &m_BytesOfCleartextData[target];
    bytes_of_encrypted_data = &m_BytesOfEncryptedData[target];
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleInfoTable::GetSubsampleInfo
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleInfoTable::GetSubsampleInfo(AP4_Cardinal sample_index, 
                                          AP4_Cardinal subsample_index,
                                          AP4_UI16&    bytes_of_cleartext_data,
                                          AP4_UI32&    bytes_of_encrypted_data)
{
    if (sample_index    >= m_SampleCount ||
        subsample_index >= m_SubSampleMapLengths[sample_index]) {
        return AP4_ERROR_OUT_OF_RANGE;
    }
    unsigned int target = m_SubSampleMapStarts[sample_index]+subsample_index;
    if (target >= m_BytesOfCleartextData.ItemCount() || target >= m_BytesOfEncryptedData.ItemCount()) {
        return AP4_ERROR_OUT_OF_RANGE;
    }
    bytes_of_cleartext_data = m_BytesOfCleartextData[target];
    bytes_of_encrypted_data = m_BytesOfEncryptedData[target];
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::AP4_CencSampleEncryption
+---------------------------------------------------------------------*/
AP4_CencSampleEncryption::AP4_CencSampleEncryption(AP4_Atom&       outer,
                                                   AP4_Size        size,
                                                   AP4_ByteStream& stream) :
    m_Outer(outer)
{
    if (outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        stream.ReadUI24(m_AlgorithmId);
        stream.ReadUI08(m_IvSize);
        stream.Read    (m_Kid, 16);
    } else {
        m_AlgorithmId = 0;
        m_IvSize      = 0;
        AP4_SetMemory(m_Kid, 0, 16);
    }
    
    stream.ReadUI32(m_SampleInfoCount);

    AP4_Size payload_size = size-m_Outer.GetHeaderSize()-4;
    m_SampleInfos.SetDataSize(payload_size);
    stream.Read(m_SampleInfos.UseData(), payload_size);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::AP4_CencSampleEncryption
+---------------------------------------------------------------------*/
AP4_CencSampleEncryption::AP4_CencSampleEncryption(AP4_Atom& outer, AP4_UI08 iv_size) :
    m_Outer(outer),
    m_AlgorithmId(0),
    m_IvSize(iv_size),
    m_SampleInfoCount(0),
    m_SampleInfoCursor(0)
{
    AP4_SetMemory(m_Kid, 0, 16);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::AP4_CencSampleEncryption
+---------------------------------------------------------------------*/
AP4_CencSampleEncryption::AP4_CencSampleEncryption(AP4_Atom&       outer,
                                                   AP4_UI32        algorithm_id,
                                                   AP4_UI08        iv_size,
                                                   const AP4_UI08* kid) :
    m_Outer(outer),
    m_AlgorithmId(algorithm_id),
    m_IvSize(iv_size),
    m_SampleInfoCount(0),
    m_SampleInfoCursor(0)
{
    AP4_CopyMemory(m_Kid, kid, 16);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::AddSampleInfo
+---------------------------------------------------------------------*/
AP4_Result      
AP4_CencSampleEncryption::AddSampleInfo(const AP4_UI08* iv,
                                        AP4_DataBuffer& subsample_info)
{
    unsigned int added_size = m_IvSize+subsample_info.GetDataSize();
    
    if (m_SampleInfoCursor+added_size > m_SampleInfos.GetDataSize()) {
        // too much data!
        return AP4_ERROR_OUT_OF_RANGE;
    }
    AP4_UI08* info = m_SampleInfos.UseData()+m_SampleInfoCursor;
    AP4_CopyMemory(info, iv, m_IvSize);
    if (subsample_info.GetDataSize()) {
        AP4_CopyMemory(info+m_IvSize, subsample_info.GetData(), subsample_info.GetDataSize());
    }
    m_SampleInfoCursor += added_size;
    ++m_SampleInfoCount;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::SetSampleInfosSize
+---------------------------------------------------------------------*/
AP4_Result      
AP4_CencSampleEncryption::SetSampleInfosSize(AP4_Size size)
{
    m_SampleInfos.SetDataSize(size);
    AP4_SetMemory(m_SampleInfos.UseData(), 0, size);
    if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        m_Outer.SetSize(m_Outer.GetHeaderSize()+20+4+size);
    } else {
        m_Outer.SetSize(m_Outer.GetHeaderSize()+4+size);
    }
    if (m_Outer.GetParent()) {
        AP4_AtomParent* parent = AP4_DYNAMIC_CAST(AP4_AtomParent, m_Outer.GetParent());
        if (parent) {
            parent->OnChildChanged(&m_Outer);
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::CreateSampleInfoTable
+---------------------------------------------------------------------*/
AP4_Result      
AP4_CencSampleEncryption::CreateSampleInfoTable(AP4_Size                  default_iv_size,
                                                AP4_CencSampleInfoTable*& table)
{
    AP4_Size iv_size = default_iv_size;
    if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        iv_size = m_IvSize;
    } 
    bool has_subsamples = false;
    if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
        has_subsamples = true;
    }

    // create the table
    AP4_Result result = AP4_ERROR_INVALID_FORMAT;
    table = new AP4_CencSampleInfoTable(m_SampleInfoCount, iv_size);
    const AP4_UI08* data      = m_SampleInfos.GetData();
    AP4_UI32        data_size = m_SampleInfos.GetDataSize();
    for (unsigned int i=0; i<m_SampleInfoCount; i++) {
        if (data_size < iv_size) goto end;
        table->SetIv(i, data);
        data      += iv_size;
        data_size -= iv_size;
        if (has_subsamples) {
            if (data_size < 2) goto end;
            AP4_UI16 subsample_count = AP4_BytesToUInt16BE(data);
            data      += 2;
            data_size -= 2;
            
            if (data_size < subsample_count*(unsigned int)6) goto end;
            
            result = table->AddSubSampleData(subsample_count, data);
            if (AP4_FAILED(result)) goto end;
            
            data      += 6*subsample_count;
            data_size -= 6*subsample_count;
        }
    }
    result = AP4_SUCCESS;
    
end:
    if (AP4_FAILED(result)) {
        delete table;
        table = NULL;
    }
    return result;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::DoInspectFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleEncryption::DoInspectFields(AP4_AtomInspector& inspector)
{
    if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        inspector.AddField("AlgorithmID", m_AlgorithmId);
        inspector.AddField("IV_size",     m_IvSize);
        inspector.AddField("KID",         m_Kid, 16);
    }
    
    inspector.AddField("sample info count", m_SampleInfoCount);

    if (inspector.GetVerbosity() < 2) {
        return AP4_SUCCESS;
    }
    
    // since we don't know the IV size necessarily (we don't have the context), we
    // will try to guess the IV size (we'll try 16 and 8)
    unsigned int iv_size = m_IvSize;
    if (iv_size == 0) {
        if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
            bool data_ok = false;
            for (unsigned int k=1; k<=2 && !data_ok; k++) {
                data_ok = true;
                iv_size = 8*k;
                const AP4_UI08* info = m_SampleInfos.GetData();
                AP4_Size        info_size = m_SampleInfos.GetDataSize();
                for (unsigned int i=0; i<m_SampleInfoCount; i++) {
                    if (info_size < iv_size+2) {
                        data_ok = false;
                        break;
                    }
                    info      += iv_size;
                    info_size -= iv_size;
                    unsigned int num_entries = AP4_BytesToInt16BE(info);
                    info      += 2;
                    info_size -= 2;
                    if (info_size < num_entries*6) {
                        data_ok = false;
                        break;
                    }
                    info      += num_entries*6;
                    info_size -= num_entries*6;
                }
            }
            if (data_ok == false) return AP4_SUCCESS;
        } else {
            iv_size = m_SampleInfoCount?(m_SampleInfos.GetDataSize()/m_SampleInfoCount):0;
            if (iv_size*m_SampleInfoCount != m_SampleInfos.GetDataSize()) {
                // not a multiple...
                return AP4_SUCCESS;
            }
        }
    }
    inspector.AddField("IV Size (inferred)", iv_size);
    
    const AP4_UI08* info = m_SampleInfos.GetData();
    for (unsigned int i=0; i<m_SampleInfoCount; i++) {
        char header[64];
        AP4_FormatString(header, sizeof(header), "entry %04d", i);
        inspector.AddField(header, info, iv_size);
        info += iv_size;
        if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
            unsigned int num_entries = AP4_BytesToInt16BE(info);
            info += 2;
            for (unsigned int j=0; j<num_entries; j++) {
                unsigned int bocd = AP4_BytesToUInt16BE(info);
                AP4_FormatString(header, sizeof(header), "sub-entry %04d.%d bytes of clear data", i, j);
                inspector.AddField(header, bocd);
                unsigned int boed = AP4_BytesToUInt32BE(info+2);
                AP4_FormatString(header, sizeof(header), "sub-entry %04d.%d bytes of encrypted data", i, j);
                inspector.AddField(header, boed);
                info += 6;
            }
        }        
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::DoWriteFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencSampleEncryption::DoWriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // optional fields
    if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {   
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

