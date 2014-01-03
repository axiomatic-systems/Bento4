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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Dec3Atom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_Dec3Atom)

/*----------------------------------------------------------------------
|   AP4_Dec3Atom::Create
+---------------------------------------------------------------------*/
AP4_Dec3Atom* 
AP4_Dec3Atom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;
    
    // check the version
    const AP4_UI08* payload = payload_data.GetData();
    return new AP4_Dec3Atom(size, payload);
}

/*----------------------------------------------------------------------
|   AP4_Dec3Atom::AP4_Dec3Atom
+---------------------------------------------------------------------*/
AP4_Dec3Atom::AP4_Dec3Atom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_DEC3, size),
    m_DataRate(0)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    m_RawBytes.SetData(payload, payload_size);

    // sanity check
    if (payload_size < 2) return;
    
    // parse the payload
    m_DataRate = (payload[0]<<5)|(payload[1]>>3);
    unsigned int substream_count = 1+(payload[1]&7);
    payload      += 2;
    payload_size -= 2;
    m_SubStreams.SetItemCount(substream_count);
    for (unsigned int i=0; i<substream_count; i++) {
        if (payload_size < 3) {
            m_SubStreams[i].fscod       = 0;
            m_SubStreams[i].bsid        = 0;
            m_SubStreams[i].bsmod       = 0;
            m_SubStreams[i].acmod       = 0;
            m_SubStreams[i].lfeon       = 0;
            m_SubStreams[i].num_dep_sub = 0;
            m_SubStreams[i].chan_loc    = 0;
            continue;
        }
        m_SubStreams[i].fscod       = (payload[0]>>6) & 0x3;
        m_SubStreams[i].bsid        = (payload[0]>>1) & 0x1F;
        m_SubStreams[i].bsmod       = (payload[0]<<4 | payload[1]>>4) & 0x1F;
        m_SubStreams[i].acmod       = (payload[1]>>1) & 0x7;
        m_SubStreams[i].lfeon       = (payload[1]   ) & 0x1;
        m_SubStreams[i].num_dep_sub = (payload[2]>>1) & 0xF;
        if (m_SubStreams[i].num_dep_sub) {
            m_SubStreams[i].chan_loc = (payload[2]<<7 | payload[3]) & 0x1F;
            payload      += 4;
            payload_size -= 4;
        } else {
            m_SubStreams[i].chan_loc = 0;
            payload      += 3;
            payload_size -= 3;
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_Dec3Atom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dec3Atom::WriteFields(AP4_ByteStream& stream)
{
    return stream.Write(m_RawBytes.GetData(), m_RawBytes.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_Dec3Atom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Dec3Atom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("data_rate", m_DataRate);
    for (unsigned int i=0; i<m_SubStreams.ItemCount(); i++) {
        char name[16];
        char value[256];
        AP4_FormatString(name, sizeof(name), "[%02d]", i);
        AP4_FormatString(value, sizeof(value),
                         "fscod=%d, bsid=%d, bsmod=%d, acmod=%d, lfeon=%d, num_dep_sub=%d, chan_loc=%d",
                         m_SubStreams[i].fscod,
                         m_SubStreams[i].bsid,
                         m_SubStreams[i].bsmod,
                         m_SubStreams[i].acmod,
                         m_SubStreams[i].lfeon,
                         m_SubStreams[i].num_dep_sub,
                         m_SubStreams[i].chan_loc);
        inspector.AddField(name, value);
    }
    return AP4_SUCCESS;
}
