/*****************************************************************
|
|    AP4 - colr Atoms
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
#include "Ap4ColrAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_ColrAtom)

/*----------------------------------------------------------------------
|   AP4_ColrAtom::Create
+---------------------------------------------------------------------*/
AP4_ColrAtom*
AP4_ColrAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    return new AP4_ColrAtom(size, stream);
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::AP4_ColrAtom
+---------------------------------------------------------------------*/
AP4_ColrAtom::AP4_ColrAtom(AP4_UI32 type,
                           AP4_UI16 primaries,
                           AP4_UI16 transfer_function,
                           AP4_UI16 matrix_index) :
    AP4_Atom(AP4_ATOM_TYPE_COLR, AP4_ATOM_HEADER_SIZE + 10),
    m_Type(type),
    m_Primaries(primaries),
    m_TransferFunction(transfer_function),
    m_MatrixIndex(matrix_index)
{
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::AP4_ColrAtom
+---------------------------------------------------------------------*/
AP4_ColrAtom::AP4_ColrAtom() :
    AP4_Atom(AP4_ATOM_TYPE_COLR, AP4_ATOM_HEADER_SIZE + 10),
    m_Type(AP4_COLR_TYPE_NCLC),
    m_Primaries(AP4_COLR_PRIMARIES_BT709),
    m_TransferFunction(AP4_COLR_TRANSFER_FUNCTION_BT709),
    m_MatrixIndex(AP4_COLR_COLORSPACE_BT709)
{
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::AP4_ColrAtom
+---------------------------------------------------------------------*/
AP4_ColrAtom::AP4_ColrAtom(AP4_UI32        size,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_COLR, size)
{
    stream.ReadUI32(m_Type);
    stream.ReadUI16(m_Primaries);
    stream.ReadUI16(m_TransferFunction);
    stream.ReadUI16(m_MatrixIndex);
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ColrAtom::InspectFields(AP4_AtomInspector& inspector)
{
    char type[5];
    AP4_FormatFourChars(type, m_Type);
    inspector.AddField("type", type);
    inspector.AddField("primaries", m_Primaries);
    inspector.AddField("transfer_function", m_TransferFunction);
    inspector.AddField("matrix_index", m_MatrixIndex);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ColrAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    result = stream.WriteUI32(m_Type);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_Primaries);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_TransferFunction);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_MatrixIndex);
    return result;
}
