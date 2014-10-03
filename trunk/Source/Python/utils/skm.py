import aes
import urllib2
import os
import hashlib
import json

KEKID_CONSTANT_1 = "KEKID_1"

def WrapKey(key, kek):
    if len(key) > 16:
        # assume hex
        key = key.decode('hex')
    if len(kek) > 16:
        # assume hex
        kek = kek.decode('hex')

    if (len(key)% 8) or (len(kek) % 8):
        raise Exception('key and kek must be a multiple of 64 bits')

    # create a cipher with kek
    cipher = aes.rijndael(kek)

    # Inputs:      Plaintext, n 64-bit values {P1, P2, ..., Pn}, and
    # Key, K (the KEK).
    # Outputs:     Ciphertext, (n+1) 64-bit values {C0, C1, ..., Cn}.
    n = len(key)/8;
    if n < 1: 
        raise Exception('key too short')

    # 1) Initialize variables.
    #
    #    Set A = IV, an initial value (see 2.2.3)
    #      For i = 1 to n
    #      R[i] = P[i]
    A = 'A6A6A6A6A6A6A6A6'.decode('hex');
    R = [key[i*8:(i+1)*8] for i in range(n)]

    # 2) Calculate intermediate values.
    #
    #    For j = 0 to 5
    #      For i=1 to n
    #        B = AES(K, A | R[i])
    #        A = MSB(64, B) ^ t where t = (n*j)+i
    #        R[i] = LSB(64, B)
    for j in range(6):
        for i in range(n):
            B = cipher.encrypt(A+R[i])
            A = B[0:7]+chr(ord(B[7]) ^ ((n*j)+i+1))
            R[i] = B[8:16]

    # 3) Output the results.
    #
    #    Set C[0] = A
    #    For i = 1 to n
    #      C[i] = R[i]
    return ''.join([A]+R)

def UnwrapKey(key, kek):
    if len(key) > 32:
        # assume hex
        key = key.decode('hex')
    if len(kek) > 16:
        # assume hex
        kek = kek.decode('hex')

    if (len(key)% 8) or (len(kek) % 8):
        raise Exception('key and kek must be a multiple of 64 bits')

    # create a de-cipher with kek
    decipher = aes.rijndael(kek)

    # Inputs:  Ciphertext, (n+1) 64-bit values {C0, C1, ..., Cn}, and
    # Key, K (the KEK).
    # Outputs: Plaintext, n 64-bit values {P0, P1, K, Pn}.
    n = len(key)/8 - 1;
    if n < 1:
        raise Exception('wrapped key too short');

    # 1) Initialize variables.
    #
    #    Set A = C[0]
    #    For i = 1 to n
    #      R[i] = C[i]
    A = key[0:8]
    R = [key[(i+1)*8:(i+2)*8] for i in range(n)]

    # 2) Compute intermediate values.
    #
    #    For j = 5 to 0
    #     For i = n to 1
    #       B = AES-1(K, (A ^ t) | R[i]) where t = n*j+i
    #       A = MSB(64, B)
    #       R[i] = LSB(64, B)
    for j in range(5, -1, -1):
        for i in range(n-1, -1, -1):
            B = decipher.decrypt(A[0:7] + chr(ord(A[7])^((n*j)+i+1))+R[i])
            A = B[0:8]
            R[i] = B[8:16]

    # 3) Output results.
    #
    #    If A is an appropriate initial value (see 2.2.3),
    #    Then
    #      For i = 1 to n
    #        P[i] = R[i]
    #    Else
    #      Return an error
    for i in range(8):
        if ord(A[i]) != 0xA6:
            raise Exception('invalid/corrupted wrapped key or wrong kek')
    return ''.join(R)

def ComputeKekId(kek):
    if len(kek) > 16:
        kek = kek.decode('hex')
    sha1 = hashlib.sha1()
    sha1.update(KEKID_CONSTANT_1)
    sha1.update(kek)
    return '#1.'+sha1.digest()[0:16].encode('hex')

def ResolveKey(options, spec):
    if '#' in spec:
        (base_url, spec_params_str) = spec.split('#', 1)
        spec_params_list = spec_params_str.split('&')
        spec_params = dict([tuple(spec_param.split('=', 1)) for spec_param in spec_params_list])
    else:
        base_url = spec
        spec_params = {}

    skm_mode = spec_params.get('mode', 'auto')
    skm_kek  = spec_params.get('kek', None)
    key = None
    key_object = {}
    for name in ['kid', 'kekId', 'info', 'contentId']:
        if name in spec_params:
            key_object[name] = spec_params[name]

    # soft-import the Requests module
    try:
        import requests
    except:
        raise Exception('"Requests" python module not installed. Please install it (use "pip install requests" or "easy_install requests" in a command shell, or consult the "Requests" documentation at http://docs.python-requests.org/en/latest/)')

    if options.debug:
        print 'Key Object Input:', key_object

    if skm_mode == 'get':
        if 'kid' not in spec_params:
            raise Exception('kid parameter must be specified when using "get" mode')
        kid = spec_params['kid']
        if '?' in base_url:
            (base_url_path, base_url_query) = tuple(base_url.split('?', 1))
            base_url_query = '?'+base_url_query
        else:
            base_url_path = base_url
            base_url_query = ''
        if base_url_path.endswith('/'):
            base_url_path = base_url_path[:-1]
        base_url = base_url_path+'/'+kid+base_url_query

        if options.debug:
            print 'Request:', base_url

        response = requests.get(base_url)
    elif skm_mode == 'auto':
        if skm_kek:
            # generate a key locally and wrap it
            key = os.urandom(16)
            key_object['ek'] = WrapKey(key, skm_kek).encode('hex')
            if 'kekId' not in key_object:
                key_object['kekId'] = ComputeKekId(skm_kek)
            if options.verbose:
                print 'Generating key locally'

        if options.debug:
            print 'Request:', base_url, json.dumps(key_object)
            
        response = requests.post(base_url, headers={'content-type': 'application/json'}, data=json.dumps(key_object))
    else:
        raise Exception('Unsupported SKM query mode')

    if response.status_code != 200 and response.status_code != 201:
        raise Exception('HTTP error while getting key: '+str(response.status_code)+', '+response.text)

    if options.debug:
        print 'Response:', response.text
    response_json = json.loads(response.text)
    kid = response_json['kid']
    if skm_kek and ('ek' in response_json):
        received_key = UnwrapKey(response_json['ek'], skm_kek)
        if options.verbose:
            if key and (key != received_key):
                print 'Locally generated key was ignored because a key with the same KID already existed on the server'
        key = received_key

    if not key:
        key = response_json['k']
    else:
        key = key.encode('hex')

    return (kid, key)

