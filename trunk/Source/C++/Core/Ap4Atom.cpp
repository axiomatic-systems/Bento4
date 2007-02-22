/*****************************************************************
|
|    AP4 - Atoms 
|
|    Copyright 2002-2006 Gilles Boccon-Gibod
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
#include "Ap4Types.h"
#include "Ap4Atom.h"
#include "Ap4Utils.h"
#include "Ap4ContainerAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4Debug.h"

/*----------------------------------------------------------------------
|   AP4_Atom::AP4_Atom
+---------------------------------------------------------------------*/
AP4_Atom::AP4_Atom(Type type, AP4_UI32 size /* = AP4_ATOM_HEADER_SIZE */) : 
    m_Type(type),
    m_Size32(size),
    m_Size64(0),
    m_IsFull(false),
    m_Version(0),
    m_Flags(0),
    m_Parent(NULL)
{
}

/*----------------------------------------------------------------------
|   AP4_Atom::AP4_Atom
+---------------------------------------------------------------------*/
AP4_Atom::AP4_Atom(Type type, AP4_UI64 size) : 
    m_Type(type),
    m_IsFull(false),
    m_Version(0),
    m_Flags(0),
    m_Parent(NULL)
{
    SetSize(size);
}

/*----------------------------------------------------------------------
|   AP4_Atom::AP4_Atom
+---------------------------------------------------------------------*/
AP4_Atom::AP4_Atom(Type     type, 
                   AP4_UI32 size, 
                   AP4_UI32 version, 
                   AP4_UI32 flags) :
    m_Type(type),
    m_Size32(size),
    m_Size64(0),
    m_IsFull(true),
    m_Version(version),
    m_Flags(flags),
    m_Parent(NULL)
{
}

/*----------------------------------------------------------------------
|   AP4_Atom::AP4_Atom
+---------------------------------------------------------------------*/
AP4_Atom::AP4_Atom(Type     type, 
                   AP4_UI64 size, 
                   AP4_UI32 version, 
                   AP4_UI32 flags) :
    m_Type(type),
    m_IsFull(true),
    m_Version(version),
    m_Flags(flags),
    m_Parent(NULL)
{
    SetSize(size);
}

