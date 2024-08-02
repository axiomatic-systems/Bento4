/*****************************************************************
|
|    AP4 - av1C Atoms
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

#ifndef _AP4_AV1C_ATOM_H_
#define _AP4_AV1C_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_AV1_PROFILE_MAIN         = 0;
const AP4_UI08 AP4_AV1_PROFILE_HIGH         = 1;
const AP4_UI08 AP4_AV1_PROFILE_PROFESSIONAL = 2;

/*----------------------------------------------------------------------
|   AP4_Av1cAtom
+---------------------------------------------------------------------*/
class AP4_Av1cAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_Av1cAtom, AP4_Atom)

    // class methods
    static AP4_Av1cAtom* Create(AP4_Size size, AP4_ByteStream& stream);
    static const char*   GetProfileName(AP4_UI08 profile);

    // constructors
    AP4_Av1cAtom(AP4_UI08        version,
                 AP4_UI08        seq_profile,
                 AP4_UI08        seq_level_idx_0,
                 AP4_UI08        seq_tier_0,
                 AP4_UI08        high_bitdepth,
                 AP4_UI08        twelve_bit,
                 AP4_UI08        monochrome,
                 AP4_UI08        chroma_subsampling_x,
                 AP4_UI08        chroma_subsampling_y,
                 AP4_UI08        chroma_sample_position,
                 AP4_UI08        initial_presentation_delay_present,
                 AP4_UI08        initial_presentation_delay_minus_one,
                 const AP4_UI08* config_obus,
                 AP4_Size        config_obus_size);
    AP4_Av1cAtom();
    
    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI08 GetSeqProfile()                       const { return m_SeqProfile; }
    AP4_UI08 GetSeqLevelIdx0()                     const { return m_SeqLevelIdx0; }
    AP4_UI08 GetSeqTier0()                         const { return m_SeqTier0; }
    AP4_UI08 GetHighBitDepth()                     const { return m_HighBitDepth; }
    AP4_UI08 GetTwelveBit()                        const { return m_TwelveBit; }
    AP4_UI08 GetMonochrome()                       const { return m_Monochrome; }
    AP4_UI08 GetChromaSubsamplingX()               const { return m_ChromaSubsamplingX; }
    AP4_UI08 GetChromaSubsamplingY()               const { return m_ChromaSubsamplingY; }
    AP4_UI08 GetChromaSamplePosition()             const { return m_ChromaSamplePosition; }
    AP4_UI08 GetInitialPresentationDelayPresent()  const { return m_InitialPresentationDelayPresent; }
    AP4_UI08 GetInitialPresentationDelayMinusOne() const { return m_InitialPresentationDelayMinusOne; }
    const AP4_DataBuffer& GetConfigObus()          const { return m_ConfigObus; }
    
private:
    // methods
    AP4_Av1cAtom(AP4_UI32 size, const AP4_UI08* payload);
    
    // members
    AP4_UI08       m_Version;
    AP4_UI08       m_SeqProfile;
    AP4_UI08       m_SeqLevelIdx0;
    AP4_UI08       m_SeqTier0;
    AP4_UI08       m_HighBitDepth;
    AP4_UI08       m_TwelveBit;
    AP4_UI08       m_Monochrome;
    AP4_UI08       m_ChromaSubsamplingX;
    AP4_UI08       m_ChromaSubsamplingY;
    AP4_UI08       m_ChromaSamplePosition;
    AP4_UI08       m_InitialPresentationDelayPresent;
    AP4_UI08       m_InitialPresentationDelayMinusOne;
    AP4_DataBuffer m_ConfigObus;
};

#endif // _AP4_AV1C_ATOM_H_
