/*
 * AES Block cipher
 * (c) 2005-2010 Axiomatic Systems, LLC
 */

#ifndef _AP4_AES_BLOCK_CIPHER_H_
#define _AP4_AES_BLOCK_CIPHER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4Config.h"
#include "Ap4Protection.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
struct aes_ctx;

/*----------------------------------------------------------------------
|   AES constants
+---------------------------------------------------------------------*/
#define AP4_AES_BLOCK_SIZE  16
#define AP4_AES_KEY_LENGTH  16

/*----------------------------------------------------------------------
|   AP4_AesBlockCipher class
+---------------------------------------------------------------------*/
class AP4_AesBlockCipher : public AP4_BlockCipher
{
public:
    // constructor and destructor
    static AP4_Result Create(const AP4_UI08*      key, 
                             CipherDirection      direction,
                             CipherMode           mode,
                             const void*          mode_params,
                             AP4_AesBlockCipher*& cipher);
    virtual ~AP4_AesBlockCipher();

    virtual CipherDirection GetDirection() { return m_Direction; }
    
protected:
    // constructor
    AP4_AesBlockCipher(CipherDirection direction, 
                       CipherMode      mode,
                       aes_ctx*        context) :
        m_Direction(direction),
        m_Mode(mode),
        m_Context(context) {}

    // members
    CipherDirection m_Direction;
    CipherMode      m_Mode;
    aes_ctx*        m_Context;
};

#endif // _AP4_AES_BLOCK_CIPHER_H_ 
