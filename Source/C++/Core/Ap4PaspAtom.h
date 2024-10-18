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

#ifndef _AP4_PASP_ATOM_H_
#define _AP4_PASP_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|   AP4_PaspAtom
+---------------------------------------------------------------------*/
class AP4_PaspAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_PaspAtom, AP4_Atom)

    // class methods
    static AP4_PaspAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // constructors
    AP4_PaspAtom(AP4_UI32 hspacing, AP4_UI32 vspacing);

    AP4_PaspAtom();

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI32 GetHSpacing()              const { return m_HSpacing; }
    AP4_UI32 GetVSpacing()              const { return m_VSpacing; }

private:
    // methods
    AP4_PaspAtom(AP4_UI32 size, AP4_ByteStream& stream);

    // members
    AP4_UI32 m_HSpacing;
    AP4_UI32 m_VSpacing;
};

#endif // _AP4_PASP_ATOM_H_
