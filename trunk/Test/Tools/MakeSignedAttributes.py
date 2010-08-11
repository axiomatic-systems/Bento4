import struct
import sys
import subprocess
import tempfile
import os
import hashlib

"""
Example usage:
python MakeSignedAttributes.py attr.priv attr.cert 1:2:http://blabla.com/foo.xml 2:2:http://blabla.com/bar.xml
"""

if len(sys.argv) < 4:
    print "usage: MakeSignedAttributes.py <private-key-filename> <cert-filename> <rurl> [<rurl>...]"
    print "    where <rurl> is expressed as: <type>:<method>:<url>. <type> and <method> are integers, and <url> is a string"
    sys.exit(1)
    
private_key_filename = sys.argv[1]
cert_filename = sys.argv[2]
rurls = sys.argv[3:]

# read the cert file
cert = open(cert_filename, "rb").read()

# parse the rurls
rurl_atoms = []
for rurl in rurls:
    (type, method, url) = rurl.split(':', 2)
    if not (type and method and url):
        raise Exception('invalid <rurl> syntax')
    rurl_atom = struct.pack('BB', int(type), int(method))+url+chr(0)
    rurl_atoms.append(struct.pack('>I', 12+len(rurl_atom))+'rurl'+struct.pack('>I', 0)+rurl_atom)
    
# compute the snga container
snga = ''.join(rurl_atoms)
snga_atom = struct.pack('>I', 8+len(snga))+'snga'+snga

# openssl rsautl -sign -inkey attr.priv -in log.txt
sha1 = hashlib.sha1()
sha1.update(snga_atom)
digest = sha1.digest()
(digest_file_fd, digest_filename) = tempfile.mkstemp()
os.write(digest_file_fd, digest)
os.close(digest_file_fd)
signature = subprocess.Popen(['openssl', 'rsautl', '-sign', '-inkey', private_key_filename, '-in', digest_filename], stdout=subprocess.PIPE).communicate()[0]

asig = struct.pack('>I', 1)+signature
asig_atom = struct.pack('>I', 12+len(asig))+'asig'+struct.pack('>I', 0)+asig

cert_atom = struct.pack('>I', 12+len(cert))+'cert'+struct.pack('>I', 0)+cert

print (snga_atom+asig_atom+cert_atom).encode("hex")