/*----------------------------------------------------------------------
|   AP4_Atom::ReadFullHeader
+---------------------------------------------------------------------*/
AP4_Result
AP4_Atom::ReadFullHeader(AP4_ByteStream& stream, 
                         AP4_UI32&       version, 
                         AP4_UI32&       flags)
{
    AP4_UI32 header;
    AP4_CHECK(stream.ReadUI32(header));
    version = (header>>24)&0x000000FF;
    flags   = (header    )&0x00FFFFFF;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Atom::SetSize
+---------------------------------------------------------------------*/
void
AP4_Atom::SetSize(AP4_UI64 size)
{
    if ((size >> 32) == 0) {
        m_Size32 = (AP4_UI32)size;
        m_Size64 = 0;
    } else {
        m_Size32 = 1;
        m_Size64 = size;
    }
}

/*----------------------------------------------------------------------
|   AP4_Atom::GetHeaderSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_Atom::GetHeaderSize() const
{
    return m_IsFull ? AP4_FULL_ATOM_HEADER_SIZE : AP4_ATOM_HEADER_SIZE;
}

/*----------------------------------------------------------------------
|   AP4_Atom::WriteHeader
+---------------------------------------------------------------------*/
AP4_Result
AP4_Atom::WriteHeader(AP4_ByteStream& stream)
{
    AP4_Result result;

    // write the size
    result = stream.WriteUI32(m_Size32);
    if (AP4_FAILED(result)) return result;

    // handle 64-bit sizes
    if (m_Size32 == 1) {
        result = stream.WriteUI64(m_Size64);
        if (AP4_FAILED(result)) return result;
    }

    // write the type
    result = stream.WriteUI32(m_Type);
    if (AP4_FAILED(result)) return result;

    // for full atoms, write version and flags
    if (m_IsFull) {
        result = stream.WriteUI08(m_Version);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI24(m_Flags);
        if (AP4_FAILED(result)) return result;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Atom::Write
+---------------------------------------------------------------------*/
AP4_Result
AP4_Atom::Write(AP4_ByteStream& stream)
{
    AP4_Result result;

#if defined(AP4_DEBUG)
    AP4_Position before;
    stream.Tell(before);
#endif

    // write the header
    result = WriteHeader(stream);
    if (AP4_FAILED(result)) return result;

    // write the fields
    result = WriteFields(stream);
    if (AP4_FAILED(result)) return result;

#if defined(AP4_DEBUG)
    AP4_Position after;
    stream.Tell(after);
    AP4_ASSERT(after-before == GetSize());
#endif

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Atom::Inspect
+---------------------------------------------------------------------*/
AP4_Result
AP4_Atom::Inspect(AP4_AtomInspector& inspector)
{
    InspectHeader(inspector);
    InspectFields(inspector);
    inspector.EndElement();

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Atom::InspectHeader
+---------------------------------------------------------------------*/
AP4_Result
AP4_Atom::InspectHeader(AP4_AtomInspector& inspector)
{
    // write atom name
    char name[7];
    name[0] = '[';
    AP4_FormatFourChars(&name[1], m_Type);
    name[5] = ']';
    name[6] = '\0';
    char size[64];
    if (m_Size64 == 0) {
        AP4_FormatString(size, sizeof(size), "size=%ld+%ld", GetHeaderSize(), 
            m_Size32-GetHeaderSize());
    } else {
        AP4_UI64 payload_size = m_Size64-GetHeaderSize();
        AP4_UI32 payload_hi = (AP4_UI32)(payload_size>>32);
        AP4_UI32 payload_lo = (AP4_UI32)payload_size;
        AP4_FormatString(size, sizeof(size), "size=%ld+{%x%x}", GetHeaderSize(), 
            payload_hi, payload_lo);
    }
    inspector.StartElement(name, size);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Atom::Detach
+---------------------------------------------------------------------*/
AP4_Result
AP4_Atom::Detach() 
{
    if (m_Parent) {
        return m_Parent->RemoveChild(this);
    } else {
        return AP4_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   AP4_UnknownAtom::AP4_UnknownAtom
+---------------------------------------------------------------------*/
AP4_UnknownAtom::AP4_UnknownAtom(Type            type, 
                                 AP4_UI64        size,
                                 AP4_ByteStream& stream) : 
    AP4_Atom(type, size),
    m_SourceStream(&stream)
{
    // store source stream position
    stream.Tell(m_SourcePosition);

    // clamp to the file size
    AP4_UI64 file_size;
    if (AP4_SUCCEEDED(stream.GetSize(file_size))) {
        if (m_SourcePosition+size > file_size) {
            if (m_Size32 == 1) {
                // size is encoded as a large size
                m_Size64 = file_size-m_SourcePosition;
            } else {
                AP4_ASSERT(size <= 0xFFFFFFFF);
                m_Size32 = (AP4_UI32)(file_size-m_SourcePosition);
            }
        }
    }

    // keep a reference to the source stream
    m_SourceStream->AddReference();
}

/*----------------------------------------------------------------------
|   AP4_UnknownAtom::~AP4_UnknownAtom
+---------------------------------------------------------------------*/
AP4_UnknownAtom::~AP4_UnknownAtom()
{
    // release the source stream reference
    if (m_SourceStream) {
        m_SourceStream->Release();
    }
}

/*----------------------------------------------------------------------
|   AP4_UnknownAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_UnknownAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    // check that we have a source stream
    // and a normal size
    if (m_SourceStream == NULL || GetSize() < 8) {
        return AP4_FAILURE;
    }

    // remember the source position
    AP4_Position position;
    m_SourceStream->Tell(position);

    // seek into the source at the stored offset
    result = m_SourceStream->Seek(m_SourcePosition);
    if (AP4_FAILED(result)) return result;

    // copy the source stream to the output
    AP4_UI64 payload_size = GetSize()-GetHeaderSize();
    if (GetSize32() == 1) payload_size -= 8;
    result = m_SourceStream->CopyTo(stream, payload_size);
    if (AP4_FAILED(result)) return result;

    // restore the original stream position
    m_SourceStream->Seek(position);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_UnknownAtom::Clone
+---------------------------------------------------------------------*/
AP4_Atom*  
AP4_UnknownAtom::Clone()
{
    // refuse to clone large atoms
    if (GetSize() >= 32768) return NULL;
    AP4_UI32 size = (AP4_UI32)GetSize();

    AP4_MemoryByteStream* memory_stream = new AP4_MemoryByteStream(size);
    m_SourceStream->Seek(m_SourcePosition);
    m_SourceStream->CopyTo(*memory_stream, size);
    memory_stream->Seek(0);
    return new AP4_UnknownAtom(m_Type, GetSize(), *memory_stream);
    memory_stream->Release();
}

/*----------------------------------------------------------------------
|   AP4_AtomParent::~AP4_AtomParent
+---------------------------------------------------------------------*/
AP4_AtomParent::~AP4_AtomParent()
{
    m_Children.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_AtomParent::AddChild
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomParent::AddChild(AP4_Atom* child, int position)
{
    // check that the child does not already have a parent
    if (child->GetParent() != NULL) return AP4_ERROR_INVALID_PARAMETERS;

    // attach the child
    AP4_Result result;
    if (position == -1) {
        // insert at the tail
        result = m_Children.Add(child);
    } else if (position == 0) {
        // insert at the head
        result = m_Children.Insert(NULL, child);
    } else {
        // insert after <n-1>
        AP4_List<AP4_Atom>::Item* insertion_point = m_Children.FirstItem();
        unsigned int count = position;
        while (insertion_point && --count) {
            insertion_point = insertion_point->GetNext();
        }
        if (insertion_point) {
            result = m_Children.Insert(insertion_point, child);
        } else {
            result = AP4_ERROR_OUT_OF_RANGE;
        }
    }
    if (AP4_FAILED(result)) return result;

    // notify the child of its parent
    child->SetParent(this);

    // get a chance to update
    OnChildAdded(child);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AtomParent::RemoveChild
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomParent::RemoveChild(AP4_Atom* child)
{
    // check that this is our child
    if (child->GetParent() != this) return AP4_ERROR_INVALID_PARAMETERS;

    // remove the child
    AP4_Result result = m_Children.Remove(child);
    if (AP4_FAILED(result)) return result;

    // notify that child that it is orphaned
    child->SetParent(NULL);

    // get a chance to update
    OnChildRemoved(child);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AtomParent::DeleteChild
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomParent::DeleteChild(AP4_Atom::Type type)
{
    // find the child
    AP4_Atom* child = GetChild(type);
    if (child == NULL) return AP4_FAILURE;

    // remove the child
    AP4_Result result = RemoveChild(child);
    if (AP4_FAILED(result)) return result;

    // delete the child
    delete child;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AtomParent::GetChild
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_AtomParent::GetChild(AP4_Atom::Type type, AP4_Ordinal index /* = 0 */) const
{
    AP4_Atom* atom;
    AP4_Result result = m_Children.Find(AP4_AtomFinder(type, index), atom);
    if (AP4_SUCCEEDED(result)) {
        return atom;
    } else { 
        return NULL;
    }
}

/*----------------------------------------------------------------------
|   AP4_AtomParent::FindChild
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_AtomParent::FindChild(const char* path, 
                          bool        auto_create)
{
    // start from here
    AP4_AtomParent* parent = this;

    // walk the path
    while (path[0] && path[1] && path[2] && path[3]) {
        // we have 4 valid chars
        const char* tail;
        int         index = 0;
        if (path[4] == '\0') {
            tail = NULL;
        } else if (path[4] == '/') {
            // separator
            tail = &path[5];
        } else if (path[4] == '[') {
            const char* x = &path[5];
            while (*x >= '0' && *x <= '9') {
                index = 10*index+(*x++ - '0');
            }
            if (x[0] == ']') {
                if (x[1] == '\0') {
                    tail = NULL;
                } else {
                    tail = x+2;
                }
            } else {
                // malformed path
                return NULL;
            }
        } else {
            // malformed path
            return NULL;
        }

        // look for this atom in the current list
        AP4_Atom::Type type = AP4_ATOM_TYPE(path[0], path[1], path[2], path[3]); 
        AP4_Atom* atom = parent->GetChild(type, index);
        if (atom == NULL) {
            // not found
            if (auto_create && (index == 0)) {
                atom = new AP4_ContainerAtom(type, false);
                parent->AddChild(atom);
            } else {
                return NULL;
            }
        }

        if (tail) {
            path = tail;
            // if this atom is an atom parent, recurse
            parent = dynamic_cast<AP4_ContainerAtom*>(atom);
            if (parent == NULL) return NULL;
        } else {
            return atom;
        }
    }

    // not found
    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_AtomParent::FindChild
+---------------------------------------------------------------------*/
const unsigned int AP4_ATOM_LIST_WRITER_MAX_PADDING=1024;

AP4_Result
AP4_AtomListWriter::Action(AP4_Atom* atom) const
{
    AP4_Position before;
    m_Stream.Tell(before);

    atom->Write(m_Stream);

    AP4_Position after;
    m_Stream.Tell(after);

    AP4_UI64 bytes_written = after-before;
    AP4_ASSERT(bytes_written <= atom->GetSize());
    if (bytes_written < atom->GetSize()) {
        AP4_Debug("WARNING: atom serialized to fewer bytes that declared size\n");
        AP4_UI64 padding = atom->GetSize()-bytes_written;
        if (padding > AP4_ATOM_LIST_WRITER_MAX_PADDING) {
            AP4_Debug("WARNING: padding would be too large\n");
            return AP4_FAILURE;
        } else {
            for (unsigned int i=0; i<padding; i++) {
                m_Stream.WriteUI08(0);
            }
        }
    }

    return AP4_SUCCESS;
}
