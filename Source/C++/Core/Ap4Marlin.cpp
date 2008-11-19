/*****************************************************************
|
|    AP4 - Marlin File Format Support
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
#include "Ap4SchmAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4Sample.h"
#include "Ap4StreamCipher.h"
#include "Ap4FrmaAtom.h"
#include "Ap4Utils.h"
#include "Ap4TrakAtom.h"
#include "Ap4FtypAtom.h"
#include "Ap4IodsAtom.h"
#include "Ap4MoovAtom.h"
#include "Ap4Track.h"
#include "Ap4DescriptorFactory.h"
#include "Ap4CommandFactory.h"
#include "Ap4Marlin.h"
#include "Ap4FileByteStream.h"
#include "Ap4Ipmp.h"
#include "Ap4AesBlockCipher.h"

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpDecryptingProcessor:AP4_MarlinIpmpDecryptingProcessor
+---------------------------------------------------------------------*/
AP4_MarlinIpmpDecryptingProcessor::AP4_MarlinIpmpDecryptingProcessor(
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
|   AP4_MarlinIpmpDecryptingProcessor::~AP4_MarlinIpmpDecryptingProcessor
+---------------------------------------------------------------------*/
AP4_MarlinIpmpDecryptingProcessor::~AP4_MarlinIpmpDecryptingProcessor()
{
    m_SinfEntries.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpDecryptingProcessor:Initialize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_MarlinIpmpDecryptingProcessor::Initialize(AP4_AtomParent&   top_level, 
                                              AP4_ByteStream&   stream,
                                              ProgressListener* /*listener*/)
{
    // check the file type
    AP4_FtypAtom* ftyp = dynamic_cast<AP4_FtypAtom*>(top_level.GetChild(AP4_ATOM_TYPE_FTYP));
    if (ftyp == NULL ||
        (ftyp->GetMajorBrand() != AP4_MARLIN_BRAND_MGSV && !ftyp->HasCompatibleBrand(AP4_MARLIN_BRAND_MGSV))) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    
    // check the initial object descriptor and get the OD Track ID
    AP4_IodsAtom* iods = dynamic_cast<AP4_IodsAtom*>(top_level.FindChild("moov/iods"));
    AP4_UI32      od_track_id = 0;
    if (iods == NULL) return AP4_ERROR_INVALID_FORMAT;
    const AP4_ObjectDescriptor* od = iods->GetObjectDescriptor();
    if (od == NULL) return AP4_ERROR_INVALID_FORMAT;
    const AP4_EsIdIncDescriptor* es_id_inc = dynamic_cast<const AP4_EsIdIncDescriptor*>(
        od->FindSubDescriptor(AP4_DESCRIPTOR_TAG_ES_ID_INC));
    if (es_id_inc == NULL) return AP4_ERROR_INVALID_FORMAT;
    od_track_id = es_id_inc->GetTrackId();
    
    // find the track pointed to by the descriptor
    AP4_MoovAtom* moov = dynamic_cast<AP4_MoovAtom*>(top_level.GetChild(AP4_ATOM_TYPE_MOOV));
    if (moov == NULL) return AP4_ERROR_INVALID_FORMAT;
    AP4_TrakAtom* od_trak = NULL;
    AP4_List<AP4_TrakAtom>::Item* trak_item = moov->GetTrakAtoms().FirstItem();
    while (trak_item) {
        AP4_TrakAtom* trak = trak_item->GetData();
        if (trak) {
            if (trak->GetId() == od_track_id) {
                od_trak = trak;
            } else {
                m_SinfEntries.Add(new SinfEntry(trak->GetId(), NULL));
            }
        }
        trak_item = trak_item->GetNext();
    }

    // check that we have found the OD track 
    if (od_trak == NULL) return AP4_ERROR_INVALID_FORMAT;

    // look for the 'mpod' trak references
    AP4_TrefTypeAtom* track_references;
    track_references = dynamic_cast<AP4_TrefTypeAtom*>(od_trak->FindChild("tref/mpod"));
    if (track_references == NULL) return AP4_ERROR_INVALID_FORMAT;

    // create an AP4_Track object from the trak atom and check that it has samples
    AP4_Track* od_track = new AP4_Track(*od_trak, stream, 0);
    if (od_track->GetSampleCount() < 1) {
        delete od_track;
        return AP4_ERROR_INVALID_FORMAT;
    }
    
    // get the first sample (in this version, we only look at a single OD command)
    AP4_Sample od_sample;
    AP4_Result result = od_track->GetSample(0, od_sample);
    if (AP4_FAILED(result)) {
        delete od_track;
        return AP4_ERROR_INVALID_FORMAT;
    }
    
    // adapt the sample data into a byte stream for parsing
    AP4_DataBuffer sample_data;
    od_sample.ReadData(sample_data);
    AP4_MemoryByteStream* sample_stream = new AP4_MemoryByteStream(sample_data);
    
    // look for one ObjectDescriptorUpdate command and 
    // one IPMP_DescriptorUpdate command
    AP4_DescriptorUpdateCommand* od_update = NULL;
    AP4_DescriptorUpdateCommand* ipmp_update = NULL;
    do {
        AP4_Command* command = NULL;
        result = AP4_CommandFactory::CreateCommandFromStream(*sample_stream, command);
        if (AP4_SUCCEEDED(result)) {
            // found a command in the sample, check the type
            switch (command->GetTag()) {
              case AP4_COMMAND_TAG_OBJECT_DESCRIPTOR_UPDATE:
                if (od_update == NULL) {
                    od_update = dynamic_cast<AP4_DescriptorUpdateCommand*>(command);
                }
                break;
                
              case AP4_COMMAND_TAG_IPMP_DESCRIPTOR_UPDATE:
                if (ipmp_update == NULL) {
                    ipmp_update = dynamic_cast<AP4_DescriptorUpdateCommand*>(command);
                }
                break;

              default:
                break;
            }
        }
    } while (AP4_SUCCEEDED(result));
    sample_stream->Release();
    sample_stream = NULL;
    
    // check that we have what we need
    if (od_update == NULL || ipmp_update == NULL) {
        delete od_track;
        return AP4_ERROR_INVALID_FORMAT;
    }
        
    // process all the object descriptors in the od update
    for (AP4_List<AP4_Descriptor>::Item* od_item = od_update->GetDescriptors().FirstItem();
         od_item;
         od_item = od_item->GetNext()) {
        od = dynamic_cast<AP4_ObjectDescriptor*>(od_item->GetData());
        if (od == NULL) continue;

        // find which track this od references
        const AP4_EsIdRefDescriptor* es_id_ref;
        es_id_ref = dynamic_cast<const AP4_EsIdRefDescriptor*>(
            od->FindSubDescriptor(AP4_DESCRIPTOR_TAG_ES_ID_REF));
        if (es_id_ref == NULL || 
            es_id_ref->GetRefIndex() > track_references->GetTrackIds().ItemCount() ||
            es_id_ref->GetRefIndex() == 0) {
            continue;
        }
        AP4_UI32 track_id = track_references->GetTrackIds()[es_id_ref->GetRefIndex()-1];
        SinfEntry* sinf_entry = NULL;
        for (AP4_List<SinfEntry>::Item* sinf_entry_item = m_SinfEntries.FirstItem();
             sinf_entry_item;
             sinf_entry_item = sinf_entry_item->GetNext()) {
            sinf_entry = sinf_entry_item->GetData();
            if (sinf_entry->m_TrackId == track_id) {
                break; // match
            } else {
                sinf_entry = NULL; // no match
            }
        }
        if (sinf_entry == NULL) continue; // no matching entry
        if (sinf_entry->m_Sinf != NULL) continue; // entry already populated
        
        // see what ipmp descriptor this od points to
        const AP4_IpmpDescriptorPointer* ipmpd_pointer;
        ipmpd_pointer = dynamic_cast<const AP4_IpmpDescriptorPointer*>(
            od->FindSubDescriptor(AP4_DESCRIPTOR_TAG_IPMP_DESCRIPTOR_POINTER));
        if (ipmpd_pointer == NULL) continue; // no pointer

        // find the ipmp descriptor referenced by the pointer
        AP4_IpmpDescriptor* ipmpd = NULL;
        for (AP4_List<AP4_Descriptor>::Item* ipmpd_item = ipmp_update->GetDescriptors().FirstItem();
             ipmpd_item;
             ipmpd_item = ipmpd_item->GetNext()) {
            // check that this descriptor is of the right type
            ipmpd = dynamic_cast<AP4_IpmpDescriptor*>(ipmpd_item->GetData());
            if (ipmpd == NULL || ipmpd->GetIpmpsType() != AP4_MARLIN_IPMPS_TYPE_MGSV) continue;
            
            // check the descriptor id
            if (ipmpd->GetDescriptorId() == ipmpd_pointer->GetDescriptorId()) {
                break; // match
            } else {
                ipmpd = NULL; // no match
            }
        }
        if (ipmpd == NULL) continue; // no matching entry
        
        // parse the ipmp data into one or more 'sinf' atoms, and keep the one with the
        // right type
        AP4_MemoryByteStream* data = new AP4_MemoryByteStream(ipmpd->GetData().GetData(),
                                                              ipmpd->GetData().GetDataSize());
        AP4_LargeSize bytes_available = ipmpd->GetData().GetDataSize();
        do {
            AP4_Atom* atom = NULL;
            
            // setup the factory with a context so we can instantiate an 'schm'
            // atom with a slightly different format than the standard 'schm'
            AP4_AtomFactory* factory = &AP4_DefaultAtomFactory::Instance;
            factory->PushContext(AP4_ATOM_TYPE('m','r','l','n'));
            
            // parse the next atom in the stream 
            result = AP4_DefaultAtomFactory::Instance.CreateAtomFromStream(*data, bytes_available, atom);
            factory->PopContext();
            if (AP4_FAILED(result) || atom == NULL) break;
            
            // check that what we have parsed is indeed an 'sinf' of the right type
            if (atom->GetType() == AP4_ATOM_TYPE_SINF) {
                AP4_ContainerAtom* sinf = dynamic_cast<AP4_ContainerAtom*>(atom);
                AP4_SchmAtom* schm = dynamic_cast<AP4_SchmAtom*>(sinf->FindChild("schm"));
                if (schm->GetSchemeType()    == AP4_MARLIN_SCHEME_TYPE_ACBC && 
                    schm->GetSchemeVersion() == 0x0100) {
                    // store the sinf in the entry for that track
                    sinf_entry->m_Sinf = sinf;
                    break;
                }
            }
            delete atom;
        } while (AP4_SUCCEEDED(result));
        data->Release();
        
#if 0 // DEBUG
        if (sinf_entry->m_Sinf) {
            AP4_ByteStream* output =
                new AP4_FileByteStream("-stdout",
                                       AP4_FileByteStream::STREAM_MODE_WRITE);
            AP4_PrintInspector inspector(*output);
            sinf_entry->m_Sinf->Inspect(inspector);
            output->Release();
    }
#endif
    }
    
    // remove the iods atom and the OD track
    od_trak->Detach();
    delete od_trak;
    iods->Detach();
    delete iods;
    
    // cleanup
    delete od_track;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpDecryptingProcessor:CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_MarlinIpmpDecryptingProcessor::CreateTrackHandler(AP4_TrakAtom* trak)
{
    // look for this track in the list of entries
    SinfEntry* sinf_entry = NULL;
    for (AP4_List<SinfEntry>::Item* sinf_entry_item = m_SinfEntries.FirstItem();
         sinf_entry_item;
         sinf_entry_item = sinf_entry_item->GetNext()) {
        sinf_entry = sinf_entry_item->GetData();
        if (sinf_entry->m_TrackId == trak->GetId()) {
            break; // match
        } else {
            sinf_entry = NULL; // no match
        }
    }
    if (sinf_entry == NULL) return NULL; // no matching entry

    // find the key
    const AP4_UI08* key = m_KeyMap.GetKey(sinf_entry->m_TrackId);
    if (key == NULL) return NULL;

    // create the decrypter
    AP4_MarlinIpmpTrackDecrypter* decrypter = NULL;
    AP4_Result result = AP4_MarlinIpmpTrackDecrypter::Create(*m_BlockCipherFactory,
                                                             key, decrypter);
    if (AP4_FAILED(result)) return NULL;
    
    return decrypter;
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpTrackDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_MarlinIpmpTrackDecrypter::Create(AP4_BlockCipherFactory&        cipher_factory,
                                     const AP4_UI08*                key,
                                     AP4_MarlinIpmpTrackDecrypter*& decrypter)
{
    // default value
    decrypter = NULL;
    
    // create a block cipher for the decrypter
    AP4_BlockCipher* block_cipher = NULL;
    AP4_Result result = cipher_factory.Create(AP4_BlockCipher::AES_128,
                                              AP4_BlockCipher::DECRYPT,
                                              key,
                                              AP4_AES_BLOCK_SIZE,
                                              block_cipher);
    if (AP4_FAILED(result)) return result;
    
    // create a CBC cipher
    AP4_CbcStreamCipher* cbc_cipher = new AP4_CbcStreamCipher(block_cipher, AP4_StreamCipher::DECRYPT);
    
    // create the track decrypter
    decrypter = new AP4_MarlinIpmpTrackDecrypter(cbc_cipher);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpTrackDecrypter::~AP4_MarlinIpmpTrackDecrypter
+---------------------------------------------------------------------*/
AP4_MarlinIpmpTrackDecrypter::~AP4_MarlinIpmpTrackDecrypter()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpTrackDecrypter:GetProcessedSampleSize
+---------------------------------------------------------------------*/
AP4_Size 
AP4_MarlinIpmpTrackDecrypter::GetProcessedSampleSize(AP4_Sample& sample)
{
    //AP4_DataBuffer foo;
    //sample.ReadData(foo);
    //FILE* out = fopen("audio.cbc", "w");
    //fwrite(foo.GetData(), 1, foo.GetDataSize(), out);
    //fclose(out);
    
    // with CBC, we need to decrypt the last block to know what the padding was
    AP4_Size       encrypted_size = sample.GetSize()-AP4_AES_BLOCK_SIZE;
    AP4_DataBuffer encrypted;
    AP4_DataBuffer decrypted;
    AP4_Size       decrypted_size = AP4_CIPHER_BLOCK_SIZE;
    if (sample.GetSize() < 2*AP4_CIPHER_BLOCK_SIZE) {
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
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpTrackDecrypter:ProcessSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_MarlinIpmpTrackDecrypter::ProcessSample(AP4_DataBuffer& data_in,
                                            AP4_DataBuffer& data_out)
{
    AP4_Result result;
    
    const AP4_UI08* in = data_in.GetData();
    AP4_Size        in_size = data_in.GetDataSize();

    // default to 0 output 
    data_out.SetDataSize(0);

    // check the selective encryption flag
    if (in_size < 2*AP4_AES_BLOCK_SIZE) return AP4_ERROR_INVALID_FORMAT;

    // process the sample data
    AP4_Size out_size = in_size-AP4_AES_BLOCK_SIZE; // worst case
    data_out.SetDataSize(out_size);
    AP4_UI08* out = data_out.UseData();

    // decrypt the data
    m_Cipher->SetIV(in);
    result = m_Cipher->ProcessBuffer(in+AP4_AES_BLOCK_SIZE, 
                                     in_size-AP4_AES_BLOCK_SIZE, 
                                     out,
                                     &out_size,
                                     true);
    if (AP4_FAILED(result)) return result;
    
    // update the payload size
    data_out.SetDataSize(out_size);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpEncryptingProcessor::AP4_MarlinIpmpEncryptingProcessor
+---------------------------------------------------------------------*/
AP4_MarlinIpmpEncryptingProcessor::AP4_MarlinIpmpEncryptingProcessor(
    AP4_BlockCipherFactory* /*block_cipher_factory*/)
{
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpEncryptingProcessor::Initialize
+---------------------------------------------------------------------*/
AP4_Result 
AP4_MarlinIpmpEncryptingProcessor::Initialize(
    AP4_AtomParent&                  /*top_level*/,
    AP4_ByteStream&                  /*stream*/,
    AP4_Processor::ProgressListener* /*listener = NULL*/)
{
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_MarlinIpmpEncryptingProcessor::CreateTrackHandler
+---------------------------------------------------------------------*/
AP4_Processor::TrackHandler* 
AP4_MarlinIpmpEncryptingProcessor::CreateTrackHandler(AP4_TrakAtom* /*trak*/)
{
    return NULL;
}

