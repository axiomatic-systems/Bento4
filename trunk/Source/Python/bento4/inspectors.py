from bento4 import *
from ctypes import CFUNCTYPE, c_char_p, byref, c_int, c_float

class AtomInspector(object):
    def __init__(self, bt4inspector):
        self.bt4inspector = inspector
        super(AtomInspector, self).__init__()
        
    def __del__(self):
        lb4.AP4_AtomInspector_Destroy(self.bt4inspector)
        
        
class PrintInspector(AtomInspector):
    def __init__(self, stream):
        self.stream = stream
        bt4inspector = lb4.AP4_PrintInspector_Create(stream.bt4stream)
        super(PrintInspector, self).__init__(bt4inspector)
        
class InspectorDelegate(Structure):
    pass

start_element_proto = CFUNCTYPE(None,
                                POINTER(InspectorDelegate),
                                c_char_p, c_char_p)

end_element_proto = CFUNCTYPE(None,
                              POINTER(InspectorDelegate))

add_int_field_proto = CFUNCTYPE(None,
                                POINTER(InspectorDelegate),
                                c_char_p, Ap4UI32, c_int)

add_float_field_proto = CFUNCTYPE(None,
                                  POINTER(InspectorDelegate),
                                  c_char_p, c_float, c_int)

add_string_field_proto = CFUNCTYPE(None,
                                   POINTER(InspectorDelegate),
                                   c_char_p, c_char_p, c_int)

add_bytes_field_proto = CFUNCTYPE(None,
                                  POINTER(InspectorDelegate),
                                  c_char_p, c_void_p, Ap4Size, c_int)

InspectorDelegate._fields_ = [("start_element", start_element_proto),
                              ("end_element", end_element_proto),
                              ("add_int_field", add_int_field_proto),
                              ("add_float_field", add_float_field_proto),
                              ("add_string_field", add_string_field_proto),
                              ("add_bytes_field", add_bytes_field_proto),
                              ("destroy", c_void_p), # must be set to None
                              ("oid", c_int)] # object id

PYINSPECTOR_OBJECTS = {}

def delegate_start_element(pdelegate, name, extra):
    pyinspector = PYINSPECTOR_OBJECTS[pdelegate[0].oid]
    pyinspector.c_start_element(name, extra)

def delegate_end_element(pdelegate):
    pyinspector = PYINSPECTOR_OBJECTS[pdelegate[0].oid]
    pyinspector.c_end_element()
    
def delegate_add_int_field(pdelegate, name, value, hint):
    pyinspector = PYINSPECTOR_OBJECTS[pdelegate[0].oid]
    pyinspector.c_add_int_field(name, value, hint)
    
def delegate_add_float_field(pdelegate, name, value, hint):
    pyinspector = PYINSPECTOR_OBJECTS[pdelegate[0].oid]
    pyinspector.c_add_float_field(name, value, hint)
    
def delegate_add_string_field(pdelegate, name, value, hint):
    pyinspector = PYINSPECTOR_OBJECTS[pdelegate[0].oid]
    pyinspector.c_add_string_field(name, value, hint)
    
def delegate_add_bytes_field(pdelegate, name, bytes, byte_count, hint):
    pyinspector = PYINSPECTOR_OBJECTS[pdelegate[0].oid]
    pyinspector.c_add_bytes_field(name, bytes, byte_count, hint)


class PyInspector(AtomInspector):
    
    def __init__(self, pyinspector):
        self.delegate = InspectorDelegate(
            start_element=delegate_start_element,
            end_element=delegate_end_element,
            add_int_field=delegate_add_int_field,
            add_float_field=delegate_add_float_field,
            add_string_field=delegate_add_string_field,
            add_bytes_field=delegate_add_bytes_field,
            destroy=None,
            oid=id(pyinspector))
        PYINSPECTOR_OBJECTS[id(pyinspector)] = pyinspector
        bt4inspector = lb4.AP4_AtomInspector_FromDelegate(byref(self.delegate))
        super(PyInspector, self).__init__(bt4inspector)

    def c_start_element(name, extra):
        pass
    
    def c_end_element():
        pass
    
    def c_add_int_field(name, value, hint):
        pass
    
    def c_add_float_field(name, value, hint):
        pass
    
    def c_add_string_field(name, value, hint):
        pass
    
    def c_add_bytes_field(name, bytes, byte_count, hint):
        pass
    

    




