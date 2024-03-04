/*****************************************************************
|
|    AP4 - gama Atoms
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
#include "Ap4GamaAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_GamaAtom)

/*----------------------------------------------------------------------
|   AP4_GamaAtom::Create
+---------------------------------------------------------------------*/
AP4_GamaAtom*
AP4_GamaAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    return new AP4_GamaAtom(size, stream);
}

/*----------------------------------------------------------------------
|   AP4_GamaAtom::AP4_GamaAtom
+---------------------------------------------------------------------*/
AP4_GamaAtom::AP4_GamaAtom(AP4_UI32 gamma) :
    AP4_Atom(AP4_ATOM_TYPE_GAMA, AP4_ATOM_HEADER_SIZE + 4),
    m_Gamma(gamma)
{
}

/*----------------------------------------------------------------------
|   AP4_GamaAtom::AP4_GamaAtom
+---------------------------------------------------------------------*/
AP4_GamaAtom::AP4_GamaAtom() :
    AP4_Atom(AP4_ATOM_TYPE_GAMA, AP4_ATOM_HEADER_SIZE + 4),
    m_Gamma(1)
{
}

/*----------------------------------------------------------------------
|   AP4_GamaAtom::AP4_GamaAtom
+---------------------------------------------------------------------*/
AP4_GamaAtom::AP4_GamaAtom(AP4_UI32        size,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_GAMA, size)
{
    stream.ReadUI32(m_Gamma);
}

/*----------------------------------------------------------------------
|   AP4_GamaAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_GamaAtom::InspectFields(AP4_AtomInspector& inspector)
{
    double gamma = ((double)(m_Gamma)) / (1 << 16);
    inspector.AddFieldF("gamma", gamma);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_GamaAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_GamaAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    result = stream.WriteUI32(m_Gamma);
    return result;
}
