/*****************************************************************
|
|    AP4 - Stream Cipher
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf
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

#ifndef _AP4_STREAM_CIPHER_H_
#define _AP4_STREAM_CIPHER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4AesBlockCipher.h"
#include "Ap4Results.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   AP4_StreamCipher class
+---------------------------------------------------------------------*/
class AP4_StreamCipher
{
 public:
    // methods
    AP4_StreamCipher(const AP4_UI08* key = NULL, 
                     const AP4_UI08* salt = NULL,
                     AP4_Size        iv_size = 4);
    ~AP4_StreamCipher();
    AP4_Result SetStreamOffset(AP4_Offset offset);
    AP4_Result Reset(const AP4_UI08* key, const AP4_UI08* salt);
    AP4_Result ProcessBuffer(const AP4_UI08* in, 
                             AP4_UI08*       out,
                             AP4_Size        size);
    AP4_Offset GeStreamOffset() { return m_StreamOffset; }
    const AP4_UI08* GetSalt()   { return m_Salt;         }

 private:
    // members
    AP4_Offset          m_StreamOffset;
    AP4_Size            m_IvSize;
    AP4_UI08            m_CBlock[AP4_AES_BLOCK_SIZE];
    AP4_UI08            m_XBlock[AP4_AES_BLOCK_SIZE];
    AP4_UI08            m_Salt[AP4_AES_BLOCK_SIZE];
    AP4_AesBlockCipher* m_BlockCipher;

    // methods
    void SetCounter(AP4_Offset block_offset);
    AP4_Result UpdateKeyStream(AP4_Offset block_offset);
};

#endif // _AP4_STREAM_CIPHER_H_
