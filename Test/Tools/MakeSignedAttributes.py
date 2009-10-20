import struct
import sys

if len(sys.argv) != 3:
    print "usage: MakeSignedAttributes.py <private-key-filename> <cert-filename>"
    sys.exit(1)
    
snga = "hello"
snga_atom = struct.pack('>I', 8+len(snga))+'snga'+snga

signature = "blabla"
asig = struct.pack('>I', 1)+signature
asig_atom = struct.pack('>I', 12+len(asig))+'asig'+struct.pack('>I', 0)+asig

cert = "certtttt"
cert_atom = struct.pack('>I', 12+len(cert))+'cert'+struct.pack('>I', 0)+cert

print (snga_atom+asig_atom+cert_atom).encode("hex")
