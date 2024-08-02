/*****************************************************************
|
|    AP4 - bloc Atoms 
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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4BlocAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   AP4_BlocAtom Dynamic Cast Anchor
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_BlocAtom)


/*----------------------------------------------------------------------
|   AP4_BlocAtom::Create
+---------------------------------------------------------------------*/
AP4_BlocAtom*
AP4_BlocAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_BlocAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_BlocAtom::AP4_BlocAtom
+---------------------------------------------------------------------*/
AP4_BlocAtom::AP4_BlocAtom() :
    AP4_Atom(AP4_ATOM_TYPE_BLOC, AP4_FULL_ATOM_HEADER_SIZE+256+256+512, 0, 0)
{
    AP4_SetMemory(m_BaseLocation, 0, 256+1);
    AP4_SetMemory(m_PurchaseLocation, 0, 256+1);
    AP4_SetMemory(m_Reserved, 0, 512);
}

/*----------------------------------------------------------------------
|   AP4_BlocAtom::AP4_BlocAtom
+---------------------------------------------------------------------*/
AP4_BlocAtom::AP4_BlocAtom(AP4_UI32        size, 
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_BLOC, size, version, flags)
{
    m_BaseLocation[256] = 0;
    m_PurchaseLocation[256] = 0;
    stream.Read(m_BaseLocation, 256);
    stream.Read(m_PurchaseLocation, 256);
    stream.Read(m_Reserved, 512);
}

/*----------------------------------------------------------------------
|   AP4_BlocAtom::SetBaseLocation
+---------------------------------------------------------------------*/
void
AP4_BlocAtom::SetBaseLocation(const char* base_location)
{
    unsigned int len = (unsigned int)AP4_StringLength(base_location);
    if (len > 256) len = 256;
    AP4_CopyMemory(m_BaseLocation, base_location, len);
    AP4_SetMemory(&m_BaseLocation[len], 0, 256-len+1);
}

/*----------------------------------------------------------------------
|   AP4_BlocAtom::SetPurchaseLocation
+---------------------------------------------------------------------*/
void
AP4_BlocAtom::SetPurchaseLocation(const char* purchase_location)
{
    unsigned int len = (unsigned int)AP4_StringLength(purchase_location);
    if (len > 256) len = 256;
    AP4_CopyMemory(m_PurchaseLocation, purchase_location, len);
    AP4_SetMemory(&m_PurchaseLocation[len], 0, 256-len+1);
}

/*----------------------------------------------------------------------
|   AP4_BlocAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_BlocAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    result = stream.Write(m_BaseLocation, 256);
    if (AP4_FAILED(result)) return result;
    result = stream.Write(m_PurchaseLocation, 256);
    if (AP4_FAILED(result)) return result;
    result = stream.Write(m_Reserved, 512);
    if (AP4_FAILED(result)) return result;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_BlocAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_BlocAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("base_location", GetBaseLocation());
    inspector.AddField("purchase_location", GetPurchaseLocation());
    return AP4_SUCCESS;
}
