#! /usr/bin/env python

import sys
from mp4utils import DerivePlayReadyKey

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
            
    dkey = DerivePlayReadyKey(seed_bin, kid_bin, swap)
    print dkey.encode('hex')