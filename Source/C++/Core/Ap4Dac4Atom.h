/*****************************************************************
|
|    AP4 - dac4 Atoms
|
|    Copyright 2002-2018 Axiomatic Systems, LLC
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

#ifndef _AP4_DEC4_ATOM_H_
#define _AP4_DEC4_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   AP4_Dac4Atom
+---------------------------------------------------------------------*/
class AP4_Dac4Atom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_Dac4Atom, AP4_Atom)

    // inner classes
    struct Ac4Dsi {
        struct PresentationV1 {
            AP4_UI08 presentation_version;
            union {
                struct {
                    AP4_UI08 presentation_config;
                    AP4_UI08 mdcompat;
                    AP4_UI08 presentation_group_index;
                    AP4_UI08 dsi_frame_rate_multiply_info;
                    AP4_UI08 presentation_emdf_version;
                    AP4_UI16 presentation_key_id;
                    AP4_UI32 presentation_channel_mask;
                    // more fields omitted
                } v0;
                struct {
                    AP4_UI08 presentation_config_v1;
                    AP4_UI08 mdcompat;
                    AP4_UI08 presentation_group_index;
                    AP4_UI08 dsi_frame_rate_multiply_info;
                    AP4_UI08 dsi_frame_rate_fraction_info;
                    AP4_UI08 presentation_emdf_version;
                    AP4_UI16 presentation_key_id;
                    AP4_UI08 b_presentation_channel_coded;
                    AP4_UI08 dsi_presentation_ch_mode;
                    AP4_UI08 pres_b_4_back_channels_present;
                    AP4_UI08 pres_top_channel_pairs;
                    AP4_UI32 presentation_channel_mask_v1;
                    // more fields omitted
                } v1;
            } d;
        };
        
        AP4_UI08 ac4_dsi_version;
        union {
            struct {
                AP4_UI08 bitstream_version;
                AP4_UI08 fs_index;
                AP4_UI32 fs;
                AP4_UI08 frame_rate_index;
                AP4_UI16 n_presentations;
                // more fields from `ac4_dsi` are not included here
            } v0;
            struct {
                AP4_UI08 bitstream_version;
                AP4_UI08 fs_index;
                AP4_UI32 fs;
                AP4_UI08 frame_rate_index;
                AP4_UI16 short_program_id;
                AP4_UI08 program_uuid[16];
                struct {
                    AP4_UI08 bit_rate_mode;
                    AP4_UI32 bit_rate;
                    AP4_UI32 bit_rate_precision;
                } ac4_bitrate_dsi;
                AP4_UI16        n_presentations;
                PresentationV1* presentations;
                // more fields from `ac4_dsi_v1` are not included here
            } v1;
        } d;
    };
    
    // class methods
    static AP4_Dac4Atom* Create(AP4_Size size, AP4_ByteStream& stream);

    // destructor
    ~AP4_Dac4Atom();
    
    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);
    virtual AP4_Atom*  Clone() { return new AP4_Dac4Atom(m_Size32, m_RawBytes.GetData()); }

    // accessors
    const AP4_DataBuffer& GetRawBytes() const { return m_RawBytes;   }
    const Ac4Dsi&         GetDsi()      const { return m_Dsi;        }

    // helpers
    void GetCodecString(AP4_String& codec);
    
private:
    // methods
    AP4_Dac4Atom(AP4_UI32 size, const AP4_UI08* payload);
    
    // members
    AP4_DataBuffer m_RawBytes;
    Ac4Dsi         m_Dsi;
};

#endif // _AP4_DEC3_ATOM_H_
