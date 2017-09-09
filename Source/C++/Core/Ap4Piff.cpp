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
#include "Ap4Piff.h"
#include "Ap4CommonEncryption.h"

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
|   AP4_PiffTrackEncryptionAtom Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PiffTrackEncryptionAtom)

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::Create
+---------------------------------------------------------------------*/
AP4_PiffTrackEncryptionAtom* 
AP4_PiffTrackEncryptionAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version = 0;
    AP4_UI32 flags   = 0;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    AP4_Result result = ReadFullHeader(stream, version, flags);
    if (AP4_FAILED(result)) return NULL;
    if (version != 0) return NULL;
    AP4_PiffTrackEncryptionAtom* piff = new AP4_PiffTrackEncryptionAtom(size, version, flags);
    if (piff == NULL) {
        return NULL;
    }
    result = piff->Parse(stream);
    if (AP4_FAILED(result)) {
        delete piff;
        return NULL;
    }
    return piff;
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::AP4_PiffTrackEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffTrackEncryptionAtom::AP4_PiffTrackEncryptionAtom(AP4_UI32        size, 
                                                         AP4_UI08        version,
                                                         AP4_UI32        flags) :
    AP4_UuidAtom(size, AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM, version, flags),
    AP4_CencTrackEncryption(version)
{
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::AP4_PiffTrackEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffTrackEncryptionAtom::AP4_PiffTrackEncryptionAtom(AP4_UI32        default_algorithm_id,
                                                         AP4_UI08        default_iv_size,
                                                         const AP4_UI08* default_kid) :
    AP4_UuidAtom(AP4_FULL_UUID_ATOM_HEADER_SIZE+20, AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM, 0, 0),
    AP4_CencTrackEncryption(0,
                            default_algorithm_id,
                            default_iv_size,
                            default_kid)
{
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffTrackEncryptionAtom::InspectFields(AP4_AtomInspector& inspector)
{
    return DoInspectFields(inspector);
}

/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffTrackEncryptionAtom::WriteFields(AP4_ByteStream& stream)
{
    return DoWriteFields(stream);
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
    AP4_UI08 version = 0;
    AP4_UI32 flags   = 0;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    AP4_Result result = ReadFullHeader(stream, version, flags);
    if (AP4_FAILED(result)) return NULL;
    if (version != 0) return NULL;
    return new AP4_PiffSampleEncryptionAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom(AP4_UI08 iv_size) :
    AP4_UuidAtom(AP4_FULL_UUID_ATOM_HEADER_SIZE+4, AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM, 0, 0),
    AP4_CencSampleEncryption(*((AP4_Atom*)this), iv_size)
{
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom(AP4_UI32        size, 
                                                           AP4_UI08        version,
                                                           AP4_UI32        flags,
                                                           AP4_ByteStream& stream) :
    AP4_UuidAtom(size, AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM, version, flags),
    AP4_CencSampleEncryption(*this, size, stream)
{
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom
+---------------------------------------------------------------------*/
AP4_PiffSampleEncryptionAtom::AP4_PiffSampleEncryptionAtom(AP4_UI32        algorithm_id,
                                                           AP4_UI08        iv_size,
                                                           const AP4_UI08* kid) :
    AP4_UuidAtom(AP4_FULL_UUID_ATOM_HEADER_SIZE+20+4, AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM, 0, 
                 AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS),
    AP4_CencSampleEncryption(*this, algorithm_id, iv_size, kid)
{
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSampleEncryptionAtom::InspectFields(AP4_AtomInspector& inspector)
{
    return DoInspectFields(inspector);
}

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result 
AP4_PiffSampleEncryptionAtom::WriteFields(AP4_ByteStream& stream)
{
    return DoWriteFields(stream);
}

