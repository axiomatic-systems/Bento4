/*****************************************************************
|
|    AP4 - Atom Factory
|
|    Copyright 2002-2012 Axiomatic Systems, LLC
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
#include "Ap4Types.h"
#include "Ap4Utils.h"
#include "Ap4AtomFactory.h"
#include "Ap4SampleEntry.h"
#include "Ap4UuidAtom.h"
#include "Ap4IsmaCryp.h"
#include "Ap4UrlAtom.h"
#include "Ap4MoovAtom.h"
#include "Ap4MvhdAtom.h"
#include "Ap4MehdAtom.h"
#include "Ap4MfhdAtom.h"
#include "Ap4TfhdAtom.h"
#include "Ap4TrunAtom.h"
#include "Ap4TrakAtom.h"
#include "Ap4HdlrAtom.h"
#include "Ap4DrefAtom.h"
#include "Ap4TkhdAtom.h"
#include "Ap4TrexAtom.h"
#include "Ap4TfhdAtom.h"
#include "Ap4MdhdAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4StscAtom.h"
#include "Ap4StcoAtom.h"
#include "Ap4Co64Atom.h"
#include "Ap4StszAtom.h"
#include "Ap4Stz2Atom.h"
#include "Ap4IodsAtom.h"
#include "Ap4EsdsAtom.h"
#include "Ap4SttsAtom.h"
#include "Ap4CttsAtom.h"
#include "Ap4StssAtom.h"
#include "Ap4FtypAtom.h"
#include "Ap4VmhdAtom.h"
#include "Ap4SmhdAtom.h"
#include "Ap4NmhdAtom.h"
#include "Ap4SthdAtom.h"
#include "Ap4HmhdAtom.h"
#include "Ap4ElstAtom.h"
#include "Ap4SchmAtom.h"
#include "Ap4FrmaAtom.h"
#include "Ap4TimsAtom.h"
#include "Ap4RtpAtom.h"
#include "Ap4SdpAtom.h"
#include "Ap4IkmsAtom.h"
#include "Ap4IsfmAtom.h"
#include "Ap4IsltAtom.h"
#include "Ap4OdheAtom.h"
#include "Ap4OhdrAtom.h"
#include "Ap4OddaAtom.h"
#include "Ap4TrefTypeAtom.h"
#include "Ap4MetaData.h"
#include "Ap4IproAtom.h"
#include "Ap4OdafAtom.h"
#include "Ap4GrpiAtom.h"
#include "Ap4AvccAtom.h"
#include "Ap4HvccAtom.h"
#include "Ap4DvccAtom.h"
#include "Ap4VpccAtom.h"
#include "Ap4Av1cAtom.h"
#include "Ap4Marlin.h"
#include "Ap48bdlAtom.h"
#include "Ap4Piff.h"
#include "Ap4TfraAtom.h"
#include "Ap4MfroAtom.h"
#include "Ap4TfdtAtom.h"
#include "Ap4TencAtom.h"
#include "Ap4SencAtom.h"
#include "Ap4SaioAtom.h"
#include "Ap4SaizAtom.h"
#include "Ap4PdinAtom.h"
#include "Ap4BlocAtom.h"
#include "Ap4AinfAtom.h"
#include "Ap4PsshAtom.h"
#include "Ap4Dac3Atom.h"
#include "Ap4Dec3Atom.h"
#include "Ap4Dac4Atom.h"
#include "Ap4SidxAtom.h"
#include "Ap4SbgpAtom.h"
#include "Ap4SgpdAtom.h"
#include "Ap4ColrAtom.h"
#include "Ap4GamaAtom.h"
#include "Ap4PaspAtom.h"
#include "Ap4FielAtom.h"

/*----------------------------------------------------------------------
|   AP4_AtomFactory::~AP4_AtomFactory
+---------------------------------------------------------------------*/
AP4_AtomFactory::~AP4_AtomFactory()
{
    m_TypeHandlers.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::AddTypeHandler
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::AddTypeHandler(TypeHandler* handler)
{
    return m_TypeHandlers.Add(handler);
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::RemoveTypeHandler
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::RemoveTypeHandler(TypeHandler* handler)
{
    return m_TypeHandlers.Remove(handler);
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::CreateAtomFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomFromStream(AP4_ByteStream& stream, 
                                      AP4_Atom*&      atom)
{
    AP4_LargeSize stream_size     = 0;
    AP4_Position  stream_position = 0;
    AP4_LargeSize bytes_available = (AP4_LargeSize)(-1);
    if (AP4_SUCCEEDED(stream.GetSize(stream_size)) && 
        stream_size != 0 &&
        AP4_SUCCEEDED(stream.Tell(stream_position)) &&
        stream_position <= stream_size) {
        bytes_available = stream_size-stream_position;
    }
    return CreateAtomFromStream(stream, bytes_available, atom);
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::CreateAtomFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomFromStream(AP4_ByteStream& stream, 
                                      AP4_LargeSize&  bytes_available,
                                      AP4_Atom*&      atom)
{
    AP4_Result result;

    // NULL by default
    atom = NULL;

    // check that there are enough bytes for at least a header
    if (bytes_available < 8) return AP4_ERROR_EOS;

    // remember current stream offset
    AP4_Position start;
    stream.Tell(start);

    // read atom size
    AP4_UI32 size_32;
    result = stream.ReadUI32(size_32);
    if (AP4_FAILED(result)) {
        stream.Seek(start);
        return result;
    }
    AP4_UI64 size = size_32;

    // read atom type
    AP4_Atom::Type type;
    result = stream.ReadUI32(type);
    if (AP4_FAILED(result)) {
        stream.Seek(start);
        return result;
    }

    // handle special size values
    bool atom_is_large = false;
    bool force_64      = false;
    if (size == 0) {
        // atom extends to end of file
        AP4_LargeSize stream_size = 0;
        stream.GetSize(stream_size);
        if (stream_size >= start) {
            size = stream_size - start;

            if (size <= 0xFFFFFFFF) {
                size_32 = (AP4_UI32)size;
            } else {
                size_32 = 1; // signal a large atom
            }
        }
    } else if (size == 1) {
        // 64-bit size
        atom_is_large = true;
        if (bytes_available < 16) {
            stream.Seek(start);
            return AP4_ERROR_INVALID_FORMAT;
        }
        stream.ReadUI64(size);
        if (size < 16) {
            stream.Seek(start);
            return AP4_ERROR_INVALID_FORMAT;
        }
        if (size <= 0xFFFFFFFF) {
            force_64 = true;
        }
    }

    // check the size
    if ((size > 0 && size < 8) || size > bytes_available) {
        stream.Seek(start);
        return AP4_ERROR_INVALID_FORMAT;
    }

    // create the atom
    result = CreateAtomFromStream(stream, type, size_32, size, atom);
    if (AP4_FAILED(result)) return result;
    
    // if we failed to create an atom, use a generic version
    if (atom == NULL) {
        unsigned int payload_offset = 8;
        if (atom_is_large) payload_offset += 8;
        stream.Seek(start+payload_offset);
        atom = new AP4_UnknownAtom(type, size, stream);
    }

    // special case: if the atom is poorly encoded and has a 64-bit
    // size header but an actual size that fits on 32-bit, adjust the
    // object to reflect that.
    if (force_64) {
        atom->SetSize32(1);
        atom->SetSize64(size);
    }

    // adjust the available size
    bytes_available -= size;

    // skip to the end of the atom
    result = stream.Seek(start+size);
    if (AP4_FAILED(result)) {
        delete atom;
        atom = NULL;
        return result;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::CreateAtomFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomFromStream(AP4_ByteStream& stream, 
                                      AP4_UI32        type,
                                      AP4_UI32        size_32,
                                      AP4_UI64        size_64,
                                      AP4_Atom*&      atom)
{
    bool atom_is_large = (size_32 == 1);
    bool force_64 = (size_32 == 1 && ((size_64 >> 32) == 0));
    
    // create the atom
    if (GetContext() == AP4_ATOM_TYPE_STSD) {
        // sample entry
        if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
        switch (type) {
          case AP4_ATOM_TYPE_MP4A:
            atom = new AP4_Mp4aSampleEntry(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_MP4V:
            atom = new AP4_Mp4vSampleEntry(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_MP4S:
            atom = new AP4_Mp4sSampleEntry(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_ENCA:
            atom = new AP4_EncaSampleEntry(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_ENCV:
            atom = new AP4_EncvSampleEntry(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_DRMS:
            atom = new AP4_DrmsSampleEntry(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_DRMI:
            atom = new AP4_DrmiSampleEntry(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_AVC1:
          case AP4_ATOM_TYPE_AVC2:
          case AP4_ATOM_TYPE_AVC3:
          case AP4_ATOM_TYPE_AVC4:
          case AP4_ATOM_TYPE_DVAV:
          case AP4_ATOM_TYPE_DVA1:
            atom = new AP4_AvcSampleEntry(type, size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_HEV1:
          case AP4_ATOM_TYPE_HVC1:
          case AP4_ATOM_TYPE_DVHE:
          case AP4_ATOM_TYPE_DVH1:
            atom = new AP4_HevcSampleEntry(type, size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_AV01:
            atom = new AP4_Av1SampleEntry(type, size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_AC_3:
            atom = new AP4_Ac3SampleEntry(type, size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_EC_3:
            atom = new AP4_Eac3SampleEntry(type, size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_AC_4:
            atom = new AP4_Ac4SampleEntry(type, size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_ALAC:
          case AP4_ATOM_TYPE_DTSC:
          case AP4_ATOM_TYPE_DTSH:
          case AP4_ATOM_TYPE_DTSL:
          case AP4_ATOM_TYPE_DTSE:
          case AP4_ATOM_TYPE_FLAC:
          case AP4_ATOM_TYPE_OPUS:
            atom = new AP4_AudioSampleEntry(type, size_32, stream, *this);
            break;
            
          // ProRes
          case AP4_ATOM_TYPE_AP4H:
          case AP4_ATOM_TYPE_AP4X:
          case AP4_ATOM_TYPE_APCH:
          case AP4_ATOM_TYPE_APCN:
          case AP4_ATOM_TYPE_APCS:
          case AP4_ATOM_TYPE_APCO:
          // mjpeg
          case AP4_ATOM_TYPE_JPEG:
          // Avid DNxHD / DNxHR
          case AP4_ATOM_TYPE_AVDN:
          case AP4_ATOM_TYPE_AVDH:
          // VP8/9/10
          case AP4_ATOM_TYPE_VP08:
          case AP4_ATOM_TYPE_VP09:
          case AP4_ATOM_TYPE_VP10:
            atom = new AP4_VisualSampleEntry(type, size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_RTP_:
            atom = new AP4_RtpHintSampleEntry(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_STPP:
            atom = new AP4_SubtitleSampleEntry(type, size_32, stream, *this);
            break;

          default: {
            // try all the external type handlers
            AP4_List<TypeHandler>::Item* handler_item = m_TypeHandlers.FirstItem();
            while (handler_item) {
                TypeHandler* handler = handler_item->GetData();
                if (AP4_SUCCEEDED(handler->CreateAtom(type, size_32, stream, GetContext(), atom))) {
                    break;
                }
                handler_item = handler_item->GetNext();
            }

            // no custom handler, create a generic entry
            if (atom == NULL) {
                atom = new AP4_UnknownSampleEntry(type, (AP4_UI32)size_64, stream);
            }

            break;
          }
        }
    } else {
        // regular atom
        switch (type) {
          case AP4_ATOM_TYPE_MOOV:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_MoovAtom::Create(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_MVHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_MvhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_MEHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_MehdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_MFHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_MfhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_TRAK:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TrakAtom::Create(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_TREX:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TrexAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_HDLR:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_HdlrAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_TKHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TkhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_TFHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TfhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_TRUN:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TrunAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_TFRA:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TfraAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_MFRO:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_MfroAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_MDHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_MdhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_STSD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_StsdAtom::Create(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_STSC:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_StscAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_STCO:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_StcoAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_CO64:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_Co64Atom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_STSZ:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_StszAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_STZ2:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_Stz2Atom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_STTS:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SttsAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_CTTS:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_CttsAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_STSS:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_StssAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_IODS:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_IodsAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_ESDS:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_EsdsAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_AVCC:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_AvccAtom::Create(size_32, stream);
            break;
            
          case AP4_ATOM_TYPE_HVCC:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_HvccAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_DVCC:
          case AP4_ATOM_TYPE_DVVC:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_DvccAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_VPCC:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_VpccAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_AV1C:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_Av1cAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_HVCE:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_HvccAtom::Create(size_32, stream);
            if (atom) {
                atom->SetType(AP4_ATOM_TYPE_HVCE);
            }
            break;

          case AP4_ATOM_TYPE_AVCE:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_AvccAtom::Create(size_32, stream);
            if (atom) {
                atom->SetType(AP4_ATOM_TYPE_AVCE);
            }
            break;

    #if !defined(AP4_CONFIG_MINI_BUILD)
          case AP4_ATOM_TYPE_UUID: {
              AP4_UI08 uuid[16];
              AP4_Result result = stream.Read(uuid, 16);
              if (AP4_FAILED(result)) return result;
              
              if (AP4_CompareMemory(uuid, AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM, 16) == 0) {
                  atom = AP4_PiffTrackEncryptionAtom::Create((AP4_UI32)size_64, stream);
              } else if (AP4_CompareMemory(uuid, AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM, 16) == 0) {
                  atom = AP4_PiffSampleEncryptionAtom::Create((AP4_UI32)size_64, stream);
              } else {
                  atom = new AP4_UnknownUuidAtom(size_64, uuid, stream);
              }
              break;
          }
            
          case AP4_ATOM_TYPE_8ID_:
            atom = new AP4_NullTerminatedStringAtom(type, size_64, stream);
            break;

          case AP4_ATOM_TYPE_8BDL:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_8bdlAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_DREF:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_DrefAtom::Create(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_URL:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_UrlAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_ELST:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_ElstAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_VMHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_VmhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_SMHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SmhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_NMHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_NmhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_STHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SthdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_HMHD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_HmhdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_FRMA:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_FrmaAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_SCHM:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SchmAtom::Create(size_32, &m_ContextStack, stream);
            break;

          case AP4_ATOM_TYPE_FTYP:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_FtypAtom::Create(size_32, stream);
            break;
                        
          case AP4_ATOM_TYPE_TIMS:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TimsAtom::Create(size_32, stream);
            break;
     
          case AP4_ATOM_TYPE_SDP_:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SdpAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_IKMS:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_IkmsAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_ISFM:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_IsfmAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_ISLT:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_IsltAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_ODHE:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_OdheAtom::Create(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_OHDR:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_OhdrAtom::Create(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_ODDA:
            atom = AP4_OddaAtom::Create(size_64, stream);
            break;

          case AP4_ATOM_TYPE_ODAF:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_OdafAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_GRPI:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_GrpiAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_IPRO:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_IproAtom::Create(size_32, stream, *this);
            break;

          case AP4_ATOM_TYPE_RTP_:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_RtpAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_TFDT:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TfdtAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_TENC:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_TencAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_SENC:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SencAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_SAIZ:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SaizAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_SAIO:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SaioAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_PDIN:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_PdinAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_BLOC:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_BlocAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_AINF:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_AinfAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_PSSH:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_PsshAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_SIDX:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SidxAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_SBGP:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SbgpAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_SGPD:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_SgpdAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_MKID:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            if (GetContext() == AP4_ATOM_TYPE_MARL) {
                atom = AP4_MkidAtom::Create(size_32, stream);
            }
            break;

          case AP4_ATOM_TYPE_DAC3:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            if (GetContext() == AP4_ATOM_TYPE_AC_3 || GetContext() == AP4_ATOM_TYPE_ENCA) {
                atom = AP4_Dac3Atom::Create(size_32, stream);
            }
            break;

          case AP4_ATOM_TYPE_DEC3:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            if (GetContext() == AP4_ATOM_TYPE_EC_3 || GetContext() == AP4_ATOM_TYPE_ENCA) {
                atom = AP4_Dec3Atom::Create(size_32, stream);
            }
            break;

          case AP4_ATOM_TYPE_DAC4:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            if (GetContext() == AP4_ATOM_TYPE_AC_4 || GetContext() == AP4_ATOM_TYPE_ENCA) {
                atom = AP4_Dac4Atom::Create(size_32, stream);
            }
            break;

          case AP4_ATOM_TYPE_COLR:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_ColrAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_GAMA:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_GamaAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_PASP:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_PaspAtom::Create(size_32, stream);
            break;

          case AP4_ATOM_TYPE_FIEL:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_FielAtom::Create(size_32, stream);
            break;

          // track ref types
          case AP4_ATOM_TYPE_HINT:
          case AP4_ATOM_TYPE_CDSC:
          case AP4_ATOM_TYPE_SYNC:
          case AP4_ATOM_TYPE_MPOD:
          case AP4_ATOM_TYPE_DPND:
          case AP4_ATOM_TYPE_IPIR:
          case AP4_ATOM_TYPE_ALIS:
          case AP4_ATOM_TYPE_CHAP:
            if (GetContext() == AP4_ATOM_TYPE_TREF) {
                if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
                atom = AP4_TrefTypeAtom::Create(type, size_32, stream);
            }
            break;

    #endif // AP4_CONFIG_MINI_BUILD

          // container atoms
          case AP4_ATOM_TYPE_MOOF:
          case AP4_ATOM_TYPE_MVEX:
          case AP4_ATOM_TYPE_TRAF:
          case AP4_ATOM_TYPE_TREF:
          case AP4_ATOM_TYPE_MFRA:
          case AP4_ATOM_TYPE_HNTI:
          case AP4_ATOM_TYPE_STBL:
          case AP4_ATOM_TYPE_MDIA:
          case AP4_ATOM_TYPE_DINF:
          case AP4_ATOM_TYPE_MINF:
          case AP4_ATOM_TYPE_SCHI:
          case AP4_ATOM_TYPE_SINF:
          case AP4_ATOM_TYPE_UDTA:
          case AP4_ATOM_TYPE_ILST:
          case AP4_ATOM_TYPE_EDTS: 
          case AP4_ATOM_TYPE_MDRI:
          case AP4_ATOM_TYPE_WAVE:
            if (atom_is_large) return AP4_ERROR_INVALID_FORMAT;
            atom = AP4_ContainerAtom::Create(type, size_64, false, force_64, stream, *this);
            break;

          // containers, only at the top
          case AP4_ATOM_TYPE_MARL:
            if (GetContext() == 0) {
                atom = AP4_ContainerAtom::Create(type, size_64, false, force_64, stream, *this);
            }
            break;
            
          // full container atoms
          case AP4_ATOM_TYPE_META:
          case AP4_ATOM_TYPE_ODRM:
          case AP4_ATOM_TYPE_ODKM:
            atom = AP4_ContainerAtom::Create(type, size_64, true, force_64, stream, *this);
            break;

          case AP4_ATOM_TYPE_FREE:
          case AP4_ATOM_TYPE_WIDE:
          case AP4_ATOM_TYPE_MDAT:
            // generic atoms
            break;
            
          default: {
            // try all the external type handlers
            AP4_List<TypeHandler>::Item* handler_item = m_TypeHandlers.FirstItem();
            while (handler_item) {
                TypeHandler* handler = handler_item->GetData();
                if (AP4_SUCCEEDED(handler->CreateAtom(type, size_32, stream, GetContext(), atom))) {
                    break;
                }
                handler_item = handler_item->GetNext();
            }

            break;
          }
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::CreateAtomsFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomsFromStream(AP4_ByteStream& stream, 
                                       AP4_AtomParent& atoms)
{
    AP4_LargeSize stream_size     = 0;
    AP4_Position  stream_position = 0;
    AP4_LargeSize bytes_available = (AP4_LargeSize)(-1);
    if (AP4_SUCCEEDED(stream.GetSize(stream_size)) && 
        stream_size != 0 &&
        AP4_SUCCEEDED(stream.Tell(stream_position)) &&
        stream_position <= stream_size) {
        bytes_available = stream_size-stream_position;
    }
    return CreateAtomsFromStream(stream, bytes_available, atoms);
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::CreateAtomsFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomsFromStream(AP4_ByteStream& stream, 
                                       AP4_LargeSize   bytes_available,
                                       AP4_AtomParent& atoms)
{
    AP4_Result result;
    do {
        AP4_Atom* atom = NULL;
        result = CreateAtomFromStream(stream, bytes_available, atom);
        if (AP4_SUCCEEDED(result) && atom != NULL) {
            atoms.AddChild(atom);
        }
    } while (AP4_SUCCEEDED(result));
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::PushContext
+---------------------------------------------------------------------*/
void
AP4_AtomFactory::PushContext(AP4_Atom::Type context) 
{
    m_ContextStack.Append(context);
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::PopContext
+---------------------------------------------------------------------*/
void
AP4_AtomFactory::PopContext() 
{
    m_ContextStack.RemoveLast();
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::GetContext
+---------------------------------------------------------------------*/
AP4_Atom::Type
AP4_AtomFactory::GetContext(AP4_Ordinal depth) 
{
    AP4_Ordinal available = m_ContextStack.ItemCount();
    if (depth >= available) return 0;
    return m_ContextStack[available-depth-1];
}

/*----------------------------------------------------------------------
|   AP4_DefaultAtomFactory::Instance
+---------------------------------------------------------------------*/
AP4_DefaultAtomFactory AP4_DefaultAtomFactory::Instance_;

/*----------------------------------------------------------------------
|   AP4_DefaultAtomFactory::Instance
+---------------------------------------------------------------------*/
AP4_DefaultAtomFactory::AP4_DefaultAtomFactory()
{
    Initialize();
}

/*----------------------------------------------------------------------
|   AP4_DefaultAtomFactory::Initialize
+---------------------------------------------------------------------*/
AP4_Result
AP4_DefaultAtomFactory::Initialize()
{
    // register built-in type handlers
    AP4_Result result = AddTypeHandler(new AP4_MetaDataAtomTypeHandler(this));
    if (AP4_SUCCEEDED(result)) m_Initialized = true;
    return result;
}
