/*****************************************************************
|
|    AP4 - odda Atoms 
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf
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
#include "Ap4Utils.h"
#include "Ap4OddaAtom.h"

/*----------------------------------------------------------------------
|   AP4_OddaAtom::Create
+---------------------------------------------------------------------*/
AP4_OddaAtom*
AP4_OddaAtom::Create(AP4_Size         size, 
                     AP4_ByteStream&  stream)
{
    AP4_UI32 version;
    AP4_UI32 flags;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version != 0) return NULL;
    return new AP4_OddaAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_OddaAtom::AP4_OddaAtom
+---------------------------------------------------------------------*/
AP4_OddaAtom::AP4_OddaAtom(AP4_Size         size, 
                           AP4_UI32         version,
                           AP4_UI32         flags,
                           AP4_ByteStream&  stream) :
    AP4_Atom(AP4_ATOM_TYPE_ODDA, size, version, flags),
    m_SourceStream(&stream)
{
    // data length
    stream.ReadUI32(m_EncryptedDataLength.hi);
    stream.ReadUI32(m_EncryptedDataLength.lo);

    // store source stream position
    stream.Tell(m_SourcePosition);

    // keep a reference to the source stream
    m_SourceStream->AddReference();
}

/*----------------------------------------------------------------------
|   AP4_OddaAtom::~AP4_OddaAtom
+---------------------------------------------------------------------*/
AP4_OddaAtom::~AP4_OddaAtom()
{
    // release the source stream reference
    if (m_SourceStream) {
        m_SourceStream->Release();
    }
}

/*----------------------------------------------------------------------
|   AP4_OddaAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_OddaAtom::WriteFields(AP4_ByteStream& stream)
{
    // write the content type
    AP4_CHECK(stream.WriteUI32(m_EncryptedDataLength.hi));
    AP4_CHECK(stream.WriteUI32(m_EncryptedDataLength.lo));

    // check that we have a source stream
    // and a normal size
    if (m_SourceStream == NULL || m_Size < 8) {
        return AP4_FAILURE;
    }

    // remember the source position
    AP4_Position position;
    m_SourceStream->Tell(position);

    // seek into the source at the stored offset
    AP4_CHECK(m_SourceStream->Seek(m_SourcePosition));

    // copy the source stream to the output
    AP4_CHECK(m_SourceStream->CopyTo(stream, m_Size-GetHeaderSize()));

    // restore the original stream position
    m_SourceStream->Seek(position);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_OddaAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_OddaAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("encrypted_data_length", m_EncryptedDataLength.lo);
    return AP4_SUCCESS;
}
