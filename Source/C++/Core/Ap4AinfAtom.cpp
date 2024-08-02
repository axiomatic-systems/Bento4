/*****************************************************************
|
|    AP4 - Ainf Atoms 
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
#include "Ap4AinfAtom.h"
#include "Ap4Utils.h"


/*----------------------------------------------------------------------
|   AP4_AinfAtom Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_AinfAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_AINF_MAX_OTHER_SIZE = 16*1024*1024; // 16MB, for sanity

/*----------------------------------------------------------------------
|   AP4_AinfAtom::Create
+---------------------------------------------------------------------*/
AP4_AinfAtom*
AP4_AinfAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_AinfAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_AinfAtom::AP4_AinfAtom
+---------------------------------------------------------------------*/
AP4_AinfAtom::AP4_AinfAtom() :
    AP4_Atom(AP4_ATOM_TYPE_AINF, AP4_FULL_ATOM_HEADER_SIZE+4+1, 0, 0)
{
}

/*----------------------------------------------------------------------
|   AP4_AinfAtom::AP4_AinfAtom
+---------------------------------------------------------------------*/
AP4_AinfAtom::AP4_AinfAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_AINF, size, version, flags)
{
    stream.ReadUI32(m_ProfileVersion);
    if (size > AP4_FULL_ATOM_HEADER_SIZE+4 && size < AP4_AINF_MAX_OTHER_SIZE) {
        AP4_DataBuffer payload;
        unsigned int payload_size = size-AP4_FULL_ATOM_HEADER_SIZE-4;
        payload.SetDataSize(payload_size+1);
        payload.UseData()[payload_size] = 0;
        stream.Read(payload.UseData(), payload_size);
        m_APID = (const char*)payload.GetData();
        
        if (payload_size > m_APID.GetLength()+1) {
            unsigned int other_boxes_size = payload_size-m_APID.GetLength()-1;
            m_OtherBoxes.SetDataSize(other_boxes_size);
            AP4_CopyMemory(m_OtherBoxes.UseData(), payload.GetData(), other_boxes_size);
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_AinfAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AinfAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    result = stream.WriteUI32(m_ProfileVersion);
    if (AP4_FAILED(result)) return result;
    
    // stop early if we have an atom with no APID and a declared size
    // that doesn't leave enough room for the NULL terminator
    if (GetSize() <= AP4_FULL_ATOM_HEADER_SIZE+4) return AP4_SUCCESS;
    
    result = stream.Write(m_APID.GetChars(), m_APID.GetLength()+1);
    if (AP4_FAILED(result)) return result;
    if (m_OtherBoxes.GetDataSize()) {
        stream.Write(m_OtherBoxes.GetData(), m_OtherBoxes.GetDataSize());
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AinfAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AinfAtom::InspectFields(AP4_AtomInspector& inspector)
{
    char v[5];
    AP4_FormatFourChars(v, m_ProfileVersion);
    v[4] = 0;
    inspector.AddField("profile_version", v);
    inspector.AddField("APID", m_APID.GetChars());
    
    return AP4_SUCCESS;
}
