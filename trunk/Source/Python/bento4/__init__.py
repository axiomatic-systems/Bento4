from ctypes import CDLL
import sys

if sys.platform == 'darwin':
    bento4dll = 'libBento4C.dylib'
else:
    raise "Unsupported Platform"

lb4 = CDLL(bento4dll)
