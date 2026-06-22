/*****************************************************************
|
|    AP4 - colr Atoms
|
|    Copyright 2002-2016 Axiomatic Systems, LLC
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
#include "Ap4ColrAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_ColrAtom)


/*----------------------------------------------------------------------
|   AP4_ColrAtom::Create
+---------------------------------------------------------------------*/
AP4_ColrAtom*
AP4_ColrAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;

    return new AP4_ColrAtom(size, payload_data.GetData());
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::AP4_ColrAtom
+---------------------------------------------------------------------*/
AP4_ColrAtom::AP4_ColrAtom() :
    AP4_Atom(AP4_ATOM_TYPE_COLR, AP4_ATOM_HEADER_SIZE + 11),
    m_ColourParameterType(0),
    m_PrimariesIndex(0),
    m_TransferFunctionIndex(0),
    m_MatrixIndex(0),
    m_FullRangeFlag(0),
    m_PayloadSize(11)
{
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::AP4_ColrAtom
+---------------------------------------------------------------------*/
AP4_ColrAtom::AP4_ColrAtom(AP4_UI32 colour_parameter_type,
                           AP4_UI16 primaries_index,
                           AP4_UI16 transfer_function_index,
                           AP4_UI16 matrix_index,
                           AP4_UI08 video_full_range_flag,
                           AP4_UI32 payload_size) :
    AP4_Atom(AP4_ATOM_TYPE_COLR, AP4_ATOM_HEADER_SIZE + payload_size),
    m_ColourParameterType(colour_parameter_type),
    m_PrimariesIndex(primaries_index),
    m_TransferFunctionIndex(transfer_function_index),
    m_MatrixIndex(matrix_index),
    m_FullRangeFlag(video_full_range_flag),
    m_PayloadSize(payload_size)
{
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::AP4_ColrAtom
+---------------------------------------------------------------------*/
AP4_ColrAtom::AP4_ColrAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_COLR, size)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    // payload size of QuickTime colr is 10,
    // payload size of ISOBMFF colr is 11.
    if (payload_size != 10 && payload_size != 11) return;

    // parse the payload
    m_PayloadSize = payload_size;
    m_ColourParameterType = AP4_BytesToUInt32BE(&payload[0]);
    m_PrimariesIndex = AP4_BytesToUInt16BE(&payload[4]);
    m_TransferFunctionIndex = AP4_BytesToUInt16BE(&payload[6]);
    m_MatrixIndex = AP4_BytesToUInt16BE(&payload[8]);
    if (payload_size == 11) {
        m_FullRangeFlag = (payload[10] >> 7) & 0x01;
    }
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ColrAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_UI08 payload[11];
    AP4_SetMemory(&payload, 0, 11);

    payload[0] = m_ColourParameterType >> 24;
    payload[1] = (m_ColourParameterType >> 16) & 0xFF;
    payload[2] = (m_ColourParameterType >> 8) & 0xFF;
    payload[3] = m_ColourParameterType & 0xFF;
    payload[4] = m_PrimariesIndex >> 8;
    payload[5] = m_PrimariesIndex & 0xFF;
    payload[6] = m_TransferFunctionIndex >> 8;
    payload[7] = m_TransferFunctionIndex & 0xFF;
    payload[8] = m_MatrixIndex >> 8;
    payload[9] = m_MatrixIndex & 0xFF;
    payload[10] = (m_FullRangeFlag << 7) & 0x80;

    if (GetPayloadSize() == 10) {
        return stream.Write(payload, 10);
    }
    return stream.Write(payload, 11);
}

/*----------------------------------------------------------------------
|   AP4_ColrAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ColrAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("Colour Parameter Type:", m_ColourParameterType);
    inspector.AddField("Primaries Index:", m_PrimariesIndex);
    inspector.AddField("Transfer Funcion Index:", m_TransferFunctionIndex);
    inspector.AddField("Matrix Index:", m_MatrixIndex);
    if (GetPayloadSize() == 11) {
        inspector.AddField("Video Full Range Flag:", m_FullRangeFlag);
    }
    return AP4_SUCCESS;
}
