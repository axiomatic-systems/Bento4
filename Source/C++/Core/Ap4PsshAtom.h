/*****************************************************************
|
|    AP4 - pssh Atoms 
|
|    Copyright 2002-2012 Axiomatic Systems, LLC
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

#ifndef _AP4_PSSH_ATOM_H_
#define _AP4_PSSH_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|   AP4_PsshAtom
+---------------------------------------------------------------------*/
class AP4_PsshAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_PsshAtom, AP4_Atom)

    // class methods
    static AP4_PsshAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // methods
    AP4_PsshAtom(const unsigned char* system_id, const AP4_UI08* kids = NULL, unsigned int kid_count = 0);
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    const unsigned char*  GetSystemId() { return m_SystemId; }
    void                  SetSystemId(const unsigned char system_id[16]);
    AP4_UI32              GetKidCount() { return m_KidCount; }
    const unsigned char*  GetKid(unsigned int index);
    void                  SetKids(const unsigned char* kids, AP4_UI32 kid_count);
    const AP4_DataBuffer& GetData() { return m_Data; }
    AP4_Result            SetData(const unsigned char* data, unsigned int data_size);
    AP4_Result            SetData(AP4_Atom& atom);
    AP4_Result            SetPadding(AP4_Byte* data, unsigned int data_size);
    
private:
    // methods
    AP4_PsshAtom(AP4_UI32        size, 
                 AP4_UI08        version,
                 AP4_UI32        flags,
                 AP4_ByteStream& stream);

    AP4_UI32 GetComputedSize();
    void     RecomputeSize();
    
    // members
    unsigned char  m_SystemId[16];
    AP4_DataBuffer m_Data;
    AP4_UI32       m_KidCount;
    AP4_DataBuffer m_Kids;
    AP4_DataBuffer m_Padding;
};

#endif // _AP4_PSSH_ATOM_H_
