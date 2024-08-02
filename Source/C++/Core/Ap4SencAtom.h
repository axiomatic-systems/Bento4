/*****************************************************************
|
|    AP4 - senc Atoms 
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

#ifndef _AP4_SENC_ATOM_H_
#define _AP4_SENC_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4CommonEncryption.h"

/*----------------------------------------------------------------------
|   AP4_SencAtom
+---------------------------------------------------------------------*/
class AP4_SencAtom : public AP4_Atom, public AP4_CencSampleEncryption
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_SencAtom, AP4_Atom, AP4_CencSampleEncryption)

    // class methods
    static AP4_SencAtom* Create(AP4_Size size, AP4_ByteStream& stream);
    
    // constructors
    AP4_SencAtom(AP4_UI08 iv_size = 16);
    AP4_SencAtom(AP4_UI32        algorithm_id,
                 AP4_UI08        per_sample_iv_size,
                 const AP4_UI08* kid);
    AP4_SencAtom(AP4_UI08        per_sample_iv_size,
                 AP4_UI08        constant_iv_size,
                 const AP4_UI08* constant_iv,
                 AP4_UI08        crypt_byte_block,
                 AP4_UI08        skip_byte_block);
    
    // methods  
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

private:
    AP4_SencAtom(AP4_UI32        size, 
                 AP4_UI08        version,
                 AP4_UI32        flags,
                 AP4_ByteStream& stream);
};

#endif // _AP4_SENC_ATOM_H_
