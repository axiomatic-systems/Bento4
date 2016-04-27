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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4DvccAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_DvccAtom)

/*----------------------------------------------------------------------
|   AP4_DvccAtom::GetProfileName
+---------------------------------------------------------------------*/
const char*
AP4_DvccAtom::GetProfileName(AP4_UI08 profile)
{
    switch (profile) {
        case AP4_DV_PROFILE_DVAV_PER: return "dvav.per";
        case AP4_DV_PROFILE_DVAV_PEN: return "dvav.pen";
        case AP4_DV_PROFILE_DVHE_DER: return "dvhe.der";
        case AP4_DV_PROFILE_DVHE_DEN: return "dvhe.den";
        case AP4_DV_PROFILE_DVHE_DTR: return "dvhe.dtr";
        case AP4_DV_PROFILE_DVHE_STN: return "dvhe.stn";
        case AP4_DV_PROFILE_DVHE_DTH: return "dvhe.dth";
        case AP4_DV_PROFILE_DVHE_DTB: return "dvhr.dtb";
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_DvccAtom::Create
+---------------------------------------------------------------------*/
AP4_DvccAtom* 
AP4_DvccAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    if (size < AP4_ATOM_HEADER_SIZE+24) return NULL;
    
    AP4_UI08 payload[24];
    AP4_Result result = stream.Read(payload, 24);
    if (AP4_FAILED(result)) return NULL;
    
    return new AP4_DvccAtom(payload[0],
                            payload[1],
                            payload[2]>>1,
                            ((payload[2]&1)<<5) | (payload[3]>>3),
                            (payload[3]&4) != 0,
                            (payload[3]&2) != 0,
                            (payload[3]&1) != 0);
}

/*----------------------------------------------------------------------
|   AP4_DvccAtom::AP4_DvccAtom
+---------------------------------------------------------------------*/
AP4_DvccAtom::AP4_DvccAtom() :
    AP4_Atom(AP4_ATOM_TYPE_DVCC, AP4_ATOM_HEADER_SIZE+24),
    m_DvVersionMajor(0),
    m_DvVersionMinor(0),
    m_DvProfile(0),
    m_DvLevel(0),
    m_RpuPresentFlag(false),
    m_ElPresentFlag(false),
    m_BlPresentFlag(false)
{
}

/*----------------------------------------------------------------------
|   AP4_DvccAtom::AP4_DvccAtom
+---------------------------------------------------------------------*/
AP4_DvccAtom::AP4_DvccAtom(AP4_UI08 dv_version_major,
                           AP4_UI08 dv_version_minor,
                           AP4_UI08 dv_profile,
                           AP4_UI08 dv_level,
                           bool     rpu_present_flag,
                           bool     el_present_flag,
                           bool     bl_present_flag) :
    AP4_Atom(AP4_ATOM_TYPE_DVCC, AP4_ATOM_HEADER_SIZE+24),
    m_DvVersionMajor(dv_version_major),
    m_DvVersionMinor(dv_version_minor),
    m_DvProfile(dv_profile),
    m_DvLevel(dv_level),
    m_RpuPresentFlag(rpu_present_flag),
    m_ElPresentFlag(el_present_flag),
    m_BlPresentFlag(bl_present_flag)
{
}

/*----------------------------------------------------------------------
|   AP4_DvccAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DvccAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_UI08 payload[24];
    AP4_SetMemory(payload, 0, 24);
    
    payload[0] = m_DvVersionMajor;
    payload[1] = m_DvVersionMinor;
    payload[2] = (m_DvProfile<<1) | ((m_DvLevel&0x20)>>5);
    payload[3] = (m_DvLevel<<3) | (m_RpuPresentFlag?4:0) | (m_ElPresentFlag?2:0) | (m_BlPresentFlag?1:0);
    
    return stream.Write(payload, 24);
}

/*----------------------------------------------------------------------
|   AP4_DvccAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DvccAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("dv_version_major", m_DvVersionMajor);
    inspector.AddField("dv_version_minor", m_DvVersionMinor);
    inspector.AddField("dv_profile", m_DvProfile);
    const char* profile_name = GetProfileName(m_DvProfile);
    if (profile_name) {
        inspector.AddField("dv_profile_name", profile_name);
    } else {
        inspector.AddField("dv_profile_name", "unknown");
    }
    inspector.AddField("dv_level", m_DvLevel);
    inspector.AddField("rpu_present_flag", m_RpuPresentFlag);
    inspector.AddField("el_present_flag",  m_ElPresentFlag);
    inspector.AddField("bl_present_flag",  m_BlPresentFlag);
    return AP4_SUCCESS;
}
