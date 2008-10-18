from ctypes import *
from bento4 import *

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
        res = lb4.AP4_ByteStream_GetSize(self.bt4stream, byref(v))
        if res != 0:
            raise StreamException(res)
        return v.value
        
    def read_partial(self, bytes_to_read):
        bytes_read = Ap4Size()
        p = create_string_buffer(bytes_to_read)
        res = lb4.AP4_ByteStream_ReadPartial(self.bt4stream, p,
                                             Ap4Size(bytes_to_read),
                                             byref(bytes_read))
        if res != 0:
            raise StreamException(res)
        return (p.raw, bytes_read.value)
        
    def read(self, bytes_to_read):
        p = create_string_buffer(bytes_to_read)
        res = lb4.AP4_ByteStream_Read(self.bt4stream, p,
                                      Ap4Size(bytes_to_read))
        if res != 0:
            raise StreamException(res)
        return p.raw
        
    def read_double(self):
        v = c_double()
        res = lb4.AP4_ByteStream_ReadDouble(self.bt4stream, byref(v))
        if res != 0:
            raise StreamException(res)
        return v.value
    
    def read_ui64(self):
        v = Ap4UI64()
        res = lb4.AP4_ByteStream_ReadUI64(self.bt4stream, byref(v))
        if res != 0:
            raise StreamException(res)
        return v.value
    
    def read_ui32(self):
        v = Ap4UI32()
        res = lb4.AP4_ByteStream_ReadUI32(self.bt4stream, byref(v))
        if res != 0:
            raise StreamException(res)
        return v.value
    
    def read_ui24(self):
        v = Ap4UI32()
        res = lb4.AP4_ByteStream_ReadUI24(self.bt4stream, byref(v))
        if res != 0:
            raise StreamException(res)
        return v.value
    
    def read_ui16(self):
        v = Ap4UI16()
        res = lb4.AP4_ByteStream_ReadUI16(self.bt4stream, byref(v))
        if res != 0:
            raise StreamException(res)
        return v.value
    
    def read_ui08(self):
        v = Ap4UI08()
        res = lb4.AP4_ByteStream_ReadUI08(self.straem, byref(v))
        if res != 0:
            raise StreamException(res)
        return v.value

    def read_string(self, length):
        p = create_string_buffer(length+1)
        res = lb4.AP4_ByteStream_ReadString(self.bt4stream, p, Ap4Size(length+1))
        if res != 0:
            raise StreamException(res)
        return p.value
    
    def write_partial(self, buffer):
        bytes_written = Ap4Size()
        res = lb4.AP4_ByteStream_WritePartial(self.bt4stream, c_char_p(buffer),
                                              Ap4Size(len(buffer)),
                                              byref(bytes_written))
        if res != 0:
            raise StreamException(res)
        return bytes_written

    def write(self, buffer):
        res = lb4.AP4_ByteStream_Write(self.bt4stream, c_char_p(buffer),
                                       Ap4Size(len(buffer)))
        if res != 0:
            raise StreamException(res)
        
    def write_double(self, value):
        res = lb4.AP4_ByteStream_WriteDouble(self.bt4stream, c_double(value))
        if res != 0:
            raise StreamException(res)

    def write_ui64(self, value):
        res = lb4.AP4_ByteStream_WriteUI64(self.bt4stream, Ap4UI64(value))
        if res != 0:
            raise StreamException(res)
    
    def write_ui32(self, value):
        res = lb4.AP4_ByteStream_WriteUI32(self.bt4stream, Ap4UI32(value))
        if res != 0:
            raise StreamException(res)
                                              
    def write_ui24(self, value):
        res = lb4.AP4_ByteStream_WriteUI24(self.bt4stream, Ap4UI32(value))
        if res != 0:
            raise StreamException(res)
        
    def write_ui16(self, value):
        res = lb4.AP4_ByteStream_WriteUI16(self.bt4stream, Ap4UI16(value))
        if res != 0:
            raise StreamException(res)
    
    def write_ui08(self, value):
        res = lb4.AP4_ByteStream_WriteUI08(self.bt4stream, Ap4UI08(value))
        if res != 0:
            raise StreamException(res)
         
    def write_string(self, value):
        res = lb4.AP4_ByteStream_Write(self.bt4stream, c_char_p(buffer))
        if res != 0:
            raise StreamException(res)
        
    def copy_to(self, receiver, size):
        res = lb4.AP4_ByteStream_CopyTo(self.bt4stream,
                                        receiver.stream,
                                        AP4_Size(size))
        if res != 0:
            raise StreamException(res)
    
    def seek(self, position):
        res = lb4.AP4_ByteStream_Seek(self.bt4stream, Ap4Position(position))
        if res != 0:
            raise StreamException(res)
    
    def flush(self):
        res = lb4.AP4_ByteStream_Flush(self.bt4stream)
        if res != 0:
            raise StreamException(res)
    
    def tell(self):
        pos = Ap4Position()
        res = lb4.AP4_ByteStream_Tell(self.bt4stream, byref(pos))
        if res != 0:
            raise StreamException(res)
        return pos.value
    
    def __del__(self):
        lb4.AP4_ByteStream_Release(self.bt4stream)
        
    
class MemoryByteStream(ByteStream):
    
    @staticmethod
    def FromBuffer(buffer):
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
        bt4stream = lb4.AP4_FileByteStream_Create(c_char_p(name), c_int(mode))
        super(FileByteStream, self).__init__(bt4stream)
        
    
