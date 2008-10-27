from ctypes import c_int, c_double, c_char_p, pointer
from bento4 import *
from bento4.errors import check_result

class StreamException(Exception):
    
    def __init__(self, code, msg=''):
        self.code = code
        self.msg  = msg


class ByteStream(object):
    """abstract ByteStream class"""
    
    def __init__(self, bt4stream, add_ref=False):
        self.bt4stream = bt4stream
        if add_ref:
            lb4.AP4_MemoryStream_AddReference(self.bt4stream)
    
    @property    
    def size(self):
        v = Ap4LargeSize()
        f = lb4.AP4_ByteStream_GetSize
        f.restype = check_result
        f(self.bt4stream, byref(v))
        return v.value
        
    def read_partial(self, bytes_to_read):
        bytes_read = Ap4Size()
        p = create_string_buffer(bytes_to_read)
        f = lb4.AP4_ByteStream_ReadPartial
        f.restype = check_result
        f(self.bt4stream, p, Ap4Size(bytes_to_read), byref(bytes_read))
        return (p.raw, bytes_read.value)
        
    def read(self, bytes_to_read):
        p = create_string_buffer(bytes_to_read)
        f = lb4.AP4_ByteStream_Read
        f.restype = check_result
        f(self.bt4stream, p, Ap4Size(bytes_to_read))
        return p.raw
        
    def read_double(self):
        v = c_double()
        f = lb4.AP4_ByteStream_ReadDouble
        f.restype = check_result
        f(self.bt4stream, byref(v))
        return v.value
        
    def read_ui64(self):
        v = Ap4UI64()
        f = lb4.AP4_ByteStream_ReadUI64
        f.restype = check_result
        f(self.bt4stream, byref(v))
        return v.value
        
    def read_ui32(self):
        v = Ap4UI32()
        f = lb4.AP4_ByteStream_ReadUI32
        f.restype = check_result
        f(self.bt4stream, byref(v))
        return v.value
    
    def read_ui24(self):
        v = Ap4UI32()
        f = lb4.AP4_ByteStream_ReadUI24
        f.restype = check_result
        f(self.bt4stream, byref(v))
        return v.value
    
    def read_ui16(self):
        v = Ap4UI16()
        f = lb4.AP4_ByteStream_ReadUI16
        f.restype = check_result
        f(self.bt4stream, byref(v))
        return v.value
    
    def read_ui08(self):
        v = Ap4UI08()
        f = lb4.AP4_ByteStream_ReadUI08
        f.restype = check_result
        f(self.straem, byref(v))
        return v.value

    def read_string(self, length):
        p = create_string_buffer(length+1)
        f = lb4.AP4_ByteStream_ReadString
        f.restype = check_result
        f(self.bt4stream, p, Ap4Size(length+1))
        return p.value
    
    def write_partial(self, buffer):
        bytes_written = Ap4Size()
        f = lb4.AP4_ByteStream_WritePartial
        f.restype = check_result
        f(self.bt4stream, c_char_p(buffer),
          Ap4Size(len(buffer)),byref(bytes_written))
        return bytes_written

    def write(self, buffer):
        f = lb4.AP4_ByteStream_Write
        f.restype = check_result
        f(self.bt4stream, c_char_p(buffer), Ap4Size(len(buffer)))
        
        
    def write_double(self, value):
        f = lb4.AP4_ByteStream_WriteDouble
        f.restype = check_result
        f(self.bt4stream, c_double(value))
        
    def write_ui64(self, value):
        f = lb4.AP4_ByteStream_WriteUI64
        f.restype = check_result
        f(self.bt4stream, Ap4UI64(value))
        
    def write_ui32(self, value):
        f = lb4.AP4_ByteStream_WriteUI32
        f.restype = check_result
        f(self.bt4stream, Ap4UI32(value))
                                              
    def write_ui24(self, value):
        f = lb4.AP4_ByteStream_WriteUI24
        f.restype = check_result
        f(self.bt4stream, Ap4UI32(value))
        
    def write_ui16(self, value):
        f = lb4.AP4_ByteStream_WriteUI16
        f.restype = check_result
        f(self.bt4stream, Ap4UI16(value))
        
    def write_ui08(self, value):
        f = lb4.AP4_ByteStream_WriteUI08
        f.restype = check_result
        f(self.bt4stream, Ap4UI08(value))
         
    def write_string(self, value):
        f = lb4.AP4_ByteStream_Write
        f.restype = check_result
        f(self.bt4stream, c_char_p(buffer))
        
    def copy_to(self, receiver, size):
        f = lb4.AP4_ByteStream_CopyTo
        f.restype = check_result
        f(self.bt4stream, receiver.stream, AP4_Size(size))
        
    def seek(self, position):
        f = lb4.AP4_ByteStream_Seek
        f.restype = check_result
        f(self.bt4stream, Ap4Position(position))
        
    def flush(self):
        f = lb4.AP4_ByteStream_Flush
        f.restype = check_result
        f(self.bt4stream)
        
    def tell(self):
        pos = Ap4Position()
        f = lb4.AP4_ByteStream_Tell
        f.restype = check_result
        f(self.bt4stream, byref(pos))
        return pos.value
    
    def __del__(self):
        lb4.AP4_ByteStream_Release(self.bt4stream)
        
    
class MemoryByteStream(ByteStream):
    
    @staticmethod
    def from_buffer(buffer):
        """Factory method"""
        buffer_size = Ap4Size(len(buffer))
        bt4stream   = lb4.AP4_MemoryByteStream_FromBuffer(c_char_p(buffer),
                                                          buffer_size)
        return MemoryByteStream(bt4stream=bt4stream)
        
    
    def __init__(self, size=0, bt4stream=None):
        if bt4stream is None:
            bt4stream = lb4.AP4_ByteStream_Create(Ap4Size(size))
        super(MemoryByteStream, self).__init__(bt4stream)
        
        
class FileByteStream(ByteStream):
    MODE_READ       = 0
    MODE_WRITE      = 1
    MODE_READ_WRITE = 2
    
    def __init__(self, name, mode):
        result = Ap4Result(0)
        f = lb4.AP4_FileByteStream_Create
        bt4stream = f(c_char_p(name), c_int(mode), byref(result))
        check_result(result)
        super(FileByteStream, self).__init__(bt4stream)
        
    
