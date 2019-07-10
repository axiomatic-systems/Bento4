/*****************************************************************
|
|    AP4 - tenc Atoms 
|
|    Copyright 2002-2011 Axiomatic Systems, LLC
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

#ifndef _AP4_TENC_ATOM_H_
#define _AP4_TENC_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4CommonEncryption.h"

/*----------------------------------------------------------------------
|   AP4_TencAtom
+---------------------------------------------------------------------*/
class AP4_TencAtom : public AP4_Atom, public AP4_CencTrackEncryption
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_TencAtom, AP4_Atom, AP4_CencTrackEncryption)

    // class methods
    static AP4_TencAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // constructors
    AP4_TencAtom(AP4_UI32        default_is_protected,
                 AP4_UI08        default_per_sample_iv_size,
                 const AP4_UI08* default_kid);
  
    AP4_TencAtom(AP4_UI32        default_is_protected,
                 AP4_UI08        default_per_sample_iv_size,
                 const AP4_UI08* default_kid,
                 AP4_UI08        default_constant_iv_size,
                 const AP4_UI08* default_constant_iv,
                 AP4_UI08        default_crypt_byte_block,
                 AP4_UI08        default_skip_byte_block);

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

private:
    // methods
    AP4_TencAtom(AP4_UI32 size,
                 AP4_UI08 version,
                 AP4_UI32 flags);
};

#endif // _AP4_TENC_ATOM_H_
