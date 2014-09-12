/*****************************************************************
|
|    AP4 - ainf Atoms 
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

#ifndef _AP4_AINF_ATOM_H_
#define _AP4_AINF_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|   AP4_AinfAtom
+---------------------------------------------------------------------*/
class AP4_AinfAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_AinfAtom, AP4_Atom)

    // class methods
    static AP4_AinfAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // methods
    AP4_AinfAtom();
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI32          GetProfileVersion()           { return m_ProfileVersion; }
    void              SetProfileVersion(AP4_UI32 v) { m_ProfileVersion = v;    }
    const AP4_String& GetAPID()                     { return m_APID;           }
    void              SetAPID(const char* apid)     { m_APID = apid;           }
    
private:
    // methods
    AP4_AinfAtom(AP4_UI32        size, 
                 AP4_UI08        version,
                 AP4_UI32        flags,
                 AP4_ByteStream& stream);

    // members
    AP4_UI32       m_ProfileVersion;
    AP4_String     m_APID;
    AP4_DataBuffer m_OtherBoxes;
};

#endif // _AP4_AINF_ATOM_H_
