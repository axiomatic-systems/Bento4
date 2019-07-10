/*****************************************************************
|
|    AP4 - Stream Cipher
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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4StreamCipher.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::AP4_CtrStreamCipher
+---------------------------------------------------------------------*/
AP4_CtrStreamCipher::AP4_CtrStreamCipher(AP4_BlockCipher* block_cipher,
                                         AP4_Size         counter_size) :
    m_StreamOffset(0),
    m_CounterSize(counter_size),
    m_CacheValid(false),
    m_BlockCipher(block_cipher)
{
    if (m_CounterSize > 16) m_CounterSize = 16;

    // reset the stream offset
    AP4_SetMemory(m_IV, 0, AP4_CIPHER_BLOCK_SIZE);
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
AP4_CtrStreamCipher::SetIV(const AP4_UI08* iv)
{
    if (iv) {
        AP4_CopyMemory(m_IV, iv, AP4_CIPHER_BLOCK_SIZE);
    } else {
        AP4_SetMemory(m_IV, 0, AP4_CIPHER_BLOCK_SIZE);
    }

    // for the stream offset back to 0
    m_CacheValid = false;
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
    m_CacheValid = false;
    m_StreamOffset = offset;

    // no preroll in CTR mode
    if (preroll != NULL) *preroll = 0;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::ComputeCounter
+---------------------------------------------------------------------*/
void
AP4_CtrStreamCipher::ComputeCounter(AP4_UI64 stream_offset, 
                                    AP4_UI08 counter_block[AP4_CIPHER_BLOCK_SIZE])
{
    // setup counter offset bytes
    AP4_UI64 counter_offset = stream_offset/AP4_CIPHER_BLOCK_SIZE;
    AP4_UI08 counter_offset_bytes[8];
    AP4_BytesFromUInt64BE(counter_offset_bytes, counter_offset);
    
    // compute the new counter
    unsigned int carry = 0;
    for (unsigned int i=0; i<m_CounterSize; i++) {
        unsigned int o = AP4_CIPHER_BLOCK_SIZE-1-i;
        unsigned int x = m_IV[o];
        unsigned int y = (i<8)?counter_offset_bytes[7-i]:0;
        unsigned int sum = x+y+carry;
        counter_block[o] = (AP4_UI08)(sum&0xFF);
        carry = ((sum >= 0x100)?1:0);
    }
    for (unsigned int i=m_CounterSize; i<AP4_CIPHER_BLOCK_SIZE; i++) {
        unsigned int o = AP4_CIPHER_BLOCK_SIZE-1-i;
        counter_block[o] = m_IV[o];
    }
}

/*----------------------------------------------------------------------
|   AP4_CtrStreamCipher::ProcessBuffer
+---------------------------------------------------------------------*/
AP4_Result
AP4_CtrStreamCipher::ProcessBuffer(const AP4_UI08* in, 
                                   AP4_Size        in_size,
                                   AP4_UI08*       out, 
                                   AP4_Size*       out_size   /* = NULL */,
                                   bool            /* is_last_buffer */)
{
    if (m_BlockCipher == NULL) return AP4_ERROR_INVALID_STATE;
    
    if (out_size != NULL && *out_size < in_size) {
        *out_size = in_size;
        return AP4_ERROR_BUFFER_TOO_SMALL;
    }

    // in CTR mode, the output is the same size as the input 
    if (out_size != NULL) *out_size = in_size;

    // deal with inputs not aligned on block boundaries
    if (m_StreamOffset%AP4_CIPHER_BLOCK_SIZE) {
        unsigned int cache_offset = (unsigned int)(m_StreamOffset%AP4_CIPHER_BLOCK_SIZE);
        if (!m_CacheValid) {
            // fill the cache
            AP4_UI08 block[AP4_CIPHER_BLOCK_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            AP4_UI08 counter_block[AP4_CIPHER_BLOCK_SIZE];
            ComputeCounter(m_StreamOffset-cache_offset, counter_block);
            AP4_Result result = m_BlockCipher->Process(block, AP4_CIPHER_BLOCK_SIZE, m_CacheBlock, counter_block);
            if (AP4_FAILED(result)) {
                if (out_size) *out_size = 0;
                return result;
            }
            m_CacheValid = true;
        }
        unsigned int partial = AP4_CIPHER_BLOCK_SIZE-cache_offset;
        if (partial > in_size) partial = in_size;
        for (unsigned int i=0; i<partial; i++) {
            out[i] = in[i]^m_CacheBlock[i+cache_offset];
        }

        // advance to the end of the partial block
        m_StreamOffset += partial;
        in             += partial;
        out            += partial;
        in_size        -= partial;
    }
    
    // process all the remaining bytes in the buffer
    if (in_size) {
        // the cache won't be valid anymore
        m_CacheValid = false;

        // compute the counter
        AP4_UI08 counter_block[AP4_CIPHER_BLOCK_SIZE];
        ComputeCounter(m_StreamOffset, counter_block);
        
        // process the data
        AP4_Result result = m_BlockCipher->Process(in, in_size, out, counter_block);
        if (AP4_FAILED(result)) {
            if (out_size) *out_size = 0;
            return result;
        }
        m_StreamOffset += in_size;
        return result;
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher::AP4_CbcStreamCipher
+---------------------------------------------------------------------*/
AP4_CbcStreamCipher::AP4_CbcStreamCipher(AP4_BlockCipher* block_cipher) :
    m_StreamOffset(0),
    m_OutputSkip(0),
    m_InBlockFullness(0),
    m_ChainBlockFullness(AP4_CIPHER_BLOCK_SIZE),
    m_BlockCipher(block_cipher),
    m_Eos(false)
{
    AP4_SetMemory(m_Iv, 0, AP4_CIPHER_BLOCK_SIZE);
    AP4_SetMemory(m_ChainBlock, 0, AP4_CIPHER_BLOCK_SIZE);
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
    AP4_CopyMemory(m_ChainBlock, m_Iv, AP4_CIPHER_BLOCK_SIZE);
    m_ChainBlockFullness = AP4_CIPHER_BLOCK_SIZE;
    m_InBlockFullness = 0;
    m_OutputSkip = 0;
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
    if (m_BlockCipher->GetDirection() == AP4_BlockCipher::ENCRYPT) {
        return AP4_ERROR_NOT_SUPPORTED;
    }
    
    // check params
    if (preroll == NULL) return AP4_ERROR_INVALID_PARAMETERS;

    // reset the end of stream flag
    m_Eos = false;
    
    // invalidate the chain block
    m_ChainBlockFullness = 0;
    
    // flush cached data
    m_InBlockFullness = 0;
    
    // compute the preroll
    if (offset < AP4_CIPHER_BLOCK_SIZE) {
        AP4_CopyMemory(m_ChainBlock, m_Iv, AP4_CIPHER_BLOCK_SIZE);
        m_ChainBlockFullness = AP4_CIPHER_BLOCK_SIZE;
        *preroll = (AP4_Cardinal)offset;
    } else {
        *preroll = (AP4_Cardinal) ((offset%AP4_CIPHER_BLOCK_SIZE) + AP4_CIPHER_BLOCK_SIZE);
    }
    
    m_StreamOffset = offset-*preroll;
    m_OutputSkip   = (AP4_Size)(offset%AP4_CIPHER_BLOCK_SIZE);
    return AP4_SUCCESS;
}


/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher::EncryptBuffer
+---------------------------------------------------------------------*/
AP4_Result
AP4_CbcStreamCipher::EncryptBuffer(const AP4_UI08* in, 
                                   AP4_Size        in_size,
                                   AP4_UI08*       out, 
                                   AP4_Size*       out_size,
                                   bool            is_last_buffer)
{
    // we don't do much checking here because this method is only called
    // from ProcessBuffer(), which does all the parameter checking
    
    // compute how many blocks we span
    AP4_UI64 start_block   = (m_StreamOffset-m_InBlockFullness)/AP4_CIPHER_BLOCK_SIZE;
    AP4_UI64 end_block     = (m_StreamOffset+in_size)/AP4_CIPHER_BLOCK_SIZE;
    AP4_UI32 blocks_needed = (AP4_UI32)(end_block-start_block);
 
    // compute how many blocks we will need to produce
    if (is_last_buffer) {
        ++blocks_needed;
    }
    if (*out_size < blocks_needed*AP4_CIPHER_BLOCK_SIZE) {
        *out_size = blocks_needed*AP4_CIPHER_BLOCK_SIZE;
        return AP4_ERROR_BUFFER_TOO_SMALL;
    }
    *out_size = blocks_needed*AP4_CIPHER_BLOCK_SIZE;

    // finish any incomplete block from a previous call
    unsigned int offset = (unsigned int)(m_StreamOffset%AP4_CIPHER_BLOCK_SIZE);
    AP4_ASSERT(m_InBlockFullness == offset);
    if (offset) {
        unsigned int chunk = AP4_CIPHER_BLOCK_SIZE-offset;
        if (chunk > in_size) chunk = in_size;
        for (unsigned int x=0; x<chunk; x++) {
            m_InBlock[x+offset] = in[x];
        }
        in                += chunk;
        in_size           -= chunk;
        m_StreamOffset    += chunk;        
        m_InBlockFullness += chunk;
        if (offset+chunk == AP4_CIPHER_BLOCK_SIZE) {
            // we have filled the input block, encrypt it
            AP4_Result result = m_BlockCipher->Process(m_InBlock, AP4_CIPHER_BLOCK_SIZE, out, m_ChainBlock);
            AP4_CopyMemory(m_ChainBlock, out, AP4_CIPHER_BLOCK_SIZE);
            m_InBlockFullness = 0;
            if (AP4_FAILED(result)) {
                *out_size = 0;
                return result;
            }
            out += AP4_CIPHER_BLOCK_SIZE;
        }
    }
    
    // encrypt the whole blocks
    unsigned int block_count = in_size/AP4_CIPHER_BLOCK_SIZE;
    if (block_count) {
        AP4_ASSERT(m_InBlockFullness == 0);
        AP4_UI32 blocks_size = block_count*AP4_CIPHER_BLOCK_SIZE;
        AP4_Result result = m_BlockCipher->Process(in, blocks_size, out, m_ChainBlock);
        AP4_CopyMemory(m_ChainBlock, out+blocks_size-AP4_CIPHER_BLOCK_SIZE, AP4_CIPHER_BLOCK_SIZE);
        if (AP4_FAILED(result)) {
            *out_size = 0;
            return result;
        }
        in             += blocks_size;
        out            += blocks_size;
        in_size        -= blocks_size;
        m_StreamOffset += blocks_size;
    }
    
    // deal with what's left
    if (in_size) {
        AP4_ASSERT(in_size < AP4_CIPHER_BLOCK_SIZE);
        for (unsigned int x=0; x<in_size; x++) {
            m_InBlock[x+m_InBlockFullness] = in[x];
        }
        m_InBlockFullness += in_size;
        m_StreamOffset    += in_size;
    }
    
    // pad if needed 
    if (is_last_buffer) {
        AP4_ASSERT(m_InBlockFullness == m_StreamOffset%AP4_CIPHER_BLOCK_SIZE);
        AP4_UI08 pad_byte = AP4_CIPHER_BLOCK_SIZE-(AP4_UI08)(m_StreamOffset%AP4_CIPHER_BLOCK_SIZE);
        for (unsigned int x=AP4_CIPHER_BLOCK_SIZE-pad_byte; x<AP4_CIPHER_BLOCK_SIZE; x++) {
            m_InBlock[x] = pad_byte;
        }
        AP4_Result result = m_BlockCipher->Process(m_InBlock, AP4_CIPHER_BLOCK_SIZE, out, m_ChainBlock);
        AP4_CopyMemory(m_ChainBlock, out, AP4_CIPHER_BLOCK_SIZE);
        m_InBlockFullness = 0;
        if (AP4_FAILED(result)) {
            *out_size = 0;
            return result;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CbcStreamCipher::DecryptBuffer
+---------------------------------------------------------------------*/
AP4_Result
AP4_CbcStreamCipher::DecryptBuffer(const AP4_UI08* in, 
                                   AP4_Size        in_size,
                                   AP4_UI08*       out, 
                                   AP4_Size*       out_size,
                                   bool            is_last_buffer)
{
    // we don't do much checking here because this method is only called
    // from ProcessBuffer(), which does all the parameter checking
    
    // deal chain-block buffering
    if (m_ChainBlockFullness != AP4_CIPHER_BLOCK_SIZE) {
        unsigned int needed = AP4_CIPHER_BLOCK_SIZE-m_ChainBlockFullness;
        unsigned int chunk = (in_size > needed) ? needed : in_size;
        AP4_CopyMemory(&m_ChainBlock[m_ChainBlockFullness], in, chunk);
        in_size              -= chunk;
        in                   += chunk;
        m_ChainBlockFullness += chunk;
        m_StreamOffset       += chunk;
        if (m_ChainBlockFullness != AP4_CIPHER_BLOCK_SIZE) {
            // not enough to continue
            *out_size = 0;
            return AP4_SUCCESS;
        }
    }
    AP4_ASSERT(m_ChainBlockFullness == AP4_CIPHER_BLOCK_SIZE);
        
    // compute how many blocks we span
    AP4_UI64 start_block   = (m_StreamOffset-m_InBlockFullness)/AP4_CIPHER_BLOCK_SIZE;
    AP4_UI64 end_block     = (m_StreamOffset+in_size)/AP4_CIPHER_BLOCK_SIZE;
    AP4_UI32 blocks_needed = (AP4_UI32)(end_block-start_block);

    // compute how many blocks we will need to produce
    if (*out_size < blocks_needed*AP4_CIPHER_BLOCK_SIZE) {
        *out_size = blocks_needed*AP4_CIPHER_BLOCK_SIZE;
        return AP4_ERROR_BUFFER_TOO_SMALL;
    }
    *out_size = blocks_needed*AP4_CIPHER_BLOCK_SIZE;
    if (blocks_needed && m_OutputSkip) *out_size -= m_OutputSkip;

    // shortcut
    if (in_size == 0) return AP4_SUCCESS;
    
    // deal with in-block buffering
    // NOTE: if we have bytes to skip on output, always use the in-block buffer for
    // the first block, even if we're aligned, this makes the code simpler
    AP4_ASSERT(m_InBlockFullness < AP4_CIPHER_BLOCK_SIZE);
    if (m_OutputSkip || m_InBlockFullness) {
        unsigned int needed = AP4_CIPHER_BLOCK_SIZE-m_InBlockFullness;
        unsigned int chunk = (in_size > needed) ? needed : in_size;
        AP4_CopyMemory(&m_InBlock[m_InBlockFullness], in, chunk);
        in_size           -= chunk;
        in                += chunk;
        m_InBlockFullness += chunk;
        m_StreamOffset    += chunk;
        if (m_InBlockFullness != AP4_CIPHER_BLOCK_SIZE) {
            // not enough to continue
            *out_size = 0;
            return AP4_SUCCESS;
        }
        AP4_UI08 out_block[AP4_CIPHER_BLOCK_SIZE];
        AP4_Result result = m_BlockCipher->Process(m_InBlock, AP4_CIPHER_BLOCK_SIZE, out_block, m_ChainBlock);
        m_InBlockFullness = 0;
        if (AP4_FAILED(result)) {
            *out_size = 0;
            return result;
        }
        AP4_CopyMemory(m_ChainBlock, m_InBlock, AP4_CIPHER_BLOCK_SIZE);
        if (m_OutputSkip) {
            AP4_ASSERT(m_OutputSkip < AP4_CIPHER_BLOCK_SIZE);
            AP4_CopyMemory(out, &out_block[m_OutputSkip], AP4_CIPHER_BLOCK_SIZE-m_OutputSkip);
            out += AP4_CIPHER_BLOCK_SIZE-m_OutputSkip;
            m_OutputSkip = 0;
        } else {
            AP4_CopyMemory(out, out_block, AP4_CIPHER_BLOCK_SIZE);
            out += AP4_CIPHER_BLOCK_SIZE;
        }
    }
    AP4_ASSERT(m_InBlockFullness == 0);
    AP4_ASSERT(m_OutputSkip == 0);
    
    // process full blocks
    unsigned int block_count = in_size/AP4_CIPHER_BLOCK_SIZE;
    if (block_count) {
        AP4_UI32 blocks_size = block_count*AP4_CIPHER_BLOCK_SIZE;
        AP4_Result result = m_BlockCipher->Process(in, blocks_size, out, m_ChainBlock);
        AP4_CopyMemory(m_ChainBlock, in+blocks_size-AP4_CIPHER_BLOCK_SIZE, AP4_CIPHER_BLOCK_SIZE);
        if (AP4_FAILED(result)) {
            *out_size = 0;
            return result;
        }
        in             += blocks_size;
        out            += blocks_size;
        in_size        -= blocks_size;
        m_StreamOffset += blocks_size;
    }

    // buffer partial block leftovers
    if (in_size) {
        AP4_ASSERT(in_size < AP4_CIPHER_BLOCK_SIZE);
        AP4_CopyMemory(m_InBlock, in, in_size);
        m_InBlockFullness = in_size;
        m_StreamOffset   += in_size;
    }
    
    // deal with padding
    if (is_last_buffer) {
        AP4_UI08 pad_byte = *(out-1);
        if (pad_byte > AP4_CIPHER_BLOCK_SIZE ||
            *out_size < pad_byte) {
            *out_size = 0;
            return AP4_ERROR_INVALID_FORMAT;
        }
        *out_size -= pad_byte;
    }
    
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
                                   bool            is_last_buffer)
{
    // check the parameters
    if (out_size == NULL) return AP4_ERROR_INVALID_PARAMETERS; 
    
    // check the state
    if (m_BlockCipher == NULL || m_Eos) {
        *out_size = 0;
        return AP4_ERROR_INVALID_STATE;
    }
    if (is_last_buffer) m_Eos = true;
    
    if (m_BlockCipher->GetDirection() == AP4_BlockCipher::ENCRYPT) {
        return EncryptBuffer(in, in_size, out, out_size, is_last_buffer);
    } else {
        return DecryptBuffer(in, in_size, out, out_size, is_last_buffer);
    }
}

/*----------------------------------------------------------------------
|   AP4_PatternStreamCipher
+---------------------------------------------------------------------*/
AP4_PatternStreamCipher::AP4_PatternStreamCipher(AP4_StreamCipher* cipher,
                                                 AP4_UI08          crypt_byte_block,
                                                 AP4_UI08 skip_byte_block) :
    m_Cipher(cipher),
    m_CryptByteBlock(crypt_byte_block),
    m_SkipByteBlock(skip_byte_block),
    m_StreamOffset(0)
{
}

/*----------------------------------------------------------------------
|   AP4_PatternStreamCipher::~AP4_PatternStreamCipher
+---------------------------------------------------------------------*/
AP4_PatternStreamCipher::~AP4_PatternStreamCipher()
{
    delete m_Cipher;
}

/*----------------------------------------------------------------------
|   AP4_PatternStreamCipher::SetStreamOffset
+---------------------------------------------------------------------*/
AP4_Result
AP4_PatternStreamCipher::SetStreamOffset(AP4_UI64      /*offset*/,
                                         AP4_Cardinal* /*preroll*/)
{
    return AP4_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   AP4_PatternStreamCipher
+---------------------------------------------------------------------*/
AP4_Result
AP4_PatternStreamCipher::ProcessBuffer(const AP4_UI08* in,
                                       AP4_Size        in_size,
                                       AP4_UI08*       out,
                                       AP4_Size*       out_size,
                                       bool            /* is_last_buffer */)
{
    // set default return values
    *out_size = 0;
    
    // check that the range is block-aligned (required by the spec for pattern encryption)
    if (m_StreamOffset % 16) return AP4_ERROR_INVALID_FORMAT;
    
    // compute where we are in the pattern
    unsigned int pattern_span = m_CryptByteBlock+m_SkipByteBlock;
    unsigned int block_position   = (unsigned int)(m_StreamOffset/16);
    unsigned int pattern_position = block_position % pattern_span;

    // process the range
    while (*out_size < in_size) {
        unsigned int crypt_size = 0;
        unsigned int skip_size  = m_SkipByteBlock*16;
        if (pattern_position < m_CryptByteBlock) {
            // in the encrypted part
            crypt_size = (m_CryptByteBlock-pattern_position)*16;
        } else {
            // in the skipped part
            skip_size = pattern_span-pattern_position;
        }
        
        // clip
        AP4_Size remain = in_size-*out_size;
        if (crypt_size > remain) {
            crypt_size = 16*(remain/16);
            skip_size  = remain-crypt_size;
        }
        if (crypt_size+skip_size > remain) {
            skip_size = remain-crypt_size;
        }
        
        // encrypted part
        if (crypt_size) {
            AP4_Size in_chunk_size  = crypt_size;
            AP4_Size out_chunk_size = crypt_size;
            
            AP4_Result result = m_Cipher->ProcessBuffer(in, in_chunk_size, out, &out_chunk_size);
            if (AP4_FAILED(result)) return result;
            // check that we got back what we expectected
            if (out_chunk_size != in_chunk_size) {
                return AP4_ERROR_INTERNAL;
            }
            in             += crypt_size;
            out            += crypt_size;
            *out_size      += crypt_size;
            m_StreamOffset += crypt_size;
        }
        
        // skipped part
        if (skip_size) {
            AP4_CopyMemory(out, in, skip_size);
            in             += skip_size;
            out            += skip_size;
            *out_size      += skip_size;
            m_StreamOffset += skip_size;
        }
        
        // we're now at the start of a new pattern
        pattern_position = 0;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PatternStreamCipher
+---------------------------------------------------------------------*/
AP4_Result
AP4_PatternStreamCipher::SetIV(const AP4_UI08* iv)
{
    m_StreamOffset = 0;
    return m_Cipher->SetIV(iv);
}

/*----------------------------------------------------------------------
|   AP4_PatternStreamCipher
+---------------------------------------------------------------------*/
const AP4_UI08*
AP4_PatternStreamCipher::GetIV()
{
    return m_Cipher->GetIV();
}
