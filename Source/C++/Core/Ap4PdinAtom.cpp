/*****************************************************************
|
|    AP4 - pdin Atoms 
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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4PdinAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   AP4_PdinAtom::Create
+---------------------------------------------------------------------*/
AP4_PdinAtom*
AP4_PdinAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_PdinAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_PdinAtom::AP4_PdinAtom
+---------------------------------------------------------------------*/
AP4_PdinAtom::AP4_PdinAtom() :
    AP4_Atom(AP4_ATOM_TYPE_PDIN, AP4_FULL_ATOM_HEADER_SIZE, 0, 0)
{
}

/*----------------------------------------------------------------------
|   AP4_PdinAtom::AddEntry
+---------------------------------------------------------------------*/
AP4_Result           
AP4_PdinAtom::AddEntry(AP4_UI32 rate, AP4_UI32 initial_delay)
{
    Entry entry;
    entry.m_Rate = rate;
    entry.m_InitialDelay = initial_delay;
    m_Entries.Append(entry);
    SetSize(AP4_FULL_ATOM_HEADER_SIZE+m_Entries.ItemCount()*8);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PdinAtom::AP4_PdinAtom
+---------------------------------------------------------------------*/
AP4_PdinAtom::AP4_PdinAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_PDIN, size, version, flags)
{
    AP4_UI32 entry_count = (size-AP4_FULL_ATOM_HEADER_SIZE)/8;
    m_Entries.SetItemCount(entry_count);
    for (unsigned int i=0; i<entry_count; i++) {
        stream.ReadUI32(m_Entries[i].m_Rate);
        stream.ReadUI32(m_Entries[i].m_InitialDelay);
    }
}

/*----------------------------------------------------------------------
|   AP4_PdinAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PdinAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    for (unsigned int i=0; i<m_Entries.ItemCount(); i++) {
        result = stream.WriteUI32(m_Entries[i].m_Rate);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_Entries[i].m_InitialDelay);
        if (AP4_FAILED(result)) return result;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PdinAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PdinAtom::InspectFields(AP4_AtomInspector& inspector)
{
    char header[32];
    for (AP4_Ordinal i=0; i<m_Entries.ItemCount(); i++) {
        AP4_FormatString(header, sizeof(header), "rate(%d)", i);
        inspector.AddField(header, m_Entries[i].m_Rate);
        AP4_FormatString(header, sizeof(header), "initial_delay(%d)", i);
        inspector.AddField(header, m_Entries[i].m_InitialDelay);
    }

    return AP4_SUCCESS;
}
