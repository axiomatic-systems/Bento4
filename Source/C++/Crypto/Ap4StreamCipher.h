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
|   AP4_CtrStreamCipher class
+---------------------------------------------------------------------*/
class AP4_CtrStreamCipher
{
public:
   // methods
   AP4_CtrStreamCipher(const AP4_UI08* key, 
                       const AP4_UI08* salt,
                       AP4_Size        counter_size);
   ~AP4_CtrStreamCipher();
   AP4_Result Reset(const AP4_UI08* key, const AP4_UI08* salt = NULL);
   void       SetStreamOffset(AP4_UI32 offset);
   void       SetBaseCounter(const AP4_UI08* counter);
   AP4_Result ProcessBuffer(const AP4_UI08* in, 
                            AP4_UI08*       out,
                            AP4_Size        size);
   const AP4_UI08* GetBaseCounter()  { return m_BaseCounter;  }
   AP4_Offset      GetStreamOffset() { return m_StreamOffset; }

private:
   // methods
   void SetCounterOffset(AP4_UI32 offset);
   void UpdateKeyStream();

   // members
   AP4_UI32            m_StreamOffset;
   AP4_Size            m_CounterSize;
   AP4_UI08            m_BaseCounter[AP4_AES_BLOCK_SIZE];
   AP4_UI08            m_CBlock[AP4_AES_BLOCK_SIZE];
   AP4_UI08            m_XBlock[AP4_AES_BLOCK_SIZE];
   AP4_AesBlockCipher* m_BlockCipher;
};

/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher class
+---------------------------------------------------------------------*/
class AP4_CbcStreamCipher
{
public:
    // types
    typedef enum {
        ENCRYPT,
        DECRYPT
    } CipherDirection;

    // methods
    AP4_CbcStreamCipher(const AP4_UI08* key, CipherDirection direction);
    ~AP4_CbcStreamCipher();
    AP4_Result SetIV(const AP4_UI08* iv);
    AP4_Result ProcessBuffer(const AP4_UI08* in, 
                             AP4_Size        in_size,
                             AP4_UI08*       out,
                             AP4_Size*       out_size,
                             bool            is_last_buffer = false);
    AP4_Offset GetStreamOffset() { return m_StreamOffset; }

private:
    // members
    CipherDirection     m_Direction;
    AP4_Position        m_StreamOffset;
    AP4_UI08            m_InBlockCache[AP4_AES_BLOCK_SIZE];
    AP4_UI08            m_OutBlockCache[AP4_AES_BLOCK_SIZE];
    AP4_AesBlockCipher* m_BlockCipher;
    bool                m_Eos;
};

#endif // _AP4_STREAM_CIPHER_H_
