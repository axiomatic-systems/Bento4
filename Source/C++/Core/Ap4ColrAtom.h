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

#ifndef _AP4_COLR_ATOM_H_
#define _AP4_COLR_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI16 AP4_COLR_PRIMARIES_RESERVED0 = 0;
const AP4_UI16 AP4_COLR_PRIMARIES_BT709 = 1;
const AP4_UI16 AP4_COLR_PRIMARIES_UNSPECIFIED = 2;
const AP4_UI16 AP4_COLR_PRIMARIES_RESERVED = 3;
const AP4_UI16 AP4_COLR_PRIMARIES_BT470M = 4;
const AP4_UI16 AP4_COLR_PRIMARIES_BT470BG = 5;
const AP4_UI16 AP4_COLR_PRIMARIES_SMPTE170M = 6;
const AP4_UI16 AP4_COLR_PRIMARIES_SMPTE240M = 7;
const AP4_UI16 AP4_COLR_PRIMARIES_FILM = 8;
const AP4_UI16 AP4_COLR_PRIMARIES_BT2020 = 9;
const AP4_UI16 AP4_COLR_PRIMARIES_SMPTE428 = 10;
const AP4_UI16 AP4_COLR_PRIMARIES_XYZ = 10;
const AP4_UI16 AP4_COLR_PRIMARIES_SMPTE431 = 11;
const AP4_UI16 AP4_COLR_PRIMARIES_P3DCI = 11;
const AP4_UI16 AP4_COLR_PRIMARIES_SMPTE432 = 12;
const AP4_UI16 AP4_COLR_PRIMARIES_P3D65 = 12;

const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_RESERVED0 = 0;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_BT709 = 1;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_UNSPECIFIED = 2;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_RESERVED = 3;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_GAMMA22 = 4;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_GAMMA28 = 5;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_SMPTE170M = 6;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_SMPTE240M = 7;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_LINEAR = 8;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_LOG = 9;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_LOG_SQRT = 10;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_IEC61966_2_4 = 11;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_BT1361_ECG = 12;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_IEC61966_2_1 = 13;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_BT2020_10 = 14;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_BT2020_12 = 15;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_SMPTE2084 = 16;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_ST2084 = 16;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_SMPTE428 = 17;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_ARIB_STD_B67 = 18;
const AP4_UI16 AP4_COLR_TRANSFER_FUNCTION_HLG = 18;

const AP4_UI16 AP4_COLR_COLORSPACE_GBR = 0;
const AP4_UI16 AP4_COLR_COLORSPACE_BT709 = 1;
const AP4_UI16 AP4_COLR_COLORSPACE_UNSPECIFIED = 2;
const AP4_UI16 AP4_COLR_COLORSPACE_RESERVED = 3;
const AP4_UI16 AP4_COLR_COLORSPACE_FCC = 4;
const AP4_UI16 AP4_COLR_COLORSPACE_BT470BG = 5;
const AP4_UI16 AP4_COLR_COLORSPACE_SMPTE170M = 6;
const AP4_UI16 AP4_COLR_COLORSPACE_SMPTE240M = 7;
const AP4_UI16 AP4_COLR_COLORSPACE_YCOCG = 8;
const AP4_UI16 AP4_COLR_COLORSPACE_BT2020_NCL = 9;
const AP4_UI16 AP4_COLR_COLORSPACE_BT2020_CL = 10;
const AP4_UI16 AP4_COLR_COLORSPACE_SMPTE2085 = 11;
const AP4_UI16 AP4_COLR_COLORSPACE_CHROMA_DERIVED_NCL = 12;
const AP4_UI16 AP4_COLR_COLORSPACE_CHROMA_DERIVED_CL = 13;
const AP4_UI16 AP4_COLR_COLORSPACE_ICTCP = 14;

const AP4_UI32 AP4_COLR_TYPE_NCLC = AP4_ATOM_TYPE('n','c','l','c');

/*----------------------------------------------------------------------
|   AP4_ColrAtom
+---------------------------------------------------------------------*/
class AP4_ColrAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_ColrAtom, AP4_Atom)

    // class methods
    static AP4_ColrAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // constructors
    AP4_ColrAtom(AP4_UI32  type,
                 AP4_UI16   primaries,
                 AP4_UI16   transfer_function,
                 AP4_UI16   matrix_index);
    AP4_ColrAtom();

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI32 GetType()              const { return m_Type; }
    AP4_UI16 GetPrimaries()         const { return m_Primaries; }
    AP4_UI16 GetTransferFunction()  const { return m_TransferFunction; }
    AP4_UI16 GetMatrixIndex()       const { return m_MatrixIndex; }

private:
    // methods
    AP4_ColrAtom(AP4_UI32 size, AP4_ByteStream& stream);

    // members
    AP4_UI32 m_Type;
    AP4_UI16 m_Primaries;
    AP4_UI16 m_TransferFunction;
    AP4_UI16 m_MatrixIndex;
};

#endif // _AP4_COLR_ATOM_H_
