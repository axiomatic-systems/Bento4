/*****************************************************************
|
|    AP4 - tfdt Atoms 
|
|    Copyright 2002-2010 Axiomatic Systems, LLC
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
#include "Ap4TfdtAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_TfdtAtom)

/*----------------------------------------------------------------------
|   AP4_TfdtAtom::Create
+---------------------------------------------------------------------*/
AP4_TfdtAtom*
AP4_TfdtAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_TfdtAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_TfhdAtom::AP4_TfhdAtom
+---------------------------------------------------------------------*/
AP4_TfdtAtom::AP4_TfdtAtom(AP4_UI08 version, 
                           AP4_UI64 base_media_decode_time) :
    AP4_Atom(AP4_ATOM_TYPE_TFDT, AP4_FULL_ATOM_HEADER_SIZE+(version==0?4:8), version, 0),
    m_BaseMediaDecodeTime(base_media_decode_time)
{
}

/*----------------------------------------------------------------------
|   AP4_TfhdAtom::AP4_TfhdAtom
+---------------------------------------------------------------------*/
AP4_TfdtAtom::AP4_TfdtAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_TFDT, size, version, flags)
{
    if (version == 0) {
        AP4_UI32 value = 0;
        stream.ReadUI32(value);
        m_BaseMediaDecodeTime = value;
    } else if (version == 1) {
        stream.ReadUI64(m_BaseMediaDecodeTime);
    }
}

/*----------------------------------------------------------------------
|   AP4_TfdtAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TfdtAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    if (m_Version == 0) {
        result = stream.WriteUI32((AP4_UI32)m_BaseMediaDecodeTime);
    } else if (m_Version == 1) {
        result = stream.WriteUI64(m_BaseMediaDecodeTime);
    } else {
        return AP4_ERROR_NOT_SUPPORTED;
    }
    if (AP4_FAILED(result)) return result;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_TfdtAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TfdtAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("base media decode time", m_BaseMediaDecodeTime);

    return AP4_SUCCESS;
}
