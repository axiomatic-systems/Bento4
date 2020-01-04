import importlib.util
spec = importlib.util.spec_from_file_location("aes", "../../Source/Python/utils/aes.py")
aes = importlib.util.module_from_spec(spec)
spec.loader.exec_module(aes)


def test_encrypt():
    msg = bytes.fromhex("00112233445566778899aabbccddeeff")
    key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
    encrypted = aes.rijndael(key).encrypt(msg).hex()
    assert encrypted == "69c4e0d86a7b0430d8cdb78070b4c55a"

def test_decrypt():
    msg = bytes.fromhex("69c4e0d86a7b0430d8cdb78070b4c55a")
    key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
    decrypted = aes.rijndael(key).decrypt(msg).hex()
    assert decrypted == "00112233445566778899aabbccddeeff"
