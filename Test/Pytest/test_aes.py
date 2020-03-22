import aes

def test_ecb_encrypt():
    msg = bytes.fromhex("00112233445566778899aabbccddeeff")
    key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
    encrypted = aes.rijndael(key).encrypt(msg).hex()
    assert encrypted == "69c4e0d86a7b0430d8cdb78070b4c55a"

def test_ecb_decrypt():
    msg = bytes.fromhex("69c4e0d86a7b0430d8cdb78070b4c55a")
    key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
    decrypted = aes.rijndael(key).decrypt(msg).hex()
    assert decrypted == "00112233445566778899aabbccddeeff"

def test_cbc_encrypt():
    msg = bytes.fromhex("00112233445566778899aabbccddee")
    key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
    iv  = bytes.fromhex("00112233445566778899aabbccddeeff")
    encrypted = aes.cbc_encrypt(msg, key, iv).hex()
    assert encrypted == "62cc02dafc6482edf9f2adca6abbb3d2"

def test_cbc_decrypt():
    msg = bytes.fromhex("62cc02dafc6482edf9f2adca6abbb3d2")
    key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
    iv  = bytes.fromhex("00112233445566778899aabbccddeeff")
    decrypted = aes.cbc_decrypt(msg, key, iv).hex()
    assert decrypted == "00112233445566778899aabbccddee"
