/*****************************************************************
|
|    AP4 - Marlin File Format support
|
|    Copyright 2008 Gilles Boccon-Gibod & Julien Boeuf
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

#ifndef _AP4_MARLIN_H_
#define _AP4_MARLIN_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4SampleEntry.h"
#include "Ap4Atom.h"
#include "Ap4AtomFactory.h"
#include "Ap4SampleDescription.h"
#include "Ap4Processor.h"
#include "Ap4Protection.h"
#include "Ap4TrefTypeAtom.h"
#include "Ap4ObjectDescriptor.h"
#include "Ap4Command.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_MARLIN_BRAND_MLN2       = AP4_ATOM_TYPE('M','G','S','V'); // FIXME: temporary value
const AP4_UI16 AP4_MARLIN_IPMPS_TYPE_MLN2  = 0xA551; // FIXME: temporary name
const AP4_UI32 AP4_MARLIN_SCHEME_TYPE_ACBC = AP4_ATOM_TYPE('A','C','B','C');

/*----------------------------------------------------------------------
|   AP4_MarlinDecryptingProcessor
+---------------------------------------------------------------------*/
class AP4_MarlinDecryptingProcessor : public AP4_Processor
{
public:
    // constructor and destructor
    AP4_MarlinDecryptingProcessor(const AP4_ProtectionKeyMap* key_map = NULL,
                                  AP4_BlockCipherFactory*     block_cipher_factory = NULL);
    ~AP4_MarlinDecryptingProcessor();
    
    // accessors
    AP4_ProtectionKeyMap& GetKeyMap() { return m_KeyMap; }

    // methods
    virtual AP4_Result Initialize(AP4_AtomParent&   top_level,
                                  AP4_ByteStream&   stream,
                                  ProgressListener* listener);
    virtual AP4_Processor::TrackHandler* CreateTrackHandler(AP4_TrakAtom* trak);

private:
    // types
    struct SinfEntry {
         SinfEntry(AP4_UI32 track_id, AP4_ContainerAtom* sinf) :
             m_TrackId(track_id), m_Sinf(sinf) {}
        ~SinfEntry() { delete m_Sinf; }
        AP4_UI32           m_TrackId;
        AP4_ContainerAtom* m_Sinf;
    };
    
    // members
    AP4_BlockCipherFactory*      m_BlockCipherFactory;
    AP4_ProtectionKeyMap         m_KeyMap;
    AP4_List<SinfEntry>          m_SinfEntries;
};

/*----------------------------------------------------------------------
|   AP4_MarlinTrackDecrypter
+---------------------------------------------------------------------*/
class AP4_MarlinTrackDecrypter : public AP4_Processor::TrackHandler
{
public:
    // class methods
    static AP4_Result Create(AP4_BlockCipherFactory&    cipher_factory,
                             const AP4_UI08*            key,
                             AP4_MarlinTrackDecrypter*& decrypter);
                             
    // destructor
    ~AP4_MarlinTrackDecrypter();
    
    // AP4_Processor::TrackHandler methods
    virtual AP4_Size GetProcessedSampleSize(AP4_Sample& sample);
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);


private:
    // constructor
    AP4_MarlinTrackDecrypter(AP4_StreamCipher* cipher) : m_Cipher(cipher) {}

    // members
    AP4_StreamCipher* m_Cipher;
};

#endif // _AP4_MARLIN_H_
