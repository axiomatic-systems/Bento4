/*****************************************************************
|
|    AP4 - OMA DCF support
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf & Julien Boeuf
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

#ifndef _AP4_OMA_DCF_H_
#define _AP4_OMA_DCF_H_

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

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class AP4_CtrStreamCipher;
class AP4_OdafAtom;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_PROTECTION_SCHEME_TYPE_OMA = AP4_ATOM_TYPE('o','d','k','m');
const AP4_UI32 AP4_PROTECTION_SCHEME_VERSION_OMA_20 = 0x00000200;

/*----------------------------------------------------------------------
|   AP4_OmaDcfCipher
+---------------------------------------------------------------------*/
class AP4_OmaDcfCipher
{
public:
    // constructor and destructor
    AP4_OmaDcfCipher(const AP4_UI08* key,
                     const AP4_UI08* salt,
                     AP4_Size        counter_size);
   ~AP4_OmaDcfCipher();
    AP4_Result EncryptSample(AP4_DataBuffer& data_in,
                             AP4_DataBuffer& data_out,
                             AP4_UI32        counter,
                             bool            skip_encryption);
    AP4_Result DecryptSample(AP4_DataBuffer& data_in,
                             AP4_DataBuffer& data_out);
    AP4_CtrStreamCipher* GetCipher()   { return m_Cipher;   }

private:
    // members
    AP4_CtrStreamCipher* m_Cipher;
};

/*----------------------------------------------------------------------
|   AP4_OmaDcfTrackDecrypter
+---------------------------------------------------------------------*/
class AP4_OmaDcfTrackDecrypter : public AP4_Processor::TrackHandler {
public:
    // constructor
    static AP4_OmaDcfTrackDecrypter* Create(const AP4_UI08*                 key,
                                            AP4_ProtectedSampleDescription* sample_description,
                                            AP4_SampleEntry*                sample_entry);
    virtual ~AP4_OmaDcfTrackDecrypter();

    // methods
    virtual AP4_Size   GetProcessedSampleSize(AP4_Sample& sample);
    virtual AP4_Result ProcessTrack();
    virtual AP4_Result ProcessSample(AP4_DataBuffer& data_in,
                                     AP4_DataBuffer& data_out);

private:
    // constructor
    AP4_OmaDcfTrackDecrypter(AP4_OdafAtom*     cipher_params,
                             AP4_OmaDcfCipher* cipher,
                             AP4_SampleEntry*  sample_entry,
                             AP4_UI32          original_format);

    // members
    AP4_OdafAtom*     m_CipherParams;
    AP4_OmaDcfCipher* m_Cipher;
    AP4_SampleEntry*  m_SampleEntry;
    AP4_UI32          m_OriginalFormat;
    AP4_DataBuffer    m_PeekBuffer;
};

/*----------------------------------------------------------------------
|   AP4_TrackPropertyMap
+---------------------------------------------------------------------*/
class AP4_TrackPropertyMap
{
public:
    // methods
    AP4_Result  SetProperty(AP4_UI32 track_id, const char* name, const char* value);
    AP4_Result  SetProperties(const AP4_TrackPropertyMap& properties);
    const char* GetProperty(AP4_UI32 track_id, const char* name);

    // destructor
    virtual ~AP4_TrackPropertyMap();

private:
    // types
    class Entry {
    public:
        Entry(AP4_UI32 track_id, const char* name, const char* value) :
          m_TrackId(track_id), m_Name(name), m_Value(value) {}
        AP4_UI32   m_TrackId;
        AP4_String m_Name;
        AP4_String m_Value;
    };

    // members
    AP4_List<Entry> m_Entries;
};

/*----------------------------------------------------------------------
|   AP4_OmaDcfEncryptingProcessor
+---------------------------------------------------------------------*/
class AP4_OmaDcfEncryptingProcessor : public AP4_Processor
{
public:
    // accessors
    AP4_ProtectionKeyMap& GetKeyMap()      { return m_KeyMap;      }
    AP4_TrackPropertyMap& GetPropertyMap() { return m_PropertyMap; }

    // methods
    virtual AP4_Processor::TrackHandler* CreateTrackHandler(AP4_TrakAtom* trak);

private:
    // members
    AP4_ProtectionKeyMap m_KeyMap;
    AP4_TrackPropertyMap m_PropertyMap;
};

#endif // _AP4_OMA_DCF_H_
