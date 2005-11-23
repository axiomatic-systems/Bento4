/*****************************************************************
|
|    AP4 - MetaData 
|
|    Copyright 2002 Gilles Boccon-Gibod
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

#ifndef _AP4_META_DATA_H_
#define _AP4_META_DATA_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4AtomFactory.h"

/*----------------------------------------------------------------------
|       metadata keys
+---------------------------------------------------------------------*/
const AP4_Atom::Type AP4_ATOM_TYPE_cNAM = AP4_ATOM_TYPE('©','n','a','m');
const AP4_Atom::Type AP4_ATOM_TYPE_cART = AP4_ATOM_TYPE('©','A','R','T');
const AP4_Atom::Type AP4_ATOM_TYPE_cALB = AP4_ATOM_TYPE('©','a','l','b');
const AP4_Atom::Type AP4_ATOM_TYPE_cGEN = AP4_ATOM_TYPE('©','g','e','n');

/*----------------------------------------------------------------------
|       AP4_MetaDataAtomTypeHandler
+---------------------------------------------------------------------*/
class AP4_MetaDataAtomTypeHandler : public AP4_AtomFactory::TypeHandler
{
public:
    // constructor
    AP4_MetaDataAtomTypeHandler(AP4_AtomFactory* atom_factory) :
      m_AtomFactory(atom_factory) {}
    virtual AP4_Result CreateAtom(AP4_Atom::Type  type,
                                  AP4_Size        size,
                                  AP4_ByteStream& stream,
                                  AP4_Atom*&      atom) = 0;

private:
    // members
    AP4_AtomFactory* m_AtomFactory;
};

#endif // _AP4_META_DATA_H_
