/*****************************************************************
|
|    AP4 - sidx Atoms
|
|    Copyright 2002-2014 Axiomatic Systems, LLC
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

#ifndef _AP4_SIDX_ATOM_H_
#define _AP4_SIDX_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|   AP4_SidxAtom
+---------------------------------------------------------------------*/
class AP4_SidxAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST(AP4_SidxAtom)

    // types
    struct Reference {
        Reference() :
            m_ReferenceType(0),
            m_ReferencedSize(0),
            m_SubsegmentDuration(0),
            m_StartsWithSap(false),
            m_SapType(0),
            m_SapDeltaTime(0) {}
        AP4_UI08 m_ReferenceType;
        AP4_UI32 m_ReferencedSize;
        AP4_UI32 m_SubsegmentDuration;
        bool     m_StartsWithSap;
        AP4_UI08 m_SapType;
        AP4_UI32 m_SapDeltaTime;
    };
    
    // class methods
    static AP4_SidxAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // constructor
    AP4_SidxAtom(AP4_UI32 reference_id,
                 AP4_UI32 timescale,
                 AP4_UI64 earliest_presentation_time,
                 AP4_UI64 first_offset);
    
    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI32                    GetReferenceId()              { return m_ReferenceId;              }
    AP4_UI32                    GetTimeScale()                { return m_TimeScale;                }
    AP4_UI64                    GetEarliestPresentationTime() { return m_EarliestPresentationTime; }
    AP4_UI64                    GetFirstOffset()              { return m_FirstOffset;              }

    void SetReferenceId(AP4_UI32 reference_id)      { m_ReferenceId = reference_id;        }
    void SetTimeScale(AP4_UI32 timescale)           { m_TimeScale = timescale;             }
    void SetEarliestPresentationTime(AP4_UI64 time) { m_EarliestPresentationTime = time;   }
    void SetFirstOffset(AP4_UI64 offset)            { m_FirstOffset              = offset; }

    // access to references methods
    const AP4_Array<Reference>& GetReferences() {
        return m_References;
    }
    AP4_Array<Reference>& UseReferences() {
        return m_References;
    }
    void SetReferenceCount(unsigned int count);
    void SetReference(unsigned int     reference_index,
                      const Reference& reference) {
        m_References[reference_index] = reference;
    }
    
private:
    // methods
    AP4_SidxAtom(AP4_UI32        size, 
                 AP4_UI08        version,
                 AP4_UI32        flags,
                 AP4_ByteStream& stream);

    // members
    AP4_UI32             m_ReferenceId;
    AP4_UI32             m_TimeScale;
    AP4_UI64             m_EarliestPresentationTime;
    AP4_UI64             m_FirstOffset;
    AP4_Array<Reference> m_References;
};

#endif // _AP4_SIDX_ATOM_H_
