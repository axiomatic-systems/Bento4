/*****************************************************************
|
|    AP4 - Colr Atoms
|
|    Copyright 2002-2014 Axiomatic Systems, LLC
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
#include "Ap4Array.h"

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
    AP4_ColrAtom();
    AP4_ColrAtom(AP4_UI32 colour_parameter_type,
                 AP4_UI16 primaries_index,
                 AP4_UI16 transfer_function_index,
                 AP4_UI16 matrix_index,
                 AP4_UI08 video_full_range_flag,
                 AP4_UI32 payload_size = 11); // default payload size is 11, unless specified
    AP4_ColrAtom(AP4_UI32 size, const AP4_UI08* payload);

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI32 GetColourParameterType()            { return m_ColourParameterType; }
    AP4_UI16 GetPrimariesIndex()                 { return m_PrimariesIndex; }
    AP4_UI16 GetTransferFunctionIndex()          { return m_TransferFunctionIndex; }
    AP4_UI16 GetMatrixIndex()                    { return m_MatrixIndex; }
    AP4_UI08 GetFullRangeFlag()                  { return m_FullRangeFlag; }
    AP4_UI32 GetPayloadSize()                    { return m_PayloadSize; }

private:

    // members
    AP4_UI32                  m_ColourParameterType;
    AP4_UI16                  m_PrimariesIndex;
    AP4_UI16                  m_TransferFunctionIndex;
    AP4_UI16                  m_MatrixIndex;
    AP4_UI08                  m_FullRangeFlag;
    AP4_UI32                  m_PayloadSize;

};

#endif // _AP4_COLR_ATOM_H_
