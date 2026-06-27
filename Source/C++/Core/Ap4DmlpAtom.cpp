/*****************************************************************
|
|    AP4 - dmlp Atoms
|
|    Copyright 2002-2018 Axiomatic Systems, LLC
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
#include "Ap4Atom.h"
#include "Ap4DmlpAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_DmlpAtom)

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::Create
+---------------------------------------------------------------------*/
AP4_DmlpAtom*
AP4_DmlpAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    if (size < AP4_ATOM_HEADER_SIZE + 10) {
        return NULL;
    }
    
    AP4_UI32 format_info;
    stream.ReadUI32(format_info);
    AP4_UI16 peak_data_rate;
    stream.ReadUI16(peak_data_rate);
    AP4_UI32 reserved;
    stream.ReadUI32(reserved);
    
    return new AP4_DmlpAtom(format_info, peak_data_rate);
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::AP4_DmlpAtom
+---------------------------------------------------------------------*/
AP4_DmlpAtom::AP4_DmlpAtom(AP4_UI32 format_info, AP4_UI16 peak_data_rate) :
    AP4_Atom(AP4_ATOM_TYPE_DMLP, AP4_ATOM_HEADER_SIZE),
    m_FormatInfo(format_info),
    m_PeakDataRate(peak_data_rate)
{
    m_Size32 += 10;
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::AP4_DmlpAtom
+---------------------------------------------------------------------*/
AP4_DmlpAtom::AP4_DmlpAtom(const AP4_DmlpAtom& other):
    AP4_Atom(AP4_ATOM_TYPE_DMLP, other.m_Size32),
    m_FormatInfo(other.m_FormatInfo),
    m_PeakDataRate(other.m_PeakDataRate)
{
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::GetCodecString
+---------------------------------------------------------------------*/
void
AP4_DmlpAtom::GetCodecString(AP4_String& codec)
{
    codec = "mlpa";
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DmlpAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    result = stream.WriteUI32(m_FormatInfo);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_PeakDataRate << 1);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(0);
    if (AP4_FAILED(result)) return result;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DmlpAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("format_info", m_FormatInfo);
    inspector.AddField("peak_data_rate", m_PeakDataRate);
    return AP4_SUCCESS;
}

