/*****************************************************************
|
|    AP4 - tenc Atoms 
|
|    Copyright 2002-2016 Axiomatic Systems, LLC
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
#include "Ap4TencAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_TencAtom)

/*----------------------------------------------------------------------
|   AP4_TencAtom::Create
+---------------------------------------------------------------------*/
AP4_TencAtom*
AP4_TencAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    AP4_TencAtom* tenc = new AP4_TencAtom(size, version, flags);
    if (tenc == NULL) return NULL;
    AP4_Result result = tenc->Parse(stream);
    if (AP4_FAILED(result)) {
        delete tenc;
        return NULL;
    }
    
    return tenc;
}

/*----------------------------------------------------------------------
|   AP4_TencAtom::AP4_TencAtom
+---------------------------------------------------------------------*/
AP4_TencAtom::AP4_TencAtom(AP4_UI32        default_is_protected,
                           AP4_UI08        default_per_sample_iv_size,
                           const AP4_UI08* default_kid) :
    AP4_Atom(AP4_ATOM_TYPE_TENC, AP4_FULL_ATOM_HEADER_SIZE+20, 0, 0),
    AP4_CencTrackEncryption(0,
                            default_is_protected,
                            default_per_sample_iv_size,
                            default_kid)
{
}

/*----------------------------------------------------------------------
|   AP4_TencAtom::AP4_TencAtom
+---------------------------------------------------------------------*/
AP4_TencAtom::AP4_TencAtom(AP4_UI32        default_is_protected,
                           AP4_UI08        default_per_sample_iv_size,
                           const AP4_UI08* default_kid,
                           AP4_UI08        default_constant_iv_size,
                           const AP4_UI08* default_constant_iv,
                           AP4_UI08        default_crypt_byte_block,
                           AP4_UI08        default_skip_byte_block) :
    AP4_Atom(AP4_ATOM_TYPE_TENC, AP4_FULL_ATOM_HEADER_SIZE+20+(default_per_sample_iv_size?0:1+default_constant_iv_size), 1, 0),
    AP4_CencTrackEncryption(1,
                            default_is_protected,
                            default_per_sample_iv_size,
                            default_kid,
                            default_constant_iv_size,
                            default_constant_iv,
                            default_crypt_byte_block,
                            default_skip_byte_block)
{
}

/*----------------------------------------------------------------------
|   AP4_TencAtom::Create
+---------------------------------------------------------------------*/
AP4_TencAtom::AP4_TencAtom(AP4_UI32 size,
                           AP4_UI08 version,
                           AP4_UI32 flags) :
    AP4_Atom(AP4_ATOM_TYPE_TENC, size, version, flags),
    AP4_CencTrackEncryption(version)
{
}

/*----------------------------------------------------------------------
|   AP4_TencAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TencAtom::WriteFields(AP4_ByteStream& stream)
{
    return DoWriteFields(stream);
}

/*----------------------------------------------------------------------
|   AP4_TencAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TencAtom::InspectFields(AP4_AtomInspector& inspector)
{
    return DoInspectFields(inspector);
}
