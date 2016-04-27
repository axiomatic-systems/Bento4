/*****************************************************************
|
|    AP4 - dvcC Atoms
|
|    Copyright 2002-2016 Axiomatic Systems, LLC
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

#ifndef _AP4_DVCC_ATOM_H_
#define _AP4_DVCC_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_DV_PROFILE_DVAV_PER = 0;
const AP4_UI08 AP4_DV_PROFILE_DVAV_PEN = 1;
const AP4_UI08 AP4_DV_PROFILE_DVHE_DER = 2;
const AP4_UI08 AP4_DV_PROFILE_DVHE_DEN = 3;
const AP4_UI08 AP4_DV_PROFILE_DVHE_DTR = 4;
const AP4_UI08 AP4_DV_PROFILE_DVHE_STN = 5;
const AP4_UI08 AP4_DV_PROFILE_DVHE_DTH = 6;
const AP4_UI08 AP4_DV_PROFILE_DVHE_DTB = 7;

/*----------------------------------------------------------------------
|   AP4_DvccAtom
+---------------------------------------------------------------------*/
class AP4_DvccAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_DvccAtom, AP4_Atom)

    // class methods
    static AP4_DvccAtom* Create(AP4_Size size, AP4_ByteStream& stream);
    static const char*   GetProfileName(AP4_UI08 profile);

    // constructors
    AP4_DvccAtom();
    AP4_DvccAtom(AP4_UI08 dv_version_major,
                 AP4_UI08 dv_version_minor,
                 AP4_UI08 dv_profile,
                 AP4_UI08 dv_level,
                 bool     rpu_present_flag,
                 bool     el_present_flag,
                 bool     bl_present_flag);
    
    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI08 GetDvVersionMajor() { return m_DvVersionMajor;      }
    AP4_UI08 GetDvVersionMinor() { return m_DvVersionMinor;      }
    AP4_UI08 GetDvProfile()      { return m_DvProfile;           }
    AP4_UI08 GetDvLevel()        { return m_DvLevel;             }
    bool     GetRpuPresentFlag() { return m_RpuPresentFlag != 0; }
    bool     GetElPresentFlag()  { return m_ElPresentFlag  != 0; }
    bool     GetBlPresentFlag()  { return m_BlPresentFlag  != 0; }

private:
    // members
    AP4_UI08 m_DvVersionMajor;
    AP4_UI08 m_DvVersionMinor;
    AP4_UI08 m_DvProfile;
    AP4_UI08 m_DvLevel;
    AP4_UI08 m_RpuPresentFlag;
    AP4_UI08 m_ElPresentFlag;
    AP4_UI08 m_BlPresentFlag;
};

#endif // _AP4_DVCC_ATOM_H_
