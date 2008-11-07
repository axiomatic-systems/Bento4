from ctypes import c_int, c_double, c_char_p, byref, \
                   CFUNCTYPE, POINTER, string_at
from bento4 import *
from bento4.errors import check_result, SUCCESS, ERROR_READ_FAILED, FAILURE, \
                          ERROR_WRITE_FAILED, ERROR_EOS, ERROR_OUT_OF_RANGE, \
                          ERROR_NOT_SUPPORTED


class ByteStream(object):
    """abstract ByteStream class"""
    
    def __init__(self, bt4stream):
        self.bt4stream = bt4stream
        super(ByteStream, self).__init__()
    
    def __del__(self):
        lb4.AP4_ByteStream_Release(self.bt4stream)
    
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
            bt4stream = lb4.AP4_MemoryByteStream_Create(Ap4Size(size))
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

        
class ByteStreamDelegate(Structure):
    pass

read_partial_proto  = CFUNCTYPE(Ap4Result,
                                POINTER(ByteStreamDelegate),
                                POINTER(Ap4Byte),
                                Ap4Size,
                                POINTER(Ap4Size))
write_partial_proto = CFUNCTYPE(Ap4Result,
                                POINTER(ByteStreamDelegate),
                                c_void_p,
                                Ap4Size,
                                POINTER(Ap4Size))
seek_proto          = CFUNCTYPE(Ap4Result,
                                POINTER(ByteStreamDelegate),
                                Ap4Position)
tell_proto          = CFUNCTYPE(Ap4Result,
                                POINTER(ByteStreamDelegate),
                                POINTER(Ap4Position))
get_size_proto      = CFUNCTYPE(Ap4Result,
                                POINTER(ByteStreamDelegate),
                                POINTER(Ap4LargeSize))
flush_proto         = CFUNCTYPE(Ap4Result,
                                POINTER(ByteStreamDelegate))

ByteStreamDelegate._fields_ = [("read_partial", read_partial_proto),
                               ("write_partial", write_partial_proto),
                               ("seek", seek_proto),
                               ("tell", tell_proto),
                               ("get_size", get_size_proto),
                               ("flush", flush_proto),
                               ("destroy", c_void_p), # must be set to None
                               ("oid", c_int)] # object id

# dict where we are going to store the pybytestream instances by id
PYSTREAM_OBJECTS = {}

def delegate_read_partial(pdelegate, buffer, bytes_to_read, bytes_read):
    pystream = PYSTREAM_OBJECTS[pdelegate[0].oid]
    return pystream.c_read_partial(buffer,
                                   bytes_to_read,
                                   bytes_read)

def delegate_read_partial(pdelegate, buffer, bytes_to_write, bytes_written):
    pystream = PYSTREAM_OBJECTS[pdelegate[0].oid]
    return pystream.c_write_partial(buffer,
                                    bytes_to_write,
                                    bytes_written)

def delegate_seek(pdelegate, position):
    pystream = PYSTREAM_OBJECTS[pdelegate[0].oid]
    return pystream.c_seek(position)

def delegate_tell(pdelegate, position):
    pystream = PYSTREAM_OBJECTS[pdelegate[0].oid]
    return pystream.c_tell(position)
    
def delegate_get_size(pdelegate, size):
    pystream = PYSTREAM_OBJECTS[pdelegate[0].oid]
    return pystream.c_get_size(size)

def delegate_flush(pdelegate, size):
    pystream = PYSTREAM_OBJECTS[pdelegate[0].oid]
    return pystream.c_flush()


class PyByteStream(ByteStream):
    
    def __init__(self, pystream):
        self.delegate = ByteStreamDelegate(
            read_partial=read_partial_proto(delegate_read_partial),
            write_partial=write_partial_proto(delegate_write_partial),
            seek=seek_proto(delegate_seek),
            tell=tell_proto(delegate_tell),
            get_size=get_size_proto(delegate_get_size),
            flush=flush_proto(delegate_flush),
            destroy=None,
            oid=id(stream))
        PYSTREAM_OBJECTS[id(pystream)] = pystream # store the stream
        bt4stream = lb4.AP4_ByteStream_FromDelegate(byref(self.delegate))
        super(PyByteStream, self).__init__(bt4stream)
        
    def c_read_partial(self, buffer, bytes_to_read, bytes_read):
        return ERROR_NOT_SUPPORTED
            
    def c_write_partial(self, buffer, bytes_to_write, bytes_written):
        return ERROR_NOT_SUPPORTED
    
    def c_seek(self, position):
        return ERROR_NOT_SUPPORTED

    def c_tell(self, position):
        return ERROR_NOT_SUPPORTED
    
    def c_get_size(self, size):
        return ERROR_NOT_SUPPORTED
        
    def c_flush(self):
        return ERROR_NOT_SUPPORTED
 

class PyFileByteStream(PyByteStream):
    
    def __init__(self, file, size=0):
        self.file = file
        self.size = size
        super(PyFileByteStream, self).__init__(self)
    
    def c_read_partial(self, buffer, bytes_to_read, bytes_read):
        try:
            s = self.file.read(bytes_to_read)
            bytes_read[0] = len(s)
            buffer[:bytes_to_read] = s[:bytes_to_read] # copy the buffer
        except EOFError:
            return ERROR_EOS
        except IOError:
            return READ_FAILED
        return SUCCESS
            
    def c_write_partial(self, buffer, bytes_to_write, bytes_written):
        try:
            self.file.write(string_at(buffer, bytes_to_write))
            bytes_written[0] = bytes_to_write
        except IOError:
            return WRITE_FAILED
        return SUCCESS
    
    def c_seek(self, position):
        try:
            self.file.seek(position)
        except Exception:
            return ERROR_OUT_OF_RANGE
        return SUCCESS
    
    def c_tell(self, position):
        try:
            position[0] = self.file.tell()
        except Exception:
            return FAILURE
        return SUCCESS
    
    def c_get_size(self, size):
        if self.size:
            size[0] = self.size
            return SUCCESS
        else:
            return ERROR_NOT_SUPPORTED
        
    def c_flush(self):
        try:
            self.file.flush()
        except Exception:
            return FAILURE
        return SUCCESS
    
    




        
    
