/*****************************************************************
|
|    AP4 - PIFF support
|
|    Copyright 2002-2009 Axiomatic Systems, LLC
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

#ifndef _AP4_PIFF_H_
#define _AP4_PIFF_H_

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
#include "Ap4UuidAtom.h"
#include "Ap4CommonEncryption.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class AP4_StreamCipher;
class AP4_CbcStreamCipher;
class AP4_CtrStreamCipher;
class AP4_PiffSampleEncryptionAtom;
class AP4_FragmentSampleTable;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_PROTECTION_SCHEME_TYPE_PIFF = AP4_ATOM_TYPE('p','i','f','f');
const AP4_UI32 AP4_PROTECTION_SCHEME_VERSION_PIFF_10 = 0x00010000;
const AP4_UI32 AP4_PROTECTION_SCHEME_VERSION_PIFF_11 = 0x00010001;
const AP4_UI32 AP4_PIFF_BRAND = AP4_ATOM_TYPE('p','i','f','f');

extern AP4_UI08 const AP4_UUID_PIFF_TRACK_ENCRYPTION_ATOM[16];
extern AP4_UI08 const AP4_UUID_PIFF_SAMPLE_ENCRYPTION_ATOM[16];

typedef enum {
    AP4_PIFF_CIPHER_MODE_CTR,
    AP4_PIFF_CIPHER_MODE_CBC
} AP4_PiffCipherMode;
                  
/*----------------------------------------------------------------------
|   AP4_PiffTrackEncryptionAtom
+---------------------------------------------------------------------*/
class AP4_PiffTrackEncryptionAtom : public AP4_UuidAtom, public AP4_CencTrackEncryption
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_PiffTrackEncryptionAtom, AP4_UuidAtom, AP4_CencTrackEncryption)

    // class methods
    static AP4_PiffTrackEncryptionAtom* Create(AP4_Size        size, 
                                               AP4_ByteStream& stream);

    // constructors
    AP4_PiffTrackEncryptionAtom(AP4_UI32        default_algorithm_id,
                                AP4_UI08        default_iv_size,
                                const AP4_UI08* default_kid);

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

private:
    // methods
    AP4_PiffTrackEncryptionAtom(AP4_UI32 size,
                                AP4_UI08 version,
                                AP4_UI32 flags);
};

/*----------------------------------------------------------------------
|   AP4_PiffSampleEncryptionAtom
+---------------------------------------------------------------------*/
class AP4_PiffSampleEncryptionAtom : public AP4_UuidAtom, public AP4_CencSampleEncryption
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D2(AP4_PiffSampleEncryptionAtom, AP4_UuidAtom, AP4_CencSampleEncryption)

    // class methods
    static AP4_PiffSampleEncryptionAtom* Create(AP4_Size        size, 
                                                AP4_ByteStream& stream);

    // constructors
    AP4_PiffSampleEncryptionAtom(AP4_UI08 iv_size);
    AP4_PiffSampleEncryptionAtom(AP4_UI32        algorithm_id,
                                 AP4_UI08        iv_size,
                                 const AP4_UI08* kid);

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

private:
    // methods
    AP4_PiffSampleEncryptionAtom(AP4_UI32        size, 
                                 AP4_UI08        version,
                                 AP4_UI32        flags,
                                 AP4_ByteStream& stream);
};

#endif // _AP4_PIFF_H_
