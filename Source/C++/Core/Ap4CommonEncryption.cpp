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
#include "Ap4AvcParser.h"
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_EME_COMMON_SYSTEM_ID[16] = {
    0x10, 0x77, 0xEF, 0xEC, 0xC0, 0xB2, 0x4D, 0x02, 0xAC, 0xE3, 0x3C, 0x1E, 0x52, 0xE2, 0xFB, 0x4B
};

const unsigned int AP4_CENC_NAL_UNIT_ENCRYPTION_MIN_SIZE = 112;

/*----------------------------------------------------------------------
|   AP4_CencSubSampleMapAppend
+---------------------------------------------------------------------*/
static void
AP4_CencSubSampleMapAppendEntry(AP4_Array<AP4_UI16>& bytes_of_cleartext_data,
                                AP4_Array<AP4_UI32>& bytes_of_encrypted_data,
                                unsigned int         cleartext_size,
                                unsigned int         encrypted_size)
{
    // if there's already an entry, attempt to extend the last entry
    if (bytes_of_cleartext_data.ItemCount() > 0) {
        AP4_Cardinal last_index = bytes_of_cleartext_data.ItemCount() - 1;
        if (bytes_of_encrypted_data[last_index] == 0) {
            cleartext_size += bytes_of_cleartext_data[last_index];
            bytes_of_cleartext_data.RemoveLast();
            bytes_of_encrypted_data.RemoveLast();
        }
    }
    
    // append chunks of cleartext size taking into account that the cleartext_size field is only 16 bits
    while (cleartext_size > 0xFFFF) {
        bytes_of_cleartext_data.Append(0xFFFF);
        bytes_of_encrypted_data.Append(0);
        cleartext_size -= 0xFFFF;
    }
    
    // append whatever is left
    bytes_of_cleartext_data.Append(cleartext_size);
    bytes_of_encrypted_data.Append(encrypted_size);
}

