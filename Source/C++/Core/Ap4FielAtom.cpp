/*****************************************************************
|
|    AP4 - fiel Atoms
|
|    Copyright 2002-2022 Axiomatic Systems, LLC
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
#include "Ap4FielAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_FielAtom)

/*----------------------------------------------------------------------
|   AP4_FielAtom::Create
+---------------------------------------------------------------------*/
AP4_FielAtom*
AP4_FielAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    return new AP4_FielAtom(size, stream);
}

/*----------------------------------------------------------------------
|   AP4_FielAtom::AP4_FielAtom
+---------------------------------------------------------------------*/
AP4_FielAtom::AP4_FielAtom(AP4_UI08 fields, AP4_UI08 detail) :
    AP4_Atom(AP4_ATOM_TYPE_FIEL, AP4_ATOM_HEADER_SIZE + 2),
    m_Fields(fields),
    m_Detail(detail)
{
}

/*----------------------------------------------------------------------
|   AP4_FielAtom::AP4_FielAtom
+---------------------------------------------------------------------*/
AP4_FielAtom::AP4_FielAtom() :
    AP4_Atom(AP4_ATOM_TYPE_FIEL, AP4_ATOM_HEADER_SIZE + 2),
    m_Fields(1),
    m_Detail(0)
{
}

/*----------------------------------------------------------------------
|   AP4_FielAtom::AP4_FielAtom
+---------------------------------------------------------------------*/
AP4_FielAtom::AP4_FielAtom(AP4_UI32        size,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_FIEL, size)
{
    stream.ReadUI08(m_Fields);
    stream.ReadUI08(m_Detail);
}

/*----------------------------------------------------------------------
|   AP4_FielAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_FielAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("fields", m_Fields);
    inspector.AddField("detail", m_Detail);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_FielAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_FielAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    result = stream.WriteUI08(m_Fields);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(m_Detail);
    return result;
}
