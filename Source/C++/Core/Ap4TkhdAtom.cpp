/*****************************************************************
|
|    AP4 - tkhd Atoms 
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
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
#include "Ap4TkhdAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_TkhdAtom)

/*----------------------------------------------------------------------
|   AP4_TkhdAtom::Create
+---------------------------------------------------------------------*/
AP4_TkhdAtom*
AP4_TkhdAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_TkhdAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_TkhdAtom::AP4_TkhdAtom
+---------------------------------------------------------------------*/
AP4_TkhdAtom::AP4_TkhdAtom(AP4_UI32        creation_time,
                           AP4_UI32        modification_time,
                           AP4_UI32        track_id,
                           AP4_UI64        duration,
                           AP4_UI16        volume,
                           AP4_UI32        width,
                           AP4_UI32        height,
                           AP4_UI16        layer,
                           AP4_UI16        alternate_group,
                           const AP4_SI32* matrix) :
    AP4_Atom(AP4_ATOM_TYPE_TKHD, AP4_FULL_ATOM_HEADER_SIZE+80, 0, 0),
    m_CreationTime(creation_time),
    m_ModificationTime(modification_time),
    m_TrackId(track_id),
    m_Reserved1(0),
    m_Duration(duration),
    m_Layer(layer),
    m_AlternateGroup(alternate_group),
    m_Volume(volume),
    m_Reserved3(0),
    m_Width(width),
    m_Height(height)
{
    m_Flags = AP4_TKHD_FLAG_DEFAULTS;

    if (matrix) {
        for (unsigned int i=0; i<9; i++) {
            m_Matrix[i] = matrix[i];
        }
    } else {
        // default matrix
        m_Matrix[0] = 0x00010000;
        m_Matrix[1] = 0;
        m_Matrix[2] = 0;
        m_Matrix[3] = 0;
        m_Matrix[4] = 0x00010000;
        m_Matrix[5] = 0;
        m_Matrix[6] = 0;
        m_Matrix[7] = 0;
        m_Matrix[8] = 0x40000000;
    }
    
    m_Reserved2[0] = 0;
    m_Reserved2[1] = 0;

    if (duration > 0xFFFFFFFF) {
        m_Version = 1;
        m_Size32 += 12;
    }
}

/*----------------------------------------------------------------------
|   AP4_TkhdAtom::AP4_TkhdAtom
+---------------------------------------------------------------------*/
AP4_TkhdAtom::AP4_TkhdAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_TKHD, size, version, flags)
{
    if (m_Version == 0) {
        // we only deal with version 0 for now
        AP4_UI32 creation_time;
        stream.ReadUI32(creation_time);
        m_CreationTime = creation_time;
        AP4_UI32 modification_time;
        stream.ReadUI32(modification_time);
        m_ModificationTime = modification_time;
        stream.ReadUI32(m_TrackId);
        stream.ReadUI32(m_Reserved1);
        AP4_UI32 duration;
        stream.ReadUI32(duration);
        m_Duration = duration;
    } else {
        stream.ReadUI64(m_CreationTime);
        stream.ReadUI64(m_ModificationTime);
        stream.ReadUI32(m_TrackId);
        stream.ReadUI32(m_Reserved1);
        stream.ReadUI64(m_Duration);
    }

    stream.Read((void*)m_Reserved2, 8);
    stream.ReadUI16(m_Layer);
    stream.ReadUI16(m_AlternateGroup);
    stream.ReadUI16(m_Volume);
    stream.ReadUI16(m_Reserved3);
    for (int i=0; i<9; i++) {
        AP4_UI32 coefficient = 0;
        stream.ReadUI32(coefficient);
        m_Matrix[i] = (AP4_SI32)coefficient;
    }
    stream.ReadUI32(m_Width);
    stream.ReadUI32(m_Height);
}

/*----------------------------------------------------------------------
|   AP4_TkhdAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TkhdAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    // creation/modification time, track id, reserved1 & duration
    if (m_Version == 0) {
        result = stream.WriteUI32((AP4_UI32)m_CreationTime);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32((AP4_UI32)m_ModificationTime);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_TrackId);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_Reserved1);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32((AP4_UI32)m_Duration);
        if (AP4_FAILED(result)) return result;
    } else {
        result = stream.WriteUI64(m_CreationTime);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI64(m_ModificationTime);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_TrackId);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_Reserved1);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI64(m_Duration);
        if (AP4_FAILED(result)) return result;
    }

    // reserved2
    result = stream.Write(m_Reserved2, sizeof(m_Reserved2));
    if (AP4_FAILED(result)) return result;

    // layer, alternate group & volume
    result = stream.WriteUI16(m_Layer);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_AlternateGroup);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_Volume);
    if (AP4_FAILED(result)) return result;

    // reserved3
    result = stream.WriteUI16(m_Reserved3);
    if (AP4_FAILED(result)) return result;

    // matrix
    for (int i=0; i<9; i++) {
        result = stream.WriteUI32(m_Matrix[i]);
        if (AP4_FAILED(result)) return result;
    }

    // width & height
    result = stream.WriteUI32(m_Width);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(m_Height);
    if (AP4_FAILED(result)) return result;

    return result;
}

/*----------------------------------------------------------------------
|   AP4_TkhdAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TkhdAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("enabled", ((m_Flags & AP4_TKHD_FLAG_TRACK_ENABLED) ? 1 : 0), AP4_AtomInspector::HINT_BOOLEAN);
    inspector.AddField("id", m_TrackId);
    inspector.AddField("duration", m_Duration);
    if (inspector.GetVerbosity() > 0) {
        inspector.AddField("volume", m_Volume);
        inspector.AddField("layer", m_Layer);
        inspector.AddField("alternate_group", m_AlternateGroup);
        inspector.AddFieldF("matrix_0", (float)m_Matrix[0]/65536.0f);
        inspector.AddFieldF("matrix_1", (float)m_Matrix[1]/65536.0f);
        inspector.AddFieldF("matrix_2", (float)m_Matrix[2]/65536.0f);
        inspector.AddFieldF("matrix_3", (float)m_Matrix[3]/65536.0f);
        inspector.AddFieldF("matrix_4", (float)m_Matrix[4]/65536.0f);
        inspector.AddFieldF("matrix_5", (float)m_Matrix[5]/65536.0f);
        inspector.AddFieldF("matrix_6", (float)m_Matrix[6]/65536.0f);
        inspector.AddFieldF("matrix_7", (float)m_Matrix[7]/65536.0f);
        inspector.AddFieldF("matrix_8", (float)m_Matrix[8]/65536.0f);
    }
    inspector.AddFieldF("width",    (float)m_Width/65536.0f);
    inspector.AddFieldF("height",   (float)m_Height/65536.0f);
    
    return AP4_SUCCESS;
}
