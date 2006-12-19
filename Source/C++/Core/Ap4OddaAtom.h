/*****************************************************************
|
|    AP4 - odda Atom
|
|    Copyright 2002-2005 Gilles Boccon-Gibod & Julien Boeuf
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

#ifndef _AP4_ODDA_ATOM_H_
#define _AP4_ODDA_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4Atom.h"
#include "Ap4String.h"

/*----------------------------------------------------------------------
|   AP4_OddaAtom
+---------------------------------------------------------------------*/
class AP4_OddaAtom : public AP4_Atom
{
public:
    // class methods
    static AP4_OddaAtom* Create(AP4_Size         size, 
                                AP4_ByteStream&  stream);

    // destructor
    ~AP4_OddaAtom();

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

private:
    // methods
    AP4_OddaAtom(AP4_UI32         size, 
                 AP4_UI32         version,
                 AP4_UI32         flags,
                 AP4_ByteStream&  stream);

    // members
    AP4_UI64        m_EncryptedDataLength;
    AP4_ByteStream* m_SourceStream;
    AP4_Position    m_SourcePosition;
};

#endif // _AP4_ODDA_ATOM_H_
