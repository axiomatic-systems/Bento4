/*****************************************************************
|
|    AP4 - senc Atoms 
|
|    Copyright 2002-2011 Axiomatic Systems, LLC
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
#include "Ap4SencAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_SencAtom)

/*----------------------------------------------------------------------
|   AP4_SencAtom::Create
+---------------------------------------------------------------------*/
AP4_SencAtom*
AP4_SencAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(ReadFullHeader(stream, version, flags))) return NULL;
    if (version != 0) return NULL;
    return new AP4_SencAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_SencAtom::AP4_SencAtom
+---------------------------------------------------------------------*/
AP4_SencAtom::AP4_SencAtom(AP4_UI08 iv_size) :
    AP4_Atom(AP4_ATOM_TYPE_SENC, AP4_FULL_ATOM_HEADER_SIZE+4, 0, 0),
    AP4_CencSampleEncryption(*((AP4_Atom*)this), iv_size)
{
	m_Outer = *this;
}

/*----------------------------------------------------------------------
|   AP4_SencAtom::AP4_SencAtom
+---------------------------------------------------------------------*/
AP4_SencAtom::AP4_SencAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_SENC, size, version, flags),
    AP4_CencSampleEncryption(*this, size, stream)
{
}

/*----------------------------------------------------------------------
|   AP4_SencAtom::AP4_SencAtom
+---------------------------------------------------------------------*/
AP4_SencAtom::AP4_SencAtom(AP4_UI32        algorithm_id,
                           AP4_UI08        iv_size,
                           const AP4_UI08* kid) :
    AP4_Atom(AP4_ATOM_TYPE_SENC, AP4_FULL_ATOM_HEADER_SIZE+20+4, 0, 
             AP4_CENC_SAMPLE_ENCRYPTION_FLAG_OVERRIDE_TRACK_ENCRYPTION_DEFAULTS),
    AP4_CencSampleEncryption(*this, algorithm_id, iv_size, kid)
{
}

/*----------------------------------------------------------------------
|   AP4_SencAtom::AP4_SencAtom
+---------------------------------------------------------------------*/
AP4_SencAtom::AP4_SencAtom(AP4_UI08        per_sample_iv_size,
                           AP4_UI08        constant_iv_size,
                           const AP4_UI08* constant_iv,
                           AP4_UI08        crypt_byte_block,
                           AP4_UI08        skip_byte_block):
    AP4_Atom(AP4_ATOM_TYPE_SENC, AP4_FULL_ATOM_HEADER_SIZE+4, 0, 0),
    AP4_CencSampleEncryption(*this,
                             per_sample_iv_size,
                             constant_iv_size,
                             constant_iv,
                             crypt_byte_block,
                             skip_byte_block)
{
}

/*----------------------------------------------------------------------
|   AP4_SencAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SencAtom::WriteFields(AP4_ByteStream& stream)
{
    return DoWriteFields(stream);
}

/*----------------------------------------------------------------------
|   AP4_SencAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SencAtom::InspectFields(AP4_AtomInspector& inspector)
{
    return DoInspectFields(inspector);
}
