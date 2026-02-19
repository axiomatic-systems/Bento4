/*****************************************************************
|
|    AP4 - pasp Atoms
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
#include "Ap4PaspAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PaspAtom)

/*----------------------------------------------------------------------
|   AP4_PaspAtom::Create
+---------------------------------------------------------------------*/
AP4_PaspAtom*
AP4_PaspAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    return new AP4_PaspAtom(size, stream);
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::AP4_PaspAtom
+---------------------------------------------------------------------*/
AP4_PaspAtom::AP4_PaspAtom(AP4_UI32 hspacing, AP4_UI32 vspacing) :
    AP4_Atom(AP4_ATOM_TYPE_PASP, AP4_ATOM_HEADER_SIZE + 8),
    m_HSpacing(hspacing),
    m_VSpacing(vspacing)
{
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::AP4_PaspAtom
+---------------------------------------------------------------------*/
AP4_PaspAtom::AP4_PaspAtom() :
    AP4_Atom(AP4_ATOM_TYPE_PASP, AP4_ATOM_HEADER_SIZE + 8),
    m_HSpacing(1),
    m_VSpacing(1)
{
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::AP4_PaspAtom
+---------------------------------------------------------------------*/
AP4_PaspAtom::AP4_PaspAtom(AP4_UI32        size,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_PASP, size)
{
    stream.ReadUI32(m_HSpacing);
    stream.ReadUI32(m_VSpacing);
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PaspAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("hSpacing", m_HSpacing);
    inspector.AddField("vSpacing", m_VSpacing);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PaspAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    result = stream.WriteUI32(m_HSpacing);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(m_VSpacing);
    return result;
}
