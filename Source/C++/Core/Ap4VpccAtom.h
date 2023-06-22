/*****************************************************************
|
|    AP4 - vpcC Atoms
|
|    Copyright 2002-2020 Axiomatic Systems, LLC
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

#ifndef _AP4_VPCC_ATOM_H_
#define _AP4_VPCC_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"
#include "Ap4DataBuffer.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/


/*----------------------------------------------------------------------
|   AP4_VpccAtom
+---------------------------------------------------------------------*/
class AP4_VpccAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_VpccAtom, AP4_Atom)

    // class methods
    static AP4_VpccAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // constructors
    AP4_VpccAtom(AP4_UI08        profile,
                 AP4_UI08        level,
                 AP4_UI08        bit_depth,
                 AP4_UI08        chroma_subsampling,
                 bool            videofull_range_flag,
                 AP4_UI08        colour_primaries,
                 AP4_UI08        transfer_characteristics,
                 AP4_UI08        matrix_coefficients,
                 const AP4_UI08* codec_initialization_data,
                 unsigned int    codec_initialization_data_size
    );

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI08              GetProfile()                 { return m_Profile;                 }
    AP4_UI08              GetLevel()                   { return m_Level;                   }
    AP4_UI08              GetBitDepth()                { return m_BitDepth;                }
    AP4_UI08              GetChromaSubsampling()       { return m_ChromaSubsampling;       }
    bool                  GetVideoFullRangeFlag()      { return m_VideoFullRangeFlag;      }
    AP4_UI08              GetColourPrimaries()         { return m_ColourPrimaries;         }
    AP4_UI08              GetTransferCharacteristics() { return m_TransferCharacteristics; }
    AP4_UI08              GetMatrixCoefficients()      { return m_MatrixCoefficients;      }
    const AP4_DataBuffer& GetCodecInitializationData() { return m_CodecIntializationData;  }

    // helpers
    AP4_Result GetCodecString(AP4_UI32 container_type, AP4_String& codec);

private:
    // methods
    AP4_VpccAtom(AP4_UI32 size, const AP4_UI08* payload);

    // members
    AP4_UI08       m_Profile;
    AP4_UI08       m_Level;
    AP4_UI08       m_BitDepth;
    AP4_UI08       m_ChromaSubsampling;
    bool           m_VideoFullRangeFlag;
    AP4_UI08       m_ColourPrimaries;
    AP4_UI08       m_TransferCharacteristics;
    AP4_UI08       m_MatrixCoefficients;
    AP4_DataBuffer m_CodecIntializationData;
};

#endif // _AP4_VPCC_ATOM_H_
