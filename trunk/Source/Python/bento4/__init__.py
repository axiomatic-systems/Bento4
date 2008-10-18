from ctypes import *
import sys

if sys.platform == 'darwin':
    bento4dll = 'libBento4C.dylib'
else:
    raise "Unsupported Platform"

# library
lb4 = CDLL(bento4dll)

# type mapping
Ap4Result    = c_int
Ap4Flags     = c_uint
Ap4Mask      = c_uint
Ap4Cardinal  = c_uint
Ap4Ordinal   = c_uint
Ap4TimeStamp = c_uint
Ap4Duration  = c_ulong
Ap4UI32      = c_uint
Ap4SI32      = c_int
Ap4UI16      = c_ushort
Ap4SI16      = c_short
Ap4UI08      = c_ubyte
Ap4Byte      = c_byte
Ap4Size      = c_ulong
Ap4UI64      = c_ulonglong
Ap4SI64      = c_longlong
Ap4LargeSize = c_ulonglong
Ap4Offset    = c_longlong
Ap4Position  = c_ulonglong

