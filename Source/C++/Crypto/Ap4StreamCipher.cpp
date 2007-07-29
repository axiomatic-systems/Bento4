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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4StreamCipher.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::AP4_CtrStreamCipher
+---------------------------------------------------------------------*/
AP4_CtrStreamCipher::AP4_CtrStreamCipher(AP4_BlockCipher* block_cipher,
                                         const AP4_UI08*  salt,
                                         AP4_Size         counter_size) :
    m_StreamOffset(0),
    m_CounterSize(counter_size),
    m_BlockCipher(block_cipher)
{
    if (m_CounterSize > 16) m_CounterSize = 16;

    // use the salt to initialize the base counter
    if (salt) {
        // initialize the base counter with a salting key
        AP4_CopyMemory(m_BaseCounter, salt, AP4_CIPHER_BLOCK_SIZE);
    } else {
        // initialize the base counter with zeros
        AP4_SetMemory(m_BaseCounter, 0, AP4_CIPHER_BLOCK_SIZE);
    }

    // reset the stream offset
    SetStreamOffset(0);
    SetIV(NULL);
}

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::~AP4_CtrStreamCipher
+---------------------------------------------------------------------*/
AP4_CtrStreamCipher::~AP4_CtrStreamCipher()
{
    delete m_BlockCipher;
}

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::SetIV
+---------------------------------------------------------------------*/
AP4_Result
AP4_CtrStreamCipher::SetIV(const AP4_UI08* counter)
{
    if (counter) {
        AP4_CopyMemory(&m_BaseCounter[AP4_CIPHER_BLOCK_SIZE-m_CounterSize],
                       counter, m_CounterSize);
    } else {
        AP4_SetMemory(&m_BaseCounter[AP4_CIPHER_BLOCK_SIZE-m_CounterSize],
                      0, m_CounterSize);
    }

    // for the stream offset back to 0
    return SetStreamOffset(0);
}

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::SetStreamOffset
+---------------------------------------------------------------------*/
AP4_Result
AP4_CtrStreamCipher::SetStreamOffset(AP4_UI64      offset,
                                     AP4_Cardinal* preroll)
{
    // do nothing if we're already at that offset
    if (offset == m_StreamOffset) return AP4_SUCCESS;

    // update the offset
    m_StreamOffset = offset;

    // update the key stream if necessary
    if (m_StreamOffset & 0xF) {
        UpdateKeyStream();
    }
    
    if (preroll != NULL) *preroll = 0;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::UpdateKeyStream
+---------------------------------------------------------------------*/
void
AP4_CtrStreamCipher::UpdateKeyStream()
{
    // setup counter offset bytes
    AP4_UI64 counter_offset = m_StreamOffset/AP4_CIPHER_BLOCK_SIZE;
    AP4_UI08 counter_offset_bytes[8];
    AP4_BytesFromUInt64BE(counter_offset_bytes, counter_offset);

    // compute the new counter
    unsigned int carry = 0;
    for (unsigned int i=0; i<m_CounterSize; i++) {
        unsigned int o = AP4_CIPHER_BLOCK_SIZE-1-i;
        unsigned int x = m_BaseCounter[o];
        unsigned int y = (i<8)?counter_offset_bytes[7-i]:0;
        unsigned int sum = x+y+carry;
        m_CBlock[o] = (AP4_UI08)(sum&0xFF);
        carry = ((sum >= 0x100)?1:0);
    }
    for (unsigned int i=m_CounterSize; i<AP4_CIPHER_BLOCK_SIZE; i++) {
        unsigned int o = AP4_CIPHER_BLOCK_SIZE-1-i;
        m_CBlock[o] = m_BaseCounter[o];
    }

    // compute the key block (x) from the counter block (c)
    m_BlockCipher->ProcessBlock(m_CBlock, m_XBlock);
}

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::ProcessBuffer
+---------------------------------------------------------------------*/
AP4_Result
AP4_CtrStreamCipher::ProcessBuffer(const AP4_UI08* in, 
                                   AP4_Size        in_size,
                                   AP4_UI08*       out, 
                                   AP4_Size*       out_size   /* = NULL */,
                                   bool            /* is_last_buffer */,
                                   AP4_UI64*       out_offset /* = NULL */)
{
    if (m_BlockCipher == NULL) return AP4_ERROR_INVALID_STATE;
    
    if (out_size != NULL && *out_size < in_size) {
        return AP4_ERROR_BUFFER_TOO_SMALL;
    }

    while (in_size) {
        // compute the number of bytes available in this chunk
        AP4_UI32 index = (AP4_UI32)(m_StreamOffset & (AP4_CIPHER_BLOCK_SIZE-1));
        AP4_UI32 chunk;

        // update the key stream if we are on a boundary
        if (index == 0) {
            UpdateKeyStream();
            chunk = AP4_CIPHER_BLOCK_SIZE;
        }

        // compute the number of bytes remaining in the chunk
        chunk = AP4_CIPHER_BLOCK_SIZE - index;
        if (chunk > in_size) chunk = in_size;

        // encrypt/decrypt the chunk
        AP4_UI08* x = &m_XBlock[index];
        for (AP4_UI32 i = 0; i < chunk; i++) {
            *out++ = *in++ ^ *x++;
        }

        // update offset and size
        m_StreamOffset += chunk;
        in_size -= chunk;
    }
    
    // nice thing about CTR mode 
    if (out_size != NULL)   *out_size   = in_size;
    if (out_offset != NULL) *out_offset = m_StreamOffset;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher::AP4_CbcStreamCipher
+---------------------------------------------------------------------*/
AP4_CbcStreamCipher::AP4_CbcStreamCipher(AP4_BlockCipher* block_cipher,
                                         CipherDirection  direction) :
    m_Direction(direction),
    m_StreamOffset(0),
    m_BlockCipher(block_cipher),
    m_Eos(false),
    m_PrerollByteCount(0)
{
    AP4_SetMemory(m_Iv, 0, AP4_CIPHER_BLOCK_SIZE);
    AP4_SetMemory(m_OutBlockCache, 0, AP4_CIPHER_BLOCK_SIZE);
}

/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher::~AP4_CbcStreamCipher
+---------------------------------------------------------------------*/
AP4_CbcStreamCipher::~AP4_CbcStreamCipher()
{
    delete m_BlockCipher;
}

/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher::SetIV
+---------------------------------------------------------------------*/
AP4_Result
AP4_CbcStreamCipher::SetIV(const AP4_UI08* iv)
{
    AP4_CopyMemory(m_Iv, iv, AP4_CIPHER_BLOCK_SIZE);
    m_StreamOffset = 0;
    m_Eos = false;
    AP4_CopyMemory(m_OutBlockCache, m_Iv, AP4_CIPHER_BLOCK_SIZE);
    m_PrerollByteCount = 0;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher::SetStreamOffset
+---------------------------------------------------------------------*/
AP4_Result
AP4_CbcStreamCipher::SetStreamOffset(AP4_UI64       offset,
                                     AP4_Cardinal*  preroll)
{
    // does not make sense for encryption
    if (m_Direction == AP4_StreamCipher::ENCRYPT) return AP4_ERROR_NOT_SUPPORTED;
    
    // check params
    if (preroll == NULL) return AP4_ERROR_INVALID_PARAMETERS;

    // obvious cases
    if (offset == m_StreamOffset) return AP4_SUCCESS;
    if (offset == 0) {
        *preroll = 0;
        return SetIV(m_Iv);
    }

    // other cases
    if (offset < AP4_CIPHER_BLOCK_SIZE) {
        // reset the IV to the output block cache
        AP4_CopyMemory(m_OutBlockCache, m_Iv, AP4_CIPHER_BLOCK_SIZE);
        m_PrerollByteCount = (AP4_Cardinal) offset;
    } else {
        m_PrerollByteCount = (AP4_Cardinal) (offset%AP4_CIPHER_BLOCK_SIZE 
                                             + AP4_CIPHER_BLOCK_SIZE);
    }
    
    *preroll = m_PrerollByteCount;
    m_StreamOffset = offset;
    return AP4_SUCCESS;
}


/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher::ProcessBuffer
+---------------------------------------------------------------------*/
AP4_Result
AP4_CbcStreamCipher::ProcessBuffer(const AP4_UI08* in, 
                                   AP4_Size        in_size,
                                   AP4_UI08*       out, 
                                   AP4_Size*       out_size,
                                   bool            is_last_buffer /* = false */,
                                   AP4_UI64*       out_offset     /* = NULL  */)
{
    // check the parameters
    if (out_size == NULL) return AP4_ERROR_INVALID_PARAMETERS; 
    
    // check the state
    if (m_BlockCipher == NULL || m_Eos) return AP4_ERROR_INVALID_STATE;
    if (is_last_buffer) m_Eos = true;
    
    AP4_UI64 start_block = m_StreamOffset/AP4_CIPHER_BLOCK_SIZE;
    AP4_UI64 end_block   = (m_StreamOffset+in_size-m_PrerollByteCount)/AP4_CIPHER_BLOCK_SIZE;
    AP4_UI32 blocks_needed = 0;
    // make sure we don't have a negative blocks_needed
    if (end_block>start_block) blocks_needed = (AP4_UI32)(end_block-start_block);

    if (m_Direction == ENCRYPT) {
        // compute how many blocks we will need to produce
        unsigned int padded_in_size = in_size;
        AP4_UI08     pad_byte = 0;
        if (is_last_buffer) {
            ++blocks_needed;
            pad_byte = AP4_CIPHER_BLOCK_SIZE-(AP4_UI08)((m_StreamOffset+in_size)%AP4_CIPHER_BLOCK_SIZE);
            padded_in_size += pad_byte;
        }
        if (*out_size < blocks_needed*AP4_CIPHER_BLOCK_SIZE) {
            *out_size = blocks_needed*AP4_CIPHER_BLOCK_SIZE;
            return AP4_ERROR_BUFFER_TOO_SMALL;
        }
        *out_size = blocks_needed*AP4_CIPHER_BLOCK_SIZE;

        unsigned int position = (unsigned int)(m_StreamOffset%AP4_CIPHER_BLOCK_SIZE);
        m_StreamOffset += in_size;
        for (unsigned int x=0; x<padded_in_size; x++) {
            if (x < in_size) {
                m_InBlockCache[position] = in[x] ^ m_OutBlockCache[position];
            } else {
                m_InBlockCache[position] = pad_byte ^ m_OutBlockCache[position]; 
            }
            if (++position == AP4_CIPHER_BLOCK_SIZE) {
                // encrypt and emit a block
                m_BlockCipher->ProcessBlock(m_InBlockCache, m_OutBlockCache);
                AP4_CopyMemory(out, m_OutBlockCache, AP4_CIPHER_BLOCK_SIZE);
                out += AP4_CIPHER_BLOCK_SIZE;
                position = 0;
            }
        }
    } else {
        // compute how many blocks we may produce
        if (*out_size < blocks_needed*AP4_CIPHER_BLOCK_SIZE) {
            *out_size = blocks_needed*AP4_CIPHER_BLOCK_SIZE;
            return AP4_ERROR_BUFFER_TOO_SMALL;
        }
        *out_size = blocks_needed*AP4_CIPHER_BLOCK_SIZE;
        
        // if we've just been seeked (SetStreamOffset), 
        if (m_PrerollByteCount > 0) {
            // we need at least m_PrerollByteCount to recover
            if (in_size < m_PrerollByteCount) return AP4_ERROR_NOT_ENOUGH_DATA;
            
            if (m_PrerollByteCount >= AP4_CIPHER_BLOCK_SIZE) {
                // fill the outblock cache with the first 16 bytes
                AP4_CopyMemory(m_OutBlockCache, in, AP4_CIPHER_BLOCK_SIZE);
                in += AP4_CIPHER_BLOCK_SIZE;
                m_PrerollByteCount -= AP4_CIPHER_BLOCK_SIZE;
                in_size -= AP4_CIPHER_BLOCK_SIZE;
            }

            AP4_ASSERT(m_PrerollByteCount < 16);
            
            // fill m_InBlockCache with the input for the remaining m_PrerollByteCount
            AP4_CopyMemory(m_InBlockCache, in, m_PrerollByteCount);
            in += m_PrerollByteCount;
            in_size -= m_PrerollByteCount;
            m_PrerollByteCount = 0;
        }
        

        unsigned int position = (unsigned int)(m_StreamOffset%AP4_CIPHER_BLOCK_SIZE);
        m_StreamOffset += in_size;
        for (unsigned int x=0; x<in_size; x++) {
            m_InBlockCache[position] = in[x];
            if (++position == AP4_CIPHER_BLOCK_SIZE) {
                // decrypt a block and emit the data
                m_BlockCipher->ProcessBlock(m_InBlockCache, out);
                for (unsigned int y=0; y<AP4_CIPHER_BLOCK_SIZE; y++) {
                    out[y] ^= m_OutBlockCache[y];
                }
                AP4_CopyMemory(m_OutBlockCache, m_InBlockCache, AP4_CIPHER_BLOCK_SIZE);
                out += AP4_CIPHER_BLOCK_SIZE;
                position = 0;
            }
        }
        if (is_last_buffer) {
            // check that we have fed an integral number of blocks
            if (m_StreamOffset%AP4_CIPHER_BLOCK_SIZE != 0) {
                *out_size = 0;
                return AP4_ERROR_INVALID_PARAMETERS;
            }

            AP4_UI08 pad_byte = out[-1];
            if (pad_byte > AP4_CIPHER_BLOCK_SIZE) {
                *out_size = 0;
                return AP4_ERROR_INVALID_FORMAT;
            }
            *out_size -= pad_byte;
        }
    }
    
    if (out_offset != NULL) *out_offset = start_block*AP4_CIPHER_BLOCK_SIZE;

    return AP4_SUCCESS;
}
