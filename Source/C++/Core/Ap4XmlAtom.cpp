/*****************************************************************
|
|    AP4 - xml Atoms
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
#include "Ap4XmlAtom.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(XmlAtom)

/*----------------------------------------------------------------------
|   AP4_XmlAtom::Create
+---------------------------------------------------------------------*/
AP4_XmlAtom*
AP4_XmlAtom::Create(AP4_UI32 size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 0) return NULL;
    return new AP4_XmlAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_XmlAtom::AP4_XmlAtom
+---------------------------------------------------------------------*/
AP4_XmlAtom::AP4_XmlAtom(AP4_UI32        size,
                         AP4_UI08        version,
                         AP4_UI32        flags,
                         AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_XML, size, version, flags)
{
    AP4_Size xml_size = (AP4_Size)size-AP4_FULL_ATOM_HEADER_SIZE;

    if (xml_size > 0) {
        char* xml_chars = new char[xml_size];
        stream.Read(xml_chars, xml_size);

        // Not null-terminated, so we need to specify the size
        m_Xml.Assign(xml_chars, xml_size);
    }
}

/*----------------------------------------------------------------------
|   AP4_XmlAtom::AP4_XmlAtom
+---------------------------------------------------------------------*/
AP4_XmlAtom::AP4_XmlAtom(const AP4_String& xml) :
    AP4_Atom(AP4_ATOM_TYPE_XML, AP4_FULL_ATOM_HEADER_SIZE+xml.GetLength(), 0, 0),
    m_Xml(xml)
{
}

/*----------------------------------------------------------------------
|   AP4_XmlAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_XmlAtom::WriteFields(AP4_ByteStream& stream)
{
    if (m_Size32 > AP4_FULL_ATOM_HEADER_SIZE) {
        AP4_Result result = stream.Write(m_Xml.GetChars(), m_Xml.GetLength());
        if (AP4_FAILED(result)) return result;

        // pad with zeros if necessary
        AP4_Size padding = m_Size32-(AP4_FULL_ATOM_HEADER_SIZE+m_Xml.GetLength());
        while (padding--) stream.WriteUI08(0);
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_XmlAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_XmlAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("xml", m_Xml.GetChars());

    return AP4_SUCCESS;
}