/*----------------------------------------------------------------------
|   AP4_CencBasicSubSampleMapper::GetSubSampleMap
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencBasicSubSampleMapper::GetSubSampleMap(AP4_DataBuffer&      sample_data,
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
        bytes_of_cleartext_data.Append((AP4_UI16)cleartext_size);
        bytes_of_encrypted_data.Append((AP4_UI32)(block_count*16));
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencAdvancedSubSampleEncrypter::GetSubSampleMap
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencAdvancedSubSampleMapper::GetSubSampleMap(AP4_DataBuffer&      sample_data,
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

        unsigned int nalu_size = m_NaluLengthSize+nalu_length;
        if (in+nalu_size > in_end) {
            return AP4_ERROR_INVALID_FORMAT;
        }

        // skip encryption if the NAL unit is smaller than the threshold (DECE CFF spec)
        // or should be left unencrypted for this specific format/type
        bool skip = false;
        if (nalu_size < AP4_CENC_NAL_UNIT_ENCRYPTION_MIN_SIZE) {
            skip = true;
        } else if (m_Format == AP4_SAMPLE_FORMAT_AVC1 ||
                   m_Format == AP4_SAMPLE_FORMAT_AVC2 ||
                   m_Format == AP4_SAMPLE_FORMAT_AVC3 ||
                   m_Format == AP4_SAMPLE_FORMAT_AVC4 ||
                   m_Format == AP4_SAMPLE_FORMAT_DVAV ||
                   m_Format == AP4_SAMPLE_FORMAT_DVA1) {
            unsigned int nalu_type = in[m_NaluLengthSize] & 0x1F;
            if (nalu_type != AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_NON_IDR_PICTURE &&
                nalu_type != AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A   &&
                nalu_type != AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B   &&
                nalu_type != AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C   &&
                nalu_type != AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE) {
                // this NAL unit is not a VCL NAL unit
                skip = true;
            }
        } else if (m_Format == AP4_SAMPLE_FORMAT_HEV1 ||
                   m_Format == AP4_SAMPLE_FORMAT_HVC1 ||
                   m_Format == AP4_SAMPLE_FORMAT_DVHE ||
                   m_Format == AP4_SAMPLE_FORMAT_DVH1) {
            unsigned int nalu_type = (in[m_NaluLengthSize] >> 1) & 0x3F;
            if (nalu_type >= 32) {
                // this NAL unit is not a VCL NAL unit
                skip = true;
            }
        }

        const char* cenc_layout = AP4_GlobalOptions::GetString("mpeg-cenc.encryption-layout");
        if (cenc_layout && AP4_CompareStrings(cenc_layout, "nalu-length-and-type-only") == 0) {
            unsigned int cleartext_size = m_NaluLengthSize+1;
            unsigned int encrypted_size = nalu_size > cleartext_size ? nalu_size-cleartext_size : 0;
            AP4_CencSubSampleMapAppendEntry(bytes_of_cleartext_data, bytes_of_encrypted_data, cleartext_size, encrypted_size);
        } else if (skip) {
            // use cleartext regions to cover the entire NAL unit
            AP4_CencSubSampleMapAppendEntry(bytes_of_cleartext_data, bytes_of_encrypted_data, nalu_size, 0);
        } else {
            // leave some cleartext bytes at the start and encrypt the rest (multiple of blocks)
            unsigned int encrypted_size = nalu_size-(AP4_CENC_NAL_UNIT_ENCRYPTION_MIN_SIZE-16);
            encrypted_size -= (encrypted_size % 16);
            unsigned int cleartext_size = nalu_size-encrypted_size;
            AP4_ASSERT(encrypted_size >= 16);
            AP4_ASSERT(cleartext_size >= m_NaluLengthSize);
            AP4_CencSubSampleMapAppendEntry(bytes_of_cleartext_data, bytes_of_encrypted_data, cleartext_size, encrypted_size);
        }
                
        // move the pointers
        in += nalu_size;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencCbcsSubSampleEncrypter::AP4_CencCbcsSubSampleEncrypter
+---------------------------------------------------------------------*/
AP4_CencCbcsSubSampleMapper::AP4_CencCbcsSubSampleMapper(AP4_Size nalu_length_size, AP4_UI32 format, AP4_TrakAtom* trak) :
    AP4_CencSubSampleMapper(nalu_length_size, format),
    m_AvcParser(NULL),
    m_HevcParser(NULL)
{
    if (!trak) return;
    
    // get the sample description atom
    AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));
    if (!stsd) return;
    
    if (format == AP4_SAMPLE_FORMAT_AVC1 ||
        format == AP4_SAMPLE_FORMAT_AVC2 ||
        format == AP4_SAMPLE_FORMAT_AVC3 ||
        format == AP4_SAMPLE_FORMAT_AVC4 ||
        format == AP4_SAMPLE_FORMAT_DVAV ||
        format == AP4_SAMPLE_FORMAT_DVA1) {
        // create the parser
        m_AvcParser = new AP4_AvcFrameParser();
        
        // look for an avc sample description
        AP4_AvccAtom* avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, stsd->FindChild("avc1/avcC"));
        if (!avcc)    avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, stsd->FindChild("avc2/avcC"));
        if (!avcc)    avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, stsd->FindChild("avc3/avcC"));
        if (!avcc)    avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, stsd->FindChild("avc4/avcC"));
        if (!avcc)    return;
        
        // parse the sps and pps if we have them
        AP4_Array<AP4_DataBuffer>& sps_list = avcc->GetSequenceParameters();
        for (unsigned int i=0; i<sps_list.ItemCount(); i++) {
            AP4_DataBuffer& sps = sps_list[i];
            ParseAvcData(sps.GetData(), sps.GetDataSize());
        }
        AP4_Array<AP4_DataBuffer>& pps_list = avcc->GetPictureParameters();
        for (unsigned int i=0; i<pps_list.ItemCount(); i++) {
            AP4_DataBuffer& pps = pps_list[i];
            ParseAvcData(pps.GetData(), pps.GetDataSize());
        }
    } else if (format == AP4_SAMPLE_FORMAT_HEV1 ||
               format == AP4_SAMPLE_FORMAT_HVC1 ||
               format == AP4_SAMPLE_FORMAT_DVHE ||
               format == AP4_SAMPLE_FORMAT_DVH1) {
        // create the parser
        m_HevcParser = new AP4_HevcFrameParser();
        
        // look for an hevc sample description
        AP4_HvccAtom* hvcc = AP4_DYNAMIC_CAST(AP4_HvccAtom, stsd->FindChild("hvc1/hvcC"));
        if (!hvcc)    hvcc = AP4_DYNAMIC_CAST(AP4_HvccAtom, stsd->FindChild("hev1/hvcC"));
        if (!hvcc)    return;
        
        // parse the vps, sps and pps if we have them
        const AP4_Array<AP4_HvccAtom::Sequence>& sequence_list = hvcc->GetSequences();
        for (unsigned int i=0; i<sequence_list.ItemCount(); i++) {
            const AP4_Array<AP4_DataBuffer>& nalus = sequence_list[i].m_Nalus;
            for (unsigned int j=0; j<nalus.ItemCount(); j++) {
                ParseHevcData(nalus[j].GetData(), nalus[j].GetDataSize());
            }
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_CencCbcsSubSampleEncrypter::~AP4_CencCbcsSubSampleEncrypter
+---------------------------------------------------------------------*/
AP4_CencCbcsSubSampleMapper::~AP4_CencCbcsSubSampleMapper()
{
    delete m_AvcParser;
    delete m_HevcParser;
}

/*----------------------------------------------------------------------
|   AP4_CencCbcsSubSampleMapper::ParseAvcData
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencCbcsSubSampleMapper::ParseAvcData(const AP4_UI08* data, AP4_Size data_size)
{
    if (!m_AvcParser) return AP4_ERROR_INVALID_PARAMETERS;
    
    AP4_AvcFrameParser::AccessUnitInfo access_unit_info;
    AP4_Result result = m_AvcParser->Feed(data, data_size, access_unit_info);
    if (AP4_FAILED(result)) return result;
    
    // cleanup
    access_unit_info.Reset();
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencCbcsSubSampleMapper::ParseHevcData
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencCbcsSubSampleMapper::ParseHevcData(const AP4_UI08* data, AP4_Size data_size)
{
    if (!m_HevcParser) return AP4_ERROR_INVALID_PARAMETERS;
    
    AP4_HevcFrameParser::AccessUnitInfo access_unit_info;
    AP4_Result result = m_HevcParser->Feed(data, data_size, access_unit_info);
    if (AP4_FAILED(result)) return result;
    
    // cleanup
    access_unit_info.Reset();
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencCbcsSubSampleMapper::GetSubSampleMap
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencCbcsSubSampleMapper::GetSubSampleMap(AP4_DataBuffer&      sample_data,
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

        unsigned int nalu_size = m_NaluLengthSize+nalu_length;
        if (in+nalu_size > in_end) {
            return AP4_ERROR_INVALID_FORMAT;
        }

        // skip encryption if the NAL unit should be left unencrypted for this specific format/type
        bool skip = false;
        if (m_Format == AP4_SAMPLE_FORMAT_AVC1 ||
            m_Format == AP4_SAMPLE_FORMAT_AVC2 ||
            m_Format == AP4_SAMPLE_FORMAT_AVC3 ||
            m_Format == AP4_SAMPLE_FORMAT_AVC4 ||
            m_Format == AP4_SAMPLE_FORMAT_DVAV ||
            m_Format == AP4_SAMPLE_FORMAT_DVA1) {
            const AP4_UI08* nalu_data = &in[m_NaluLengthSize];
            unsigned int nalu_type = nalu_data[0] & 0x1F;

            if (nalu_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_NON_IDR_PICTURE ||
                nalu_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A   ||
                nalu_type == AP4_AVC_NAL_UNIT_TYPE_CODED_SLICE_OF_IDR_PICTURE) {
                // parse the NAL unit to get the slice header size
                if (m_AvcParser == NULL) return AP4_ERROR_INTERNAL;
                AP4_AvcSliceHeader slice_header;
                unsigned int nalu_ref_idc = (nalu_data[0]>>5)&3;
                AP4_Result result = m_AvcParser->ParseSliceHeader(&nalu_data[1],
                                                                  nalu_length-1,
                                                                  nalu_type,
                                                                  nalu_ref_idc,
                                                                  slice_header);
                if (AP4_FAILED(result)) {
                    return result;
                }

                // round up the slide header size to a multiple of bytes
                unsigned int header_size = (slice_header.size+7)/8;

                // account for emulation prevention bytes
                unsigned int emulation_prevention_bytes = AP4_NalParser::CountEmulationPreventionBytes(&nalu_data[1], nalu_length-1, header_size);
                header_size += emulation_prevention_bytes;

                // leave the slice header in the clear, including the NAL type
                unsigned int cleartext_size = m_NaluLengthSize+1+header_size;
                unsigned int encrypted_size = nalu_size-cleartext_size;
                AP4_CencSubSampleMapAppendEntry(bytes_of_cleartext_data, bytes_of_encrypted_data, cleartext_size, encrypted_size);
            } else {
                // this NAL unit does not have a slice header
                skip = true;

                // parse SPS and PPS NAL units
                if (nalu_type == AP4_AVC_NAL_UNIT_TYPE_SPS ||
                    nalu_type == AP4_AVC_NAL_UNIT_TYPE_PPS) {
                    AP4_Result result = ParseAvcData(nalu_data, nalu_length);
                    if (AP4_FAILED(result)) {
                        return result;
                    }
                }
            }
        } else if (m_Format == AP4_SAMPLE_FORMAT_HEV1 ||
                   m_Format == AP4_SAMPLE_FORMAT_HVC1 ||
                   m_Format == AP4_SAMPLE_FORMAT_DVHE ||
                   m_Format == AP4_SAMPLE_FORMAT_DVH1) {
            const AP4_UI08* nalu_data = &in[m_NaluLengthSize];
            unsigned int nalu_type = (nalu_data[0] >> 1) & 0x3F;
            
            if (nalu_type < AP4_HEVC_NALU_TYPE_VPS_NUT) {
                // this is a VCL NAL Unit
                if (m_HevcParser == NULL) return AP4_ERROR_INTERNAL;
                AP4_HevcSliceSegmentHeader slice_header;
                AP4_Result result = m_HevcParser->ParseSliceSegmentHeader(&nalu_data[2], nalu_length-2, nalu_type, slice_header);
                if (AP4_FAILED(result)) {
                    return result;
                }
                
                // leave the slice header in the clear, including the NAL type
                // NOTE: the slice header is always a multiple of 8 bits because of byte_alignment()
                unsigned int header_size = slice_header.size/8;

                // account for emulation prevention bytes
                unsigned int emulation_prevention_bytes = AP4_NalParser::CountEmulationPreventionBytes(&nalu_data[2], nalu_length-2, header_size);
                header_size += emulation_prevention_bytes;
                
                // set the encrypted range
                unsigned int cleartext_size = m_NaluLengthSize+2+header_size;
                unsigned int encrypted_size = nalu_size-cleartext_size;
                AP4_CencSubSampleMapAppendEntry(bytes_of_cleartext_data, bytes_of_encrypted_data, cleartext_size, encrypted_size);
            } else {
                skip = true;

                // parse VPS, SPS and PPS NAL units
                if (nalu_type == AP4_HEVC_NALU_TYPE_VPS_NUT ||
                    nalu_type == AP4_HEVC_NALU_TYPE_SPS_NUT ||
                    nalu_type == AP4_HEVC_NALU_TYPE_PPS_NUT) {
                    AP4_Result result = ParseHevcData(nalu_data, nalu_length);
                    if (AP4_FAILED(result)) {
                        return result;
                    }
                }
            }
        } else {
            // only AVC and HEVC elementary streams are supported.
            return AP4_ERROR_NOT_SUPPORTED;
        }

        if (skip) {
            // use cleartext regions to cover the entire NAL unit
            AP4_CencSubSampleMapAppendEntry(bytes_of_cleartext_data, bytes_of_encrypted_data, nalu_size, 0);
        }
        
        // move the pointers
        in += nalu_size;
    }
    
    return AP4_SUCCESS;
}

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
    AP4_Result result = m_SubSampleMapper->GetSubSampleMap(data_in, bytes_of_cleartext_data, bytes_of_encrypted_data);
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
    AP4_BytesFromUInt16BE(infos, (AP4_UI16)sample_info_count);
    for (unsigned int i=0; i<sample_info_count; i++) {
        AP4_BytesFromUInt16BE(&infos[2+i*6],   bytes_of_cleartext_data[i]);
        AP4_BytesFromUInt32BE(&infos[2+i*6+2], bytes_of_encrypted_data[i]);
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
        
        if (!m_ConstantIv) {
            // update the IV (last cipherblock emitted)
            AP4_CopyMemory(m_Iv, out-16, 16);
        }
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
    AP4_Result result = m_SubSampleMapper->GetSubSampleMap(data_in, bytes_of_cleartext_data, bytes_of_encrypted_data);
    if (AP4_FAILED(result)) return result;

    for (unsigned int i=0; i<bytes_of_cleartext_data.ItemCount(); i++) {
        // copy the cleartext portion
        AP4_CopyMemory(out, in, bytes_of_cleartext_data[i]);
        
        // encrypt the rest
        if (m_ResetIvForEachSubsample) {
            m_Cipher->SetIV(m_Iv);
        }
        if (bytes_of_encrypted_data[i]) {
            AP4_Size out_size = bytes_of_encrypted_data[i];
            result = m_Cipher->ProcessBuffer(in+bytes_of_cleartext_data[i],
                                             bytes_of_encrypted_data[i],
                                             out+bytes_of_cleartext_data[i],
                                             &out_size, false);
            if (AP4_FAILED(result)) return result;
            
            if (!m_ConstantIv) {
                // update the IV (last cipherblock emitted)
                AP4_CopyMemory(m_Iv, out+bytes_of_cleartext_data[i]+bytes_of_encrypted_data[i]-16, 16);
            }
        }
        
        // move the pointers
        in  += bytes_of_cleartext_data[i]+bytes_of_encrypted_data[i];
        out += bytes_of_cleartext_data[i]+bytes_of_encrypted_data[i];
    }
    
    // encode the sample infos
    unsigned int sample_info_count = bytes_of_cleartext_data.ItemCount();
    sample_infos.SetDataSize(2+sample_info_count*6);
    AP4_UI08* infos = sample_infos.UseData();
    AP4_BytesFromUInt16BE(infos, (AP4_UI16)sample_info_count);
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
                           AP4_UI32                     default_is_protected,
                           AP4_UI08                     default_per_sample_iv_size,
                           const AP4_UI08*              default_kid,
                           AP4_UI08                     default_constant_iv_size,
                           const AP4_UI08*              default_constant_iv,
                           AP4_UI08                     default_crypt_byte_block,
                           AP4_UI08                     default_skip_byte_block,
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
    AP4_UI32                    m_DefaultIsProtected;
    AP4_UI08                    m_DefaultPerSampleIvSize;
    AP4_UI08                    m_DefaultKid[16];
    AP4_UI08                    m_DefaultConstantIvSize;
    AP4_UI08                    m_DefaultConstantIv[16];
    AP4_UI08                    m_DefaultCryptByteBlock;
    AP4_UI08                    m_DefaultSkipByteBlock;
};

/*----------------------------------------------------------------------
|   AP4_CencTrackEncrypter::AP4_CencTrackEncrypter
+---------------------------------------------------------------------*/
AP4_CencTrackEncrypter::AP4_CencTrackEncrypter(
    AP4_CencVariant              variant,
    AP4_UI32                     default_is_protected,
    AP4_UI08                     default_per_sample_iv_size,
    const AP4_UI08*              default_kid,
    AP4_UI08                     default_constant_iv_size,
    const AP4_UI08*              default_constant_iv,
    AP4_UI08                     default_crypt_byte_block,
    AP4_UI08                     default_skip_byte_block,
    AP4_Array<AP4_SampleEntry*>& sample_entries,
    AP4_UI32                     format) :
    m_Variant(variant),
    m_Format(format),
    m_DefaultIsProtected(default_is_protected),
    m_DefaultPerSampleIvSize(default_per_sample_iv_size),
    m_DefaultConstantIvSize(default_constant_iv_size),
    m_DefaultCryptByteBlock(default_crypt_byte_block),
    m_DefaultSkipByteBlock(default_skip_byte_block)
{
    // copy the KID and IV
    AP4_CopyMemory(m_DefaultKid, default_kid, 16);
    if (default_constant_iv) {
        AP4_CopyMemory(m_DefaultConstantIv, default_constant_iv, 16);
    }

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
                tenc = new AP4_PiffTrackEncryptionAtom(m_DefaultIsProtected,
                                                       m_DefaultPerSampleIvSize,
                                                       m_DefaultKid);
                break;
                
            case AP4_CENC_VARIANT_MPEG_CENC:
                schm = new AP4_SchmAtom(AP4_PROTECTION_SCHEME_TYPE_CENC, 
                                        AP4_PROTECTION_SCHEME_VERSION_CENC_10);
                tenc = new AP4_TencAtom(m_DefaultIsProtected,
                                        m_DefaultPerSampleIvSize,
                                        m_DefaultKid);
                break;

            case AP4_CENC_VARIANT_MPEG_CBC1:
                schm = new AP4_SchmAtom(AP4_PROTECTION_SCHEME_TYPE_CBC1,
                                        AP4_PROTECTION_SCHEME_VERSION_CENC_10);
                tenc = new AP4_TencAtom(m_DefaultIsProtected,
                                        m_DefaultPerSampleIvSize,
                                        m_DefaultKid);
                break;

            case AP4_CENC_VARIANT_MPEG_CENS:
                schm = new AP4_SchmAtom(AP4_PROTECTION_SCHEME_TYPE_CENS,
                                        AP4_PROTECTION_SCHEME_VERSION_CENC_10);
                tenc = new AP4_TencAtom(m_DefaultIsProtected,
                                        m_DefaultPerSampleIvSize,
                                        m_DefaultKid,
                                        m_DefaultConstantIvSize,
                                        m_DefaultConstantIv,
                                        m_DefaultCryptByteBlock,
                                        m_DefaultSkipByteBlock);
                break;

            case AP4_CENC_VARIANT_MPEG_CBCS:
                schm = new AP4_SchmAtom(AP4_PROTECTION_SCHEME_TYPE_CBCS,
                                        AP4_PROTECTION_SCHEME_VERSION_CENC_10);
                tenc = new AP4_TencAtom(m_DefaultIsProtected,
                                        m_DefaultPerSampleIvSize,
                                        m_DefaultKid,
                                        m_DefaultConstantIvSize,
                                        m_DefaultConstantIv,
                                        m_DefaultCryptByteBlock,
                                        m_DefaultSkipByteBlock);
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
                              AP4_UI32                                options,
                              AP4_ContainerAtom*                      traf,
                              AP4_CencEncryptingProcessor::Encrypter* encrypter,
                              AP4_UI32                                cleartext_sample_description_index);

    // methods
    virtual AP4_Result ProcessFragment();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);
    virtual AP4_Result PrepareForSamples(AP4_FragmentSampleTable* sample_table);
    virtual AP4_Result FinishFragment();
    
private:
    // members
    AP4_CencVariant                         m_Variant;
    AP4_UI32                                m_Options;
    AP4_ContainerAtom*                      m_Traf;
    AP4_CencSampleEncryption*               m_SampleEncryptionAtom;
    AP4_CencSampleEncryption*               m_SampleEncryptionAtomShadow;
    AP4_SaizAtom*                           m_Saiz;
    AP4_SaioAtom*                           m_Saio;
    AP4_CencEncryptingProcessor::Encrypter* m_Encrypter;
    AP4_UI32                                m_CleartextSampleDescriptionIndex;
};

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter::AP4_CencFragmentEncrypter
+---------------------------------------------------------------------*/
AP4_CencFragmentEncrypter::AP4_CencFragmentEncrypter(AP4_CencVariant                         variant,
                                                     AP4_UI32                                options,
                                                     AP4_ContainerAtom*                      traf,
                                                     AP4_CencEncryptingProcessor::Encrypter* encrypter,
                                                     AP4_UI32                                cleartext_sample_description_index) :
    m_Variant(variant),
    m_Options(options),
    m_Traf(traf),
    m_SampleEncryptionAtom(NULL),
    m_SampleEncryptionAtomShadow(NULL),
    m_Saiz(NULL),
    m_Saio(NULL),
    m_Encrypter(encrypter),
    m_CleartextSampleDescriptionIndex(cleartext_sample_description_index)
{
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter::ProcessFragment
+---------------------------------------------------------------------*/
AP4_Result   
AP4_CencFragmentEncrypter::ProcessFragment()
{
    m_SampleEncryptionAtom       = NULL;
    m_SampleEncryptionAtomShadow = NULL;
    m_Saiz = NULL;
    m_Saio = NULL;

    // set the default-base-is-moof flag
    AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, m_Traf->GetChild(AP4_ATOM_TYPE_TFHD));
    if (tfhd) {
        if (m_Variant != AP4_CENC_VARIANT_PIFF_CBC && m_Variant != AP4_CENC_VARIANT_PIFF_CTR) {
            tfhd->SetFlags(tfhd->GetFlags() | AP4_TFHD_FLAG_DEFAULT_BASE_IS_MOOF);
        }
    }
    
    // if we're still in the cleartext lead, update the tfhd and stop
    if (m_Encrypter->m_CurrentFragment < m_Encrypter->m_CleartextFragments && m_CleartextSampleDescriptionIndex) {
        if (tfhd) {
            tfhd->SetSampleDescriptionIndex(m_CleartextSampleDescriptionIndex);
            tfhd->SetFlags(tfhd->GetFlags() | AP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT);
            tfhd->SetSize(AP4_TfhdAtom::ComputeSize(tfhd->GetFlags()));
            m_Traf->OnChildChanged(tfhd);
        }
        return AP4_SUCCESS;
    }
    
    // create a sample encryption atom
    switch (m_Variant) {
        case AP4_CENC_VARIANT_PIFF_CBC:
            m_SampleEncryptionAtom = new AP4_PiffSampleEncryptionAtom(16);
            break;
            
        case AP4_CENC_VARIANT_PIFF_CTR:
            m_SampleEncryptionAtom = new AP4_PiffSampleEncryptionAtom(8);
            break;
            
        case AP4_CENC_VARIANT_MPEG_CENC:
            if (m_Options & AP4_CencEncryptingProcessor::OPTION_PIFF_COMPATIBILITY) {
                AP4_UI08 iv_size = 8;
                if (m_Options & AP4_CencEncryptingProcessor::OPTION_PIFF_IV_SIZE_16) {
                    iv_size = 16;
                }
                m_SampleEncryptionAtom       = new AP4_SencAtom(iv_size);
                m_SampleEncryptionAtomShadow = new AP4_PiffSampleEncryptionAtom(iv_size);
            } else {
                AP4_UI08 iv_size = 16; // default
                if (m_Options & AP4_CencEncryptingProcessor::OPTION_IV_SIZE_8) {
                    iv_size = 8;
                }
                m_SampleEncryptionAtom = new AP4_SencAtom(iv_size);
            }
            m_Saiz = new AP4_SaizAtom();
            m_Saio = new AP4_SaioAtom();
            break;
            
        case AP4_CENC_VARIANT_MPEG_CBC1:
            m_SampleEncryptionAtom = new AP4_SencAtom(16);
            m_Saiz = new AP4_SaizAtom();
            m_Saio = new AP4_SaioAtom();
            break;

        case AP4_CENC_VARIANT_MPEG_CENS:
            m_SampleEncryptionAtom = new AP4_SencAtom(16, 0, NULL, 0, 0);
            m_Saiz = new AP4_SaizAtom();
            m_Saio = new AP4_SaioAtom();
            break;

        case AP4_CENC_VARIANT_MPEG_CBCS:
            m_SampleEncryptionAtom = new AP4_SencAtom(0, 16, NULL, 0, 0);
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
    if (m_Options & AP4_CencEncryptingProcessor::OPTION_NO_SENC) {
        m_SampleEncryptionAtom->GetOuter().SetType(AP4_ATOM_TYPE('s', 'e', 'n', 'C'));
    }

    // add the child atoms to the traf container
    if (m_Saiz) m_Traf->AddChild(m_Saiz);
    if (m_Saio) m_Traf->AddChild(m_Saio);
    m_Traf->AddChild(&m_SampleEncryptionAtom->GetOuter());
    if (m_SampleEncryptionAtomShadow) {
        m_Traf->AddChild(&m_SampleEncryptionAtomShadow->GetOuter());
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencFragmentEncrypter::PrepareForSamples
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencFragmentEncrypter::PrepareForSamples(AP4_FragmentSampleTable* sample_table) {
    // do nothing if we're still in the clear lead part
    if (m_Encrypter->m_CurrentFragment < m_Encrypter->m_CleartextFragments) {
        return AP4_SUCCESS;
    }

    AP4_Cardinal sample_count = sample_table->GetSampleCount();

    // resize the saio atom if we have one
    if (m_Saio) {
        m_Saio->AddEntry(0); // we'll compute the offset later
    }
    
    if (!m_Encrypter->m_SampleEncrypter->UseSubSamples()) {
        m_SampleEncryptionAtom->SetSampleInfosSize(sample_count*m_SampleEncryptionAtom->GetPerSampleIvSize());
        if (m_SampleEncryptionAtomShadow) {
            m_SampleEncryptionAtomShadow->SetSampleInfosSize(sample_count*m_SampleEncryptionAtomShadow->GetPerSampleIvSize());
        }
        if (m_Saiz) {
            if (m_SampleEncryptionAtom->GetPerSampleIvSize()) {
                m_Saiz->SetDefaultSampleInfoSize(m_SampleEncryptionAtom->GetPerSampleIvSize());
                m_Saiz->SetSampleCount(sample_count);
            } else {
                m_Saiz->SetDefaultSampleInfoSize(0);
                m_Saiz->SetSampleCount(0);
            }
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
    AP4_Size            sample_info_size = sample_count*(m_SampleEncryptionAtom->GetPerSampleIvSize());
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
            m_Saiz->SetSampleInfoSize(i, (AP4_UI08)(m_SampleEncryptionAtom->GetPerSampleIvSize()+2+bytes_of_cleartext_data.ItemCount()*6));
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
    // just copy data if we're still in the clear lead part
    if (m_Encrypter->m_CurrentFragment < m_Encrypter->m_CleartextFragments) {
        data_out.SetData(data_in.GetData(), data_in.GetDataSize());
        return AP4_SUCCESS;
    }
    
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
    if (m_Encrypter->m_CurrentFragment++ < m_Encrypter->m_CleartextFragments) {
        return AP4_SUCCESS;
    }

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
                                                         AP4_UI32                options,
                                                         AP4_BlockCipherFactory* block_cipher_factory) :
    m_Variant(variant),
    m_Options(options)
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
        } else if (m_Variant == AP4_CENC_VARIANT_MPEG_CENC ||
                   m_Variant == AP4_CENC_VARIANT_MPEG_CBC1 ||
                   m_Variant == AP4_CENC_VARIANT_MPEG_CENS ||
                   m_Variant == AP4_CENC_VARIANT_MPEG_CBCS) {
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
        AP4_Array<AP4_UI32> compatible_brands;
        compatible_brands.Append(AP4_FILE_BRAND_ISO6);
        if (m_Variant == AP4_CENC_VARIANT_PIFF_CBC || m_Variant == AP4_CENC_VARIANT_PIFF_CTR) {
            compatible_brands.Append(AP4_PIFF_BRAND);
            compatible_brands.Append(AP4_FTYP_BRAND_MSDH);
        }
        ftyp = new AP4_FtypAtom(AP4_FTYP_BRAND_MP42, 0, &compatible_brands[0], compatible_brands.ItemCount());
    }
    
    // insert the ftyp atom as the first child
    AP4_Result result = top_level.AddChild(ftyp, 0);
    if (AP4_FAILED(result)) return result;
    
    // add pssh atoms if needed
    AP4_ContainerAtom* moov = AP4_DYNAMIC_CAST(AP4_ContainerAtom, top_level.GetChild(AP4_ATOM_TYPE_MOOV));
    if (moov) {
        // create a 'standard EME' pssh atom
        AP4_PsshAtom* eme_pssh = NULL;
        if ((m_Variant == AP4_CENC_VARIANT_MPEG_CENC ||
             m_Variant == AP4_CENC_VARIANT_MPEG_CBC1 ||
             m_Variant == AP4_CENC_VARIANT_MPEG_CENS ||
             m_Variant == AP4_CENC_VARIANT_MPEG_CBCS) &&
            (m_Options & OPTION_EME_PSSH)) {
            AP4_DataBuffer kids;
            AP4_UI32       kid_count = 0;
            const AP4_List<AP4_TrackPropertyMap::Entry>& prop_entries = m_PropertyMap.GetEntries();

            for (unsigned int i=0; i<prop_entries.ItemCount(); i++) {
                AP4_TrackPropertyMap::Entry* entry = NULL;
                prop_entries.Get(i, entry);
                const char* kid_hex = m_PropertyMap.GetProperty(entry->m_TrackId, "KID");
                    
                if (kid_hex && AP4_StringLength(kid_hex) == 32) {
                    AP4_UI08 kid[16];
                    AP4_ParseHex(kid_hex, kid, 16);
                    // only add this entry if there isn't already an entry for this KID
                    bool found = false;
                    for (unsigned int j=0; j<kid_count && !found; j++) {
                        if (AP4_CompareMemory(kid, kids.GetData()+(j*16), 16) == 0) {
                            found = true;
                        }
                    }
                    if (!found) {
                        kids.SetDataSize((kid_count+1)*16);
                        AP4_CopyMemory(kids.UseData()+(kid_count*16), kid, 16);
                        ++kid_count;
                    }
                }
            }
            if (kid_count) {
                eme_pssh = new AP4_PsshAtom(AP4_EME_COMMON_SYSTEM_ID);
                eme_pssh->SetKids(kids.GetData(), kid_count);
            }
        }

        // check if we need to create a Marlin 'mkid' table
        AP4_PsshAtom* marlin_pssh = NULL;
        if (m_Variant == AP4_CENC_VARIANT_MPEG_CENC) {
            const AP4_List<AP4_TrackPropertyMap::Entry>& prop_entries = m_PropertyMap.GetEntries();
            AP4_MkidAtom* mkid = NULL;
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
                        for (unsigned int j=0; j<entries.ItemCount() && !found; j++) {
                            if (AP4_CompareMemory(entries[j].m_KID, kid, 16) == 0) {
                                found = true;
                            }
                        }
                        if (!found) {
                            mkid->AddEntry(kid, entry->m_Value.GetChars());
                        }
                    }
                }
            }

            if (mkid) {
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
                marlin_pssh = new AP4_PsshAtom(AP4_MARLIN_PSSH_SYSTEM_ID);
                marlin_pssh->SetData(*marl);
                if (padding > marl->GetSize()+32 && padding < 1024*1024) {
                    padding -= (AP4_UI32)marl->GetSize()+32;
                    AP4_UI08* data = new AP4_UI08[padding];
                    AP4_SetMemory(data, 0, padding);
                    marlin_pssh->SetPadding(data, padding);
                    delete[] data;
                }
            }
        }

        // add the 'pssh' atoms at the end of the 'moov' container
        // but before any 'free' atom, if any
        int pssh_position = -1;
        int pssh_current = 0;
        for (AP4_List<AP4_Atom>::Item* child = moov->GetChildren().FirstItem();
                                       child;
                                       child = child->GetNext(), ++pssh_current) {
            if (child->GetData()->GetType() == AP4_ATOM_TYPE_FREE) {
                pssh_position = pssh_current;
            }
        }
        if (marlin_pssh) {
            moov->AddChild(marlin_pssh, pssh_position);
            if (pssh_position >= 0) {
                ++pssh_position;
            }
        }
        if (eme_pssh) {
            moov->AddChild(eme_pssh, pssh_position);
            if (pssh_position >= 0) {
                ++pssh_position;
            }
        }
        for (unsigned int i=0; i<m_PsshAtoms.ItemCount(); i++) {
            if (m_PsshAtoms[i]) {
                moov->AddChild(new AP4_PsshAtom(*m_PsshAtoms[i]), pssh_position);
            }
            if (pssh_position >= 0) {
                ++pssh_position;
            }
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
    
    AP4_UI32 format = entries[0]->GetType(); // only look at the type of the first entry
    AP4_UI32 enc_format = 0;
    switch (format) {
        case AP4_ATOM_TYPE_MP4A:
            enc_format = AP4_ATOM_TYPE_ENCA;
            break;

        case AP4_ATOM_TYPE_MP4V:
        case AP4_ATOM_TYPE_AVC1:
        case AP4_ATOM_TYPE_AVC2:
        case AP4_ATOM_TYPE_AVC3:
        case AP4_ATOM_TYPE_AVC4:
        case AP4_ATOM_TYPE_DVAV:
        case AP4_ATOM_TYPE_DVA1:
        case AP4_ATOM_TYPE_HEV1:
        case AP4_ATOM_TYPE_HVC1:
        case AP4_ATOM_TYPE_DVHE:
        case AP4_ATOM_TYPE_DVH1:
            enc_format = AP4_ATOM_TYPE_ENCV;
            break;
            
        default: {
            // try to find if this is audio or video
            AP4_HdlrAtom* hdlr = AP4_DYNAMIC_CAST(AP4_HdlrAtom, trak->FindChild("mdia/hdlr"));
            if (hdlr) {
                switch (hdlr->GetHandlerType()) {
                    case AP4_HANDLER_TYPE_SOUN:
                        enc_format = AP4_ATOM_TYPE_ENCA;
                        break;

                    case AP4_HANDLER_TYPE_VIDE:
                        enc_format = AP4_ATOM_TYPE_ENCV;
                        break;
                }
            }
            break;
        }
    }
    if (enc_format == 0) return NULL;
         
    // get the track properties
    AP4_UI08 kid[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    const char* kid_hex = m_PropertyMap.GetProperty(trak->GetId(), "KID");
    if (kid_hex && AP4_StringLength(kid_hex) == 32) {
        AP4_ParseHex(kid_hex, kid, 16);
    }
        
    // create the encrypter
    AP4_Processor::TrackHandler* track_encrypter;
    AP4_UI08                     cipher_iv_size = 16;
    AP4_BlockCipher::CipherMode  cipher_mode;
    AP4_BlockCipher::CtrParams   cipher_ctr_params;
    const void*                  cipher_mode_params = NULL;
    AP4_UI08                     crypt_byte_block = 0;
    AP4_UI08                     skip_byte_block = 0;
    bool                         constant_iv = false;
    bool                         reset_iv_at_each_subsample = false;
    switch (m_Variant) {
        case AP4_CENC_VARIANT_PIFF_CTR:
            cipher_mode = AP4_BlockCipher::CTR;
            cipher_ctr_params.counter_size = 8;
            cipher_mode_params = &cipher_ctr_params;
            cipher_iv_size = 8;
            track_encrypter = new AP4_CencTrackEncrypter(m_Variant,
                                                         1,
                                                         cipher_iv_size,
                                                         kid,
                                                         0,
                                                         NULL,
                                                         0,
                                                         0,
                                                         entries,
                                                         enc_format);
            break;
            
        case AP4_CENC_VARIANT_PIFF_CBC:
            cipher_mode = AP4_BlockCipher::CBC;
            track_encrypter = new AP4_CencTrackEncrypter(m_Variant,
                                                         2,
                                                         cipher_iv_size,
                                                         kid,
                                                         0,
                                                         NULL,
                                                         0,
                                                         0,
                                                         entries,
                                                         enc_format);
            break;
            
        case AP4_CENC_VARIANT_MPEG_CENC:
            cipher_mode = AP4_BlockCipher::CTR;
            cipher_ctr_params.counter_size = 8;
            cipher_mode_params = &cipher_ctr_params;
            if ((m_Options & OPTION_IV_SIZE_8) ||
                ((m_Options & OPTION_PIFF_COMPATIBILITY) && !(m_Options & OPTION_PIFF_IV_SIZE_16))) {
                cipher_iv_size = 8;
            }

            track_encrypter = new AP4_CencTrackEncrypter(m_Variant,
                                                         1,
                                                         cipher_iv_size,
                                                         kid,
                                                         0,
                                                         NULL,
                                                         0,
                                                         0,
                                                         entries, 
                                                         enc_format);
            break;
            
        case AP4_CENC_VARIANT_MPEG_CENS:
            cipher_mode = AP4_BlockCipher::CTR;
            cipher_ctr_params.counter_size = 8;
            cipher_mode_params = &cipher_ctr_params;
            if (m_Options & OPTION_IV_SIZE_8) {
                cipher_iv_size = 8;
            }
            if (enc_format == AP4_ATOM_TYPE_ENCV) {
                crypt_byte_block = 1;
                skip_byte_block  = 9;
            }
            track_encrypter = new AP4_CencTrackEncrypter(m_Variant,
                                                         1,
                                                         cipher_iv_size,
                                                         kid,
                                                         0,
                                                         NULL,
                                                         crypt_byte_block,
                                                         skip_byte_block,
                                                         entries, 
                                                         enc_format);
            break;

        case AP4_CENC_VARIANT_MPEG_CBC1:
            cipher_mode = AP4_BlockCipher::CBC;
            track_encrypter = new AP4_CencTrackEncrypter(m_Variant,
                                                         1,
                                                         cipher_iv_size,
                                                         kid,
                                                         0,
                                                         NULL,
                                                         0,
                                                         0,
                                                         entries, 
                                                         enc_format);
            break;

        case AP4_CENC_VARIANT_MPEG_CBCS:
            cipher_mode = AP4_BlockCipher::CBC;
            if (enc_format == AP4_ATOM_TYPE_ENCV) {
                crypt_byte_block = 1;
                skip_byte_block  = 9;
            }
            constant_iv = true;
            reset_iv_at_each_subsample = true;
            track_encrypter = new AP4_CencTrackEncrypter(m_Variant,
                                                         1,
                                                         0,
                                                         kid,
                                                         cipher_iv_size,
                                                         iv->GetData(),
                                                         crypt_byte_block,
                                                         skip_byte_block,
                                                         entries, 
                                                         enc_format);
            break;

        default:
            return NULL;
    }
    
    // create a block cipher
    AP4_BlockCipher* block_cipher = NULL;
    AP4_Result result = m_BlockCipherFactory->CreateCipher(AP4_BlockCipher::AES_128,
                                                           AP4_BlockCipher::ENCRYPT, 
                                                           cipher_mode,
                                                           cipher_mode_params,
                                                           key->GetData(), 
                                                           key->GetDataSize(), 
                                                           block_cipher);
    if (AP4_FAILED(result)) {
        delete track_encrypter;
        return NULL;
    }
    
    // compute the size of NAL units
    unsigned int nalu_length_size = 0;
    if (format == AP4_ATOM_TYPE_AVC1 ||
        format == AP4_ATOM_TYPE_AVC2 ||
        format == AP4_ATOM_TYPE_AVC3 ||
        format == AP4_ATOM_TYPE_AVC4 ||
        format == AP4_ATOM_TYPE_DVAV ||
        format == AP4_ATOM_TYPE_DVA1) {
        AP4_AvccAtom* avcc = AP4_DYNAMIC_CAST(AP4_AvccAtom, entries[0]->GetChild(AP4_ATOM_TYPE_AVCC));
        if (avcc) {
            nalu_length_size = avcc->GetNaluLengthSize();
        }
    } else if (format == AP4_ATOM_TYPE_HEV1 ||
               format == AP4_ATOM_TYPE_HVC1 ||
               format == AP4_ATOM_TYPE_DVHE ||
               format == AP4_ATOM_TYPE_DVH1) {
        AP4_HvccAtom* hvcc = AP4_DYNAMIC_CAST(AP4_HvccAtom, entries[0]->GetChild(AP4_ATOM_TYPE_HVCC));
        if (hvcc) {
            nalu_length_size = hvcc->GetNaluLengthSize();
        }
    }

    // add a new cipher state for this track
    AP4_CencSampleEncrypter* sample_encrypter = NULL;
    AP4_StreamCipher*        stream_cipher = NULL;
    switch (cipher_mode) {
        case AP4_BlockCipher::CBC:
            stream_cipher = new AP4_CbcStreamCipher(block_cipher);
            if (crypt_byte_block && skip_byte_block) {
                stream_cipher = new AP4_PatternStreamCipher(stream_cipher, crypt_byte_block, skip_byte_block);
            }

            if (nalu_length_size) {
                AP4_CencSubSampleMapper* subsample_mapper = NULL;
                if (m_Variant == AP4_CENC_VARIANT_MPEG_CBCS) {
                    subsample_mapper = new AP4_CencCbcsSubSampleMapper(nalu_length_size, format, trak);
                } else {
                    subsample_mapper = new AP4_CencBasicSubSampleMapper(nalu_length_size, format);
                }
                sample_encrypter = new AP4_CencCbcSubSampleEncrypter(stream_cipher,
                                                                     constant_iv,
                                                                     reset_iv_at_each_subsample,
                                                                     subsample_mapper);
            } else {
                sample_encrypter = new AP4_CencCbcSampleEncrypter(stream_cipher, constant_iv);
            }

            break;
            
        case AP4_BlockCipher::CTR:
            stream_cipher = new AP4_CtrStreamCipher(block_cipher, 16);
            if (crypt_byte_block && skip_byte_block) {
                stream_cipher = new AP4_PatternStreamCipher(stream_cipher, crypt_byte_block, skip_byte_block);
            }
            if (nalu_length_size) {
                AP4_CencSubSampleMapper* subsample_mapper = new AP4_CencAdvancedSubSampleMapper(nalu_length_size, format);
                sample_encrypter = new AP4_CencCtrSubSampleEncrypter(stream_cipher,
                                                                     constant_iv,
                                                                     reset_iv_at_each_subsample,
                                                                     cipher_iv_size,
                                                                     subsample_mapper);
            } else {
                sample_encrypter = new AP4_CencCtrSampleEncrypter(stream_cipher, constant_iv, cipher_iv_size);
            }
            break;
    }
    if (sample_encrypter == NULL) {
        delete stream_cipher;
        delete block_cipher;
        delete track_encrypter;
        return NULL;
    }
    sample_encrypter->SetIv(iv->GetData());

    // if we need to leave some samples unencrypted, create clones of the sample descriptions
    const char* clear_lead = m_PropertyMap.GetProperty(trak->GetId(), "ClearLeadFragments");
    AP4_UI32 clear_fragments = 0;
    if (clear_lead) {
        clear_fragments = AP4_ParseIntegerU(clear_lead);
        unsigned int sample_description_count = stsd->GetSampleDescriptionCount();
        for (unsigned int i=0; i<sample_description_count; i++) {
            stsd->AddChild(stsd->GetSampleEntry(i)->Clone());
        }
    }
    
    m_Encrypters.Add(new Encrypter(trak->GetId(), clear_fragments, sample_encrypter));
    return track_encrypter;
}

/*----------------------------------------------------------------------
|   AP4_CencEncryptingProcessor:CreateFragmentHandler
+---------------------------------------------------------------------*/
AP4_Processor::FragmentHandler* 
AP4_CencEncryptingProcessor::CreateFragmentHandler(AP4_TrakAtom*      trak,
                                                   AP4_TrexAtom*      trex,
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
    
    AP4_UI32 clear_sample_description_index = 0;
    const char* clear_lead = m_PropertyMap.GetProperty(trak->GetId(), "ClearLeadFragments");
    if (clear_lead) {
        if (encrypter->m_CurrentFragment < encrypter->m_CleartextFragments) {
            // find the stsd atom
            AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));
            if (stsd) {
                AP4_UI32 tfhd_flags = tfhd->GetFlags();
                AP4_UI32 sample_description_index = 0;
                if (tfhd_flags & AP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT) {
                    sample_description_index = tfhd->GetSampleDescriptionIndex();
                } else {
                    sample_description_index = trex->GetDefaultSampleDescriptionIndex();
                }
                if (sample_description_index > 0) {
                    clear_sample_description_index = sample_description_index+stsd->GetSampleDescriptionCount()/2;
                }
            }
        }
    }
    return new AP4_CencFragmentEncrypter(m_Variant, m_Options, traf, encrypter, clear_sample_description_index);
}

/*----------------------------------------------------------------------
|   AP4_CencSingleSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencSingleSampleDecrypter::Create(AP4_UI32                        cipher_type,
                                      const AP4_UI08*                 key,
                                      AP4_Size                        key_size,
                                      AP4_UI08                        crypt_byte_block,
                                      AP4_UI08                        skip_byte_block,
                                      AP4_BlockCipherFactory*         block_cipher_factory,
                                      bool                            reset_iv_at_each_subsample,
                                      AP4_CencSingleSampleDecrypter*& decrypter)
{
    // check the parameters
    if (key == NULL) return AP4_ERROR_INVALID_PARAMETERS;
    
    // use the default cipher factory if  none was passed
    if (block_cipher_factory == NULL) {
        block_cipher_factory = &AP4_DefaultBlockCipherFactory::Instance;
    }
    
    // create the cipher
    AP4_StreamCipher* stream_cipher    = NULL;
    bool              full_blocks_only = false;
    switch (cipher_type) {
        case AP4_CENC_CIPHER_NONE:
            break;
            
        case AP4_CENC_CIPHER_AES_128_CTR: {
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
            
        case AP4_CENC_CIPHER_AES_128_CBC: {
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

    // wrap with a pattern processor if needed
    if (crypt_byte_block && skip_byte_block) {
        stream_cipher = new AP4_PatternStreamCipher(stream_cipher, crypt_byte_block, skip_byte_block);
    }
    
    // create the decrypter
    decrypter = new AP4_CencSingleSampleDecrypter(stream_cipher, full_blocks_only, reset_iv_at_each_subsample);

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
                if (m_ResetIvAtEachSubsample) {
                    m_Cipher->SetIV(iv);
                }
                
                AP4_Result result = m_Cipher->ProcessBuffer(in+cleartext_size, encrypted_size, out+cleartext_size, &encrypted_size, false);
                if (AP4_FAILED(result)) return result;
            }
            
            // move the pointers and udate counters
            in  += cleartext_size+encrypted_size;
            out += cleartext_size+encrypted_size;
        }

        // copy any leftover partial block
        unsigned int partial = (unsigned int)(in_end-in);
        if (partial) {
            AP4_CopyMemory(out, in, partial);
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
            AP4_Result result = m_Cipher->ProcessBuffer(in, encrypted_size, out, &encrypted_size, true);
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
    AP4_UI32                 protection_sheme  = 0;
    bool                     reset_iv_at_each_subsample = false;
    AP4_Result result = AP4_CencSampleInfoTable::Create(sample_description,
                                                        traf,
                                                        saio,
                                                        saiz,
                                                        sample_encryption_atom,
                                                        protection_sheme,
                                                        reset_iv_at_each_subsample,
                                                        aux_info_data,
                                                        aux_info_data_offset,
                                                        sample_info_table);
    if (AP4_FAILED(result)) {
        return result;
    }

    // we now have all the info we need to create the decrypter
    return Create(sample_info_table,
                  protection_sheme,
                  key,
                  key_size,
                  block_cipher_factory,
                  reset_iv_at_each_subsample,
                  decrypter);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleDecrypter::Create
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencSampleDecrypter::Create(AP4_CencSampleInfoTable*  sample_info_table,
                                AP4_UI32                  cipher_type,
                                const AP4_UI08*           key, 
                                AP4_Size                  key_size,
                                AP4_BlockCipherFactory*   block_cipher_factory,
                                bool                      reset_iv_at_each_subsample,
                                AP4_CencSampleDecrypter*& decrypter)
{
    // default return value
    decrypter = NULL;
    
    // check some basic paramaters
    unsigned int iv_size = sample_info_table->GetIvSize();
    switch (cipher_type) {
        case AP4_CENC_CIPHER_NONE:
            break;
            
        case AP4_CENC_CIPHER_AES_128_CTR:
            if (iv_size != 8 && iv_size != 16) {
                return AP4_ERROR_INVALID_FORMAT;
            }
            break;
            
        case AP4_CENC_CIPHER_AES_128_CBC:
            if (iv_size != 16) {
                return AP4_ERROR_INVALID_FORMAT;
            }
            break;
            
        default:
            return AP4_ERROR_NOT_SUPPORTED;
    }

    // create a single-sample decrypter
    AP4_CencSingleSampleDecrypter* single_sample_decrypter = NULL;
    AP4_Result result = AP4_CencSingleSampleDecrypter::Create(cipher_type,
                                                              key,
                                                              key_size,
                                                              sample_info_table->GetCryptByteBlock(),
                                                              sample_info_table->GetSkipByteBlock(),
                                                              block_cipher_factory,
                                                              reset_iv_at_each_subsample,
                                                              single_sample_decrypter);
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

    // get the subsample info for this sample if needed
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
    AP4_CencFragmentDecrypter(AP4_CencSampleDecrypter*  sample_decrypter, // ownership is transfered
                              AP4_SaioAtom*             saio_atom,
                              AP4_SaizAtom*             saiz_atom,
                              AP4_CencSampleEncryption* sample_encryption_atom) :
    m_SampleDecrypter(sample_decrypter),
    m_SaioAtom(saio_atom),
    m_SaizAtom(saiz_atom),
    m_SampleEncryptionAtom(sample_encryption_atom) {}

    ~AP4_CencFragmentDecrypter() { delete m_SampleDecrypter; }

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
|   AP4_CencDecryptingProcessor:GetKeyForTrak
+---------------------------------------------------------------------*/
const AP4_DataBuffer*
AP4_CencDecryptingProcessor::GetKeyForTrak(AP4_UI32 track_id, AP4_ProtectedSampleDescription* sample_description)
{
    // look for the key by track ID
    const AP4_DataBuffer* key = m_KeyMap->GetKey(track_id);
    if (!key) {
        // no key found by track ID, look for a key by KID
        if (sample_description) {
            AP4_ProtectionSchemeInfo* scheme_info = sample_description->GetSchemeInfo();
            if (scheme_info) {
                AP4_ContainerAtom* schi = scheme_info->GetSchiAtom();
                if (schi) {
                    AP4_TencAtom* tenc = AP4_DYNAMIC_CAST(AP4_TencAtom, schi->FindChild("tenc"));
                    if (tenc) {
                        const AP4_UI08* kid = tenc->GetDefaultKid();
                        if (kid) {
                            key = m_KeyMap->GetKeyByKid(kid);
                        }
                    }
                }
            }
        }
    }

    return key;
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
                protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CENC ||
                protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CBC1 ||
                protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CENS ||
                protected_desc->GetSchemeType() == AP4_PROTECTION_SCHEME_TYPE_CBCS) {
                sample_descs.Append(protected_desc);
                sample_entries.Append(sample_entry);
            }
        }
    }
    if (sample_entries.ItemCount() == 0) return NULL;
    
    // get the key for this track
    const AP4_DataBuffer* key = GetKeyForTrak(trak->GetId(), sample_descs.ItemCount() ? sample_descs[0] : NULL);

    // create a decrypter with this key
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
AP4_CencDecryptingProcessor::CreateFragmentHandler(AP4_TrakAtom*      trak,
                                                   AP4_TrexAtom*      trex,
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

                // get the key for this track
                key = GetKeyForTrak(tfhd->GetTrackId(), sample_description);
            }
            
            break;
        }
    }
    if (sample_description == NULL || key == NULL) return NULL;
    
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
        m_BlockCipherFactory,
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
AP4_CencTrackEncryption::AP4_CencTrackEncryption(AP4_UI08 version) :
    m_Version_(version),
    m_DefaultIsProtected(0),
    m_DefaultPerSampleIvSize(0),
    m_DefaultConstantIvSize(0),
    m_DefaultCryptByteBlock(0),
    m_DefaultSkipByteBlock(0)
{
    AP4_SetMemory(m_DefaultKid, 0, 16);
    AP4_SetMemory(m_DefaultConstantIv,  0, 16);
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption::AP4_CencTrackEncryption
+---------------------------------------------------------------------*/
AP4_CencTrackEncryption::AP4_CencTrackEncryption(AP4_UI08        version,
                                                 AP4_UI08        default_is_protected,
                                                 AP4_UI08        default_per_sample_iv_size,
                                                 const AP4_UI08* default_kid,
                                                 AP4_UI08        default_constant_iv_size,
                                                 const AP4_UI08* default_constant_iv,
                                                 AP4_UI08        default_crypt_byte_block,
                                                 AP4_UI08        default_skip_byte_block) :
    m_Version_(version),
    m_DefaultIsProtected(default_is_protected),
    m_DefaultPerSampleIvSize(default_per_sample_iv_size),
    m_DefaultConstantIvSize(default_constant_iv_size),
    m_DefaultCryptByteBlock(default_crypt_byte_block),
    m_DefaultSkipByteBlock(default_skip_byte_block)
{
    AP4_CopyMemory(m_DefaultKid, default_kid, 16);
    AP4_SetMemory(m_DefaultConstantIv, 0, 16);
    if (default_per_sample_iv_size == 0 && default_constant_iv_size && default_constant_iv) {
        if (default_constant_iv_size > 16) {
            // too large, truncate
            default_constant_iv_size = 16;
        }
        AP4_CopyMemory(&m_DefaultConstantIv[16-default_constant_iv_size], default_constant_iv, default_constant_iv_size);
    }
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption::Parse
+---------------------------------------------------------------------*/
AP4_Result
AP4_CencTrackEncryption::Parse(AP4_ByteStream& stream)
{
    AP4_UI08 reserved;
    AP4_Result result = stream.ReadUI08(reserved);
    if (AP4_FAILED(result)) return result;
    if (m_Version_ == 0) {
        result = stream.ReadUI08(reserved);
        if (AP4_FAILED(result)) return result;
    } else {
        AP4_UI08 blocks;
        result = stream.ReadUI08(blocks);
        if (AP4_FAILED(result)) return result;
        m_DefaultCryptByteBlock = (blocks >> 4) & 0xF;
        m_DefaultSkipByteBlock  = (blocks     ) & 0xF;
    }
    result = stream.ReadUI08(m_DefaultIsProtected);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI08(m_DefaultPerSampleIvSize);
    if (AP4_FAILED(result)) return result;
    AP4_SetMemory(m_DefaultKid, 0, 16);
    result = stream.Read(m_DefaultKid, 16);
    if (AP4_FAILED(result)) return result;
    if (m_DefaultPerSampleIvSize == 0) {
        result = stream.ReadUI08(m_DefaultConstantIvSize);
        if (AP4_FAILED(result)) return result;
        if (m_DefaultConstantIvSize > 16) {
            m_DefaultConstantIvSize = 0;
            return AP4_ERROR_INVALID_FORMAT;
        }
        AP4_SetMemory(m_DefaultConstantIv, 0, 16);
        result = stream.Read(m_DefaultConstantIv, m_DefaultConstantIvSize);
        if (AP4_FAILED(result)) return result;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CencTrackEncryption::DoInspectFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_CencTrackEncryption::DoInspectFields(AP4_AtomInspector& inspector)
{
    // the spelling of these fields is inconsistent, but that's how it appears in the spec!
    inspector.AddField("default_isProtected",        m_DefaultIsProtected);
    inspector.AddField("default_Per_Sample_IV_Size", m_DefaultPerSampleIvSize);
    inspector.AddField("default_KID",                m_DefaultKid, 16);
    if (m_Version_ >= 1) {
        inspector.AddField("default_crypt_byte_block", m_DefaultCryptByteBlock);
        inspector.AddField("default_skip_byte_block", m_DefaultSkipByteBlock);
    }
    if (m_DefaultPerSampleIvSize == 0) {
        inspector.AddField("default_constant_IV_size", m_DefaultConstantIvSize);
        if (m_DefaultConstantIvSize <= 16) {
            inspector.AddField("default_constant_IV", m_DefaultConstantIv, m_DefaultConstantIvSize);
        }
    }
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
    result = stream.WriteUI08(0); // reserved
    if (AP4_FAILED(result)) return result;
    if (m_Version_ == 0) {
        result = stream.WriteUI08(0); // reserved
        if (AP4_FAILED(result)) return result;
    } else {
        result = stream.WriteUI08(m_DefaultCryptByteBlock<<4 | m_DefaultSkipByteBlock);
        if (AP4_FAILED(result)) return result;
    }
    result = stream.WriteUI08(m_DefaultIsProtected);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(m_DefaultPerSampleIvSize);
    if (AP4_FAILED(result)) return result;
    result = stream.Write(m_DefaultKid, 16);
    if (AP4_FAILED(result)) return result;
    if (m_DefaultPerSampleIvSize == 0) {
        result = stream.WriteUI08(m_DefaultConstantIvSize);
        if (AP4_FAILED(result)) return result;
        result = stream.Write(m_DefaultConstantIv, m_DefaultConstantIvSize <= 16 ? m_DefaultConstantIvSize : 16);
        if (AP4_FAILED(result)) return result;
    }
    
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
                                AP4_UI32&                       cipher_type,
                                bool&                           reset_iv_at_each_subsample,
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
                  cipher_type,
                  reset_iv_at_each_subsample,
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
                                AP4_UI32&                       cipher_type,
                                bool&                           reset_iv_at_each_subsample,
                                AP4_ByteStream&                 aux_info_data,
                                AP4_Position                    aux_info_data_offset,
                                AP4_CencSampleInfoTable*&       sample_info_table)
{
    // default return values
    saio                       = NULL;
    saiz                       = NULL;
    sample_encryption_atom     = NULL;
    sample_info_table          = NULL;
    cipher_type                = AP4_CENC_CIPHER_NONE;
    reset_iv_at_each_subsample = false;
    
    // get the scheme info atom
    AP4_ContainerAtom* schi = sample_description->GetSchemeInfo()->GetSchiAtom();
    if (schi == NULL) return AP4_ERROR_INVALID_FORMAT;

    // look for a track encryption atom
    AP4_CencTrackEncryption* track_encryption_atom = 
        AP4_DYNAMIC_CAST(AP4_CencTrackEncryption, schi->GetChild(AP4_ATOM_TYPE_TENC));
    if (track_encryption_atom == NULL) {
        track_encryption_atom = AP4_DYNAMIC_CAST(AP4_CencTrackEncryption, schi->GetChild(AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM));
    }
    if (track_encryption_atom == NULL) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    
    // look for a sample encryption atom
    if (traf) {
        sample_encryption_atom = AP4_DYNAMIC_CAST(AP4_SencAtom, traf->GetChild(AP4_ATOM_TYPE_SENC));
        if (sample_encryption_atom == NULL) {
            sample_encryption_atom = AP4_DYNAMIC_CAST(AP4_PiffSampleEncryptionAtom, traf->GetChild(AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM));
        }
    }
    
    // check the scheme and compute the cipher
    switch (sample_description->GetSchemeType()) {
      case AP4_PROTECTION_SCHEME_TYPE_PIFF:
        switch (track_encryption_atom->GetDefaultIsProtected()) {
          case 0:
            cipher_type = AP4_CENC_CIPHER_NONE;
            break;
            
          case 1:
            cipher_type = AP4_CENC_CIPHER_AES_128_CTR;
            break;
          case 2:
            cipher_type = AP4_CENC_CIPHER_AES_128_CBC;
            break;

          default:
            return AP4_ERROR_NOT_SUPPORTED;
        }
        break;
        
      case AP4_PROTECTION_SCHEME_TYPE_CENC:
      case AP4_PROTECTION_SCHEME_TYPE_CENS:
        cipher_type = AP4_CENC_CIPHER_AES_128_CTR;
        break;

      case AP4_PROTECTION_SCHEME_TYPE_CBC1:
        cipher_type = AP4_CENC_CIPHER_AES_128_CBC;
        break;

      case AP4_PROTECTION_SCHEME_TYPE_CBCS:
        cipher_type = AP4_CENC_CIPHER_AES_128_CBC;
        reset_iv_at_each_subsample = true;
        break;
        
      default:
        // not supported
        return AP4_ERROR_NOT_SUPPORTED;
    }
    if (!track_encryption_atom->GetDefaultIsProtected()) {
        cipher_type = AP4_CENC_CIPHER_NONE;
    }
    
    // parse the crypto params
    AP4_UI08        per_sample_iv_size = 0;
    AP4_UI08        constant_iv_size   = 0;
    const AP4_UI08* constant_iv        = NULL;
    AP4_UI08        crypt_byte_block   = 0;
    AP4_UI08        skip_byte_block    = 0;
    if (sample_encryption_atom &&
        (sample_encryption_atom->GetOuter().GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS)) {
        
        switch (sample_encryption_atom->GetAlgorithmId()) {
          case 0:
            cipher_type = AP4_CENC_CIPHER_NONE;
            break;
            
          case 1:
            cipher_type = AP4_CENC_CIPHER_AES_128_CTR;
            break;
            
          case 2:
            cipher_type = AP4_CENC_CIPHER_AES_128_CBC;
            break;
        }
        per_sample_iv_size = sample_encryption_atom->GetPerSampleIvSize();
    } else {
        per_sample_iv_size = track_encryption_atom->GetDefaultPerSampleIvSize();
        constant_iv_size   = track_encryption_atom->GetDefaultConstantIvSize();
        crypt_byte_block   = track_encryption_atom->GetDefaultCryptByteBlock();
        skip_byte_block    = track_encryption_atom->GetDefaultSkipByteBlock();
        if (constant_iv_size) {
            constant_iv = track_encryption_atom->GetDefaultConstantIv();
        }
    }

    // try to create a sample info table from saio/saiz
    if (sample_info_table == NULL && traf) {
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
        if (saio && saiz) {
            AP4_Result result = Create(0,
                                       crypt_byte_block,
                                       skip_byte_block,
                                       per_sample_iv_size,
                                       constant_iv_size,
                                       constant_iv,
                                       *traf,
                                       *saio, 
                                       *saiz,
                                       aux_info_data,
                                       aux_info_data_offset, 
                                       sample_info_table);
            // only abort the processing if the error is not due to an invalid
            // format: some files have an invalid saio/saiz based construction,
            // but may still have a valid `senc`, which we can try to parse
            // below
            if (!AP4_SUCCEEDED(result) && result != AP4_ERROR_INVALID_FORMAT) {
                return result;
            }
        }
    }
    
    // try to create a sample info table from senc
    if (sample_info_table == NULL && sample_encryption_atom) {
        AP4_Result result = sample_encryption_atom->CreateSampleInfoTable(0,
                                                                          crypt_byte_block,
                                                                          skip_byte_block,
                                                                          per_sample_iv_size,
                                                                          constant_iv_size,
                                                                          constant_iv,
                                                                          sample_info_table);
        if (AP4_FAILED(result)) return result;
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
AP4_CencSampleInfoTable::Create(AP4_UI08                  flags,
                                AP4_UI08                  crypt_byte_block,
                                AP4_UI08                  skip_byte_block,
                                AP4_UI08                  per_sample_iv_size,
                                AP4_UI08                  constant_iv_size,
                                const AP4_UI08*           constant_iv,
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
    AP4_UI08 iv_size = per_sample_iv_size;
    if (iv_size == 0) {
        iv_size = constant_iv_size;
        if (iv_size == 0 || constant_iv == NULL) {
            return AP4_ERROR_INVALID_PARAMETERS;
        }
    }
    AP4_CencSampleInfoTable* table = new AP4_CencSampleInfoTable(flags,
                                                                 crypt_byte_block,
                                                                 skip_byte_block,
                                                                 sample_info_count,
                                                                 iv_size);
    
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
                if (per_sample_iv_size) {
                    if (per_sample_iv_size > info_size) {
                        result = AP4_ERROR_INVALID_FORMAT;
                        goto end;
                    }
                    table->SetIv(saiz_index, info_data);
                } else {
                    table->SetIv(saiz_index, constant_iv);
                }
                if (info_size < per_sample_iv_size+2) {
                    result = AP4_ERROR_INVALID_FORMAT;
                    goto end;
                }
                AP4_UI16 subsample_count = AP4_BytesToUInt16BE(info_data+per_sample_iv_size);
                if (info_size < per_sample_iv_size+2+subsample_count*6) {
                    // not enough data
                    result = AP4_ERROR_INVALID_FORMAT;
                    goto end;
                }
                table->AddSubSampleData(subsample_count, info_data+per_sample_iv_size+2);
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
    
    // basic size check
    if (serialized_size < 4+4) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    
    AP4_UI32 sample_count     = AP4_BytesToUInt32BE(serialized); serialized += 4; serialized_size -= 4;
    AP4_UI08 flags            = serialized[0]; serialized += 1; serialized_size -= 1;
    AP4_UI08 crypt_byte_block = serialized[0]; serialized += 1; serialized_size -= 1;
    AP4_UI08 skip_byte_block  = serialized[0]; serialized += 1; serialized_size -= 1;
    AP4_UI08 iv_size          = serialized[0]; serialized += 1; serialized_size -= 1;
    
    if (serialized_size < sample_count*iv_size) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    AP4_CencSampleInfoTable* table = new AP4_CencSampleInfoTable(flags, crypt_byte_block, skip_byte_block, sample_count, (AP4_UI08)iv_size);
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
AP4_CencSampleInfoTable::AP4_CencSampleInfoTable(AP4_UI08 flags,
                                                 AP4_UI08 crypt_byte_block,
                                                 AP4_UI08 skip_byte_block,
                                                 AP4_UI32 sample_count,
                                                 AP4_UI08 iv_size) :
    m_SampleCount(sample_count),
    m_Flags(flags),
    m_CryptByteBlock(crypt_byte_block),
    m_SkipByteBlock(skip_byte_block),
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
    
    AP4_BytesFromUInt32BE(data, m_SampleCount); data += 4;
    *data++ = m_Flags;
    *data++ = m_CryptByteBlock;
    *data++ = m_SkipByteBlock;
    *data++ = m_IvSize;
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
    m_Outer(outer),
    m_ConstantIvSize(0),
    m_CryptByteBlock(0),
    m_SkipByteBlock(0),
    m_SampleInfoCursor(0)
{
    AP4_SetMemory(m_ConstantIv, 0, 16);
    
    if (outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        stream.ReadUI24(m_AlgorithmId);
        stream.ReadUI08(m_PerSampleIvSize);
        stream.Read    (m_Kid, 16);
    } else {
        m_AlgorithmId     = 0;
        m_PerSampleIvSize = 0;
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
AP4_CencSampleEncryption::AP4_CencSampleEncryption(AP4_Atom&       outer,
                                                   AP4_UI08        per_sample_iv_size,
                                                   AP4_UI08        constant_iv_size,
                                                   const AP4_UI08* constant_iv,
                                                   AP4_UI08        crypt_byte_block,
                                                   AP4_UI08        skip_byte_block) :
    m_Outer(outer),
    m_AlgorithmId(0),
    m_PerSampleIvSize(per_sample_iv_size),
    m_ConstantIvSize(constant_iv_size),
    m_CryptByteBlock(crypt_byte_block),
    m_SkipByteBlock(skip_byte_block),
    m_SampleInfoCount(0),
    m_SampleInfoCursor(0)
{
    AP4_SetMemory(m_ConstantIv, 0, 16);
    if (constant_iv_size <= 16 && constant_iv) {
        AP4_CopyMemory(m_ConstantIv, constant_iv, m_ConstantIvSize);
    }
    AP4_SetMemory(m_Kid, 0, 16);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::AP4_CencSampleEncryption
+---------------------------------------------------------------------*/
AP4_CencSampleEncryption::AP4_CencSampleEncryption(AP4_Atom&       outer,
                                                   AP4_UI32        algorithm_id,
                                                   AP4_UI08        per_sample_iv_size,
                                                   const AP4_UI08* kid) :
    m_Outer(outer),
    m_AlgorithmId(algorithm_id),
    m_PerSampleIvSize(per_sample_iv_size),
    m_ConstantIvSize(0),
    m_CryptByteBlock(0),
    m_SkipByteBlock(0),
    m_SampleInfoCount(0),
    m_SampleInfoCursor(0)
{
    AP4_SetMemory(m_ConstantIv, 0, 16);
    AP4_CopyMemory(m_Kid, kid, 16);
}

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryption::AddSampleInfo
+---------------------------------------------------------------------*/
AP4_Result      
AP4_CencSampleEncryption::AddSampleInfo(const AP4_UI08* iv,
                                        AP4_DataBuffer& subsample_info)
{
    unsigned int added_size = m_PerSampleIvSize+subsample_info.GetDataSize();
    
    if (m_SampleInfoCursor+added_size > m_SampleInfos.GetDataSize()) {
        // too much data!
        return AP4_ERROR_OUT_OF_RANGE;
    }
    AP4_UI08* info = m_SampleInfos.UseData()+m_SampleInfoCursor;
    if (m_PerSampleIvSize) {
        AP4_CopyMemory(info, iv, m_PerSampleIvSize);
    }
    if (subsample_info.GetDataSize()) {
        AP4_CopyMemory(info+m_PerSampleIvSize, subsample_info.GetData(), subsample_info.GetDataSize());
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
AP4_CencSampleEncryption::CreateSampleInfoTable(AP4_UI08                  flags,
                                                AP4_UI08                  default_crypt_byte_block,
                                                AP4_UI08                  default_skip_byte_block,
                                                AP4_UI08                  default_per_sample_iv_size,
                                                AP4_UI08                  default_constant_iv_size,
                                                const AP4_UI08*           default_constant_iv,
                                                AP4_CencSampleInfoTable*& table)
{
    // default return value
    table = NULL;
    
    // check if some values are overridden
    AP4_Size per_sample_iv_size = default_per_sample_iv_size;
    if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS) {
        per_sample_iv_size = m_PerSampleIvSize;
    } 
    bool has_subsamples = false;
    if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
        has_subsamples = true;
    }
    
    // check some of the parameters
    if (per_sample_iv_size == 0) {
        if (default_constant_iv_size == 0 || default_constant_iv == NULL) {
            return AP4_ERROR_INVALID_PARAMETERS;
        }
    }

    // create the table
    AP4_Result result = AP4_ERROR_INVALID_FORMAT;
    table = new AP4_CencSampleInfoTable(flags,
                                        default_crypt_byte_block,
                                        default_skip_byte_block,
                                        m_SampleInfoCount,
                                        per_sample_iv_size?per_sample_iv_size:default_constant_iv_size);
    const AP4_UI08* data      = m_SampleInfos.GetData();
    AP4_UI32        data_size = m_SampleInfos.GetDataSize();
    for (unsigned int i=0; i<m_SampleInfoCount; i++) {
        if (per_sample_iv_size) {
            if (data_size < per_sample_iv_size) goto end;
            table->SetIv(i, data);
            data      += per_sample_iv_size;
            data_size -= per_sample_iv_size;
        } else {
            table->SetIv(i, default_constant_iv);
        }
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
        inspector.AddField("IV_size",     m_PerSampleIvSize);
        inspector.AddField("KID",         m_Kid, 16);
    }
    
    inspector.AddField("sample info count", m_SampleInfoCount);

    if (inspector.GetVerbosity() < 2) {
        return AP4_SUCCESS;
    }
    
    // since we don't know the IV size necessarily (we don't have the context), we
    // will try to guess the IV size (we'll try 16 and 8)
    unsigned int iv_size = m_PerSampleIvSize;
    if (iv_size == 0) {
        if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
            bool data_ok = false;
            for (unsigned int k=0; k<=2 && !data_ok; k++) {
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

    inspector.StartArray("sample info entries", m_SampleInfoCount);
    const AP4_UI08* info = m_SampleInfos.GetData();
    for (unsigned int i=0; i<m_SampleInfoCount; i++) {
        inspector.StartObject(NULL);
        inspector.AddField("info", info, iv_size);
        info += iv_size;
        if (m_Outer.GetFlags() & AP4_CENC_SAMPLE_ENCRYPTION_FLAG_USE_SUB_SAMPLE_ENCRYPTION) {
            unsigned int num_entries = AP4_BytesToInt16BE(info);
            info += 2;
            inspector.StartArray("sub entries", num_entries);
            for (unsigned int j=0; j<num_entries; j++) {
                inspector.StartObject(NULL, 2, true);
                unsigned int bocd = AP4_BytesToUInt16BE(info);
                inspector.AddField("bytes_of_clear_data", bocd);
                unsigned int boed = AP4_BytesToUInt32BE(info+2);
                inspector.AddField("bytes_of_encrypted_data", boed);
                info += 6;
                inspector.EndObject();
            }
            inspector.EndArray();
        }
        inspector.EndObject();
    }
    inspector.EndArray();

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
        result = stream.WriteUI08(m_PerSampleIvSize);
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

/*----------------------------------------------------------------------
|   AP4_CencSampleEncryptionInformationGroupEntry::AP4_CencSampleEncryptionInformationGroupEntry
+---------------------------------------------------------------------*/
AP4_CencSampleEncryptionInformationGroupEntry::AP4_CencSampleEncryptionInformationGroupEntry(const AP4_UI08* data)
{
    m_IsEncrypted = ((data[2] & 1) != 0);
    m_IvSize      = data[3];
    AP4_CopyMemory(m_KID, data+4, 16);
}
