#! /usr/bin/env python

import sys
import hashlib

def derive_key(seed, kid):
    if len(seed) < 30:
        raise Exception('seed must be  >= 30 bytes')
    if len(kid) != 16:
        raise Exception('kid must be 16 bytes')
    
    seed = seed[:30]

    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha_A = [ord(x) for x in sha.digest()]
    
    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha.update(seed)
    sha_B = [ord(x) for x in sha.digest()]
    
    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha.update(seed)
    sha.update(kid)
    sha_C = [ord(x) for x in sha.digest()]
        
    content_key = ""
    for i in range(16):
        content_key += chr(sha_A[i] ^ sha_A[i+16] ^ sha_B[i] ^ sha_B[i+16] ^ sha_C[i] ^ sha_C[i+16])

    return content_key.encode('hex')

###########################    
if __name__ == '__main__':
    if len(sys.argv) != 3 and len(sys.argv) != 4:
        sys.stderr.write('ERROR: invalid arguments\n')
        sys.stderr.write('Usage: pr-derive-key.py [--no-swap] <seed-base64> <kid-hex>\n')
        sys.exit(1)
        
    swap = True
    if (len(sys.argv) == 4):
        if sys.argv[1] == '--no-swap':
            swap = False
        else:
            raise Exception('unknown command line option "'+sys.argv[2]+'"')
    
    (seed_base64, kid_hex) = sys.argv[-2:]
    kid_hex = kid_hex.replace(' ', '')
    kid_hex = kid_hex.replace('-', '')
    
    seed_bin = seed_base64.decode('base64')
    kid_bin = kid_hex.decode('hex')
    
    if swap:
        kid_bin = kid_bin[3]+kid_bin[2]+kid_bin[1]+kid_bin[0]+kid_bin[5]+kid_bin[4]+kid_bin[7]+kid_bin[6]+kid_bin[8:]
        
    dkey = derive_key(seed_bin, kid_bin)
    print dkey