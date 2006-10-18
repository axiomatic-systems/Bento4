/*****************************************************************
|
|    AP4 - Container Atoms
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
#include "Ap4Types.h"
#include "Ap4Atom.h"
#include "Ap4Utils.h"
#include "Ap4ContainerAtom.h"
#include "Ap4AtomFactory.h"

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::AP4_ContainerAtom
+---------------------------------------------------------------------*/
AP4_ContainerAtom*
AP4_ContainerAtom::Create(Type             type, 
                          AP4_Size         size,
                          bool             is_full,
                          AP4_ByteStream&  stream,
                          AP4_AtomFactory& atom_factory)
{
    if (is_full) {
        AP4_UI32 version;
        AP4_UI32 flags;
        if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
        if (version != 0) return NULL;
        return new AP4_ContainerAtom(type, size, 0, 0, stream, atom_factory);
    } else {
        return new AP4_ContainerAtom(type, size, stream, atom_factory);
    }
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::AP4_ContainerAtom
+---------------------------------------------------------------------*/
AP4_ContainerAtom::AP4_ContainerAtom(Type type, AP4_Size size) :
    AP4_Atom(type, size)
{
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::AP4_ContainerAtom
+---------------------------------------------------------------------*/
AP4_ContainerAtom::AP4_ContainerAtom(Type     type, 
                                     AP4_Size size,
                                     AP4_UI32 version, 
                                     AP4_UI32 flags) :
    AP4_Atom(type, size, version, flags)
{
}


/*----------------------------------------------------------------------
|   AP4_ContainerAtom::AP4_ContainerAtom
+---------------------------------------------------------------------*/
AP4_ContainerAtom::AP4_ContainerAtom(Type             type, 
                                     AP4_Size         size,
                                     AP4_ByteStream&  stream,
                                     AP4_AtomFactory& atom_factory) :
    AP4_Atom(type, size)
{
    ReadChildren(atom_factory, stream, size-GetHeaderSize());
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::AP4_ContainerAtom
+---------------------------------------------------------------------*/
AP4_ContainerAtom::AP4_ContainerAtom(Type             type, 
                                     AP4_Size         size,
                                     AP4_UI32         version,
                                     AP4_UI32         flags,
                                     AP4_ByteStream&  stream,
                                     AP4_AtomFactory& atom_factory) :
    AP4_Atom(type, size, version, flags)
{
    ReadChildren(atom_factory, stream, size-GetHeaderSize());
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::Clone
+---------------------------------------------------------------------*/
AP4_Atom* 
AP4_ContainerAtom::Clone()
{
    AP4_ContainerAtom* clone;
    if (m_IsFull) {
        clone = new AP4_ContainerAtom(m_Type, AP4_FULL_ATOM_HEADER_SIZE, m_Version, m_Flags);
    } else {
        clone = new AP4_ContainerAtom(m_Type, AP4_ATOM_HEADER_SIZE);
    }

    AP4_List<AP4_Atom>::Item* child_item = m_Children.FirstItem();
    while (child_item) {
        AP4_Atom* child_clone = child_item->GetData()->Clone();
        if (child_clone) clone->AddChild(child_clone);
        child_item = child_item->GetNext();
    }

    return clone;
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::ReadChildren
+---------------------------------------------------------------------*/
void
AP4_ContainerAtom::ReadChildren(AP4_AtomFactory& atom_factory,
                                AP4_ByteStream&  stream, 
                                AP4_Size         size)
{
    AP4_Atom* atom;
    AP4_Size  bytes_available = size;

    // save and switch the factory's context
    AP4_Atom::Type saved_context = atom_factory.GetContext();
    atom_factory.SetContext(m_Type);

    while (AP4_SUCCEEDED(
        atom_factory.CreateAtomFromStream(stream, bytes_available, atom))) {
        atom->SetParent(this);
        m_Children.Add(atom);
    }

    // restore the saved context
    atom_factory.SetContext(saved_context);
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ContainerAtom::InspectFields(AP4_AtomInspector& inspector)
{
    return InspectChildren(inspector);
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::InspectChildren
+---------------------------------------------------------------------*/
AP4_Result
AP4_ContainerAtom::InspectChildren(AP4_AtomInspector& inspector)
{
    // inspect children
    m_Children.Apply(AP4_AtomListInspector(inspector));

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ContainerAtom::WriteFields(AP4_ByteStream& stream)
{
    // write all children
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::OnChildChanged
+---------------------------------------------------------------------*/
void
AP4_ContainerAtom::OnChildChanged(AP4_Atom*)
{
    // remcompute our size
    m_Size = GetHeaderSize();
    m_Children.Apply(AP4_AtomSizeAdder(m_Size));

    // update our parent
    if (m_Parent) m_Parent->OnChildChanged(this);
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::OnChildAdded
+---------------------------------------------------------------------*/
void
AP4_ContainerAtom::OnChildAdded(AP4_Atom* child)
{
    // update our size
    m_Size += child->GetSize();

    // update our parent
    if (m_Parent) m_Parent->OnChildChanged(this);
}

/*----------------------------------------------------------------------
|   AP4_ContainerAtom::OnChildRemoved
+---------------------------------------------------------------------*/
void
AP4_ContainerAtom::OnChildRemoved(AP4_Atom* child)
{
    // update our size
    m_Size -= child->GetSize();

    // update our parent
    if (m_Parent) m_Parent->OnChildChanged(this);
}
