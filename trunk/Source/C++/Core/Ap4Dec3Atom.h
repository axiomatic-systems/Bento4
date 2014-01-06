/*****************************************************************
|
|    AP4 - dec3 Atoms
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

#ifndef _AP4_DEC3_ATOM_H_
#define _AP4_DEC3_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   AP4_Dec3Atom
+---------------------------------------------------------------------*/
class AP4_Dec3Atom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_Dec3Atom, AP4_Atom)

    // types
    struct SubStream {
		SubStream() : fscod(0), bsid(0), bsmod(0), acmod(0), lfeon(0), num_dep_sub(0), chan_loc(0) {}
        unsigned int fscod;
        unsigned int bsid;
        unsigned int bsmod;
        unsigned int acmod;
        unsigned int lfeon;
        unsigned int num_dep_sub;
        unsigned int chan_loc;
    };
    
    // class methods
    static AP4_Dec3Atom* Create(AP4_Size size, AP4_ByteStream& stream);

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);
    virtual AP4_Atom* Clone() { return new AP4_Dec3Atom(m_Size32, m_RawBytes.GetData()); }

    // accessors
    const AP4_DataBuffer&       GetRawBytes()   const { return m_RawBytes;   }
    unsigned int                GetDataRate()   const { return m_DataRate;   }
    const AP4_Array<SubStream>& GetSubStreams() const { return m_SubStreams; }
    
private:
    // methods
    AP4_Dec3Atom(AP4_UI32 size, const AP4_UI08* payload);
    
    // members
    unsigned int              m_DataRate;
    AP4_Array<SubStream>      m_SubStreams;
    AP4_DataBuffer            m_RawBytes;
};

#endif // _AP4_DEC3_ATOM_H_
