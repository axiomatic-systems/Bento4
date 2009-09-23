from bento4 import *
from ctypes import CFUNCTYPE, c_char_p, byref, c_int, c_float, string_at
from xml.etree.ElementTree import Element, SubElement
from base64 import b16encode

class AtomInspector(object):
    def __init__(self, bt4inspector):
        self.bt4inspector = bt4inspector
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
                                c_char_p, Ap4UI64, c_int)

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

# global dict of objects constructed with a delegate
pyinspector_objects = {}

#
# redirection functions
#
def delegate_start_element(pdelegate, name, extra):
    pyinspector = pyinspector_objects[pdelegate[0].oid]
    pyinspector.c_start_element(name, extra)

def delegate_end_element(pdelegate):
    pyinspector = pyinspector_objects[pdelegate[0].oid]
    pyinspector.c_end_element()
    
def delegate_add_int_field(pdelegate, name, value, hint):
    pyinspector = pyinspector_objects[pdelegate[0].oid]
    pyinspector.c_add_int_field(name, value, hint)
    
def delegate_add_float_field(pdelegate, name, value, hint):
    pyinspector = pyinspector_objects[pdelegate[0].oid]
    pyinspector.c_add_float_field(name, value, hint)
    
def delegate_add_string_field(pdelegate, name, value, hint):
    pyinspector = pyinspector_objects[pdelegate[0].oid]
    pyinspector.c_add_string_field(name, value, hint)
    
def delegate_add_bytes_field(pdelegate, name, bytes, byte_count, hint):
    pyinspector = pyinspector_objects[pdelegate[0].oid]
    pyinspector.c_add_bytes_field(name, bytes, byte_count, hint)


class PyInspector(AtomInspector):
    
    def __init__(self, pyinspector):
        self.delegate = InspectorDelegate(
            start_element=start_element_proto(delegate_start_element),
            end_element=end_element_proto(delegate_end_element),
            add_int_field=add_int_field_proto(delegate_add_int_field),
            add_float_field=add_float_field_proto(delegate_add_float_field),
            add_string_field=add_string_field_proto(delegate_add_string_field),
            add_bytes_field=add_bytes_field_proto(delegate_add_bytes_field),
            destroy=None,
            oid=id(pyinspector))
        pyinspector_objects[id(pyinspector)] = pyinspector
        bt4inspector = lb4.AP4_AtomInspector_FromDelegate(byref(self.delegate))
        super(PyInspector, self).__init__(bt4inspector)

    def c_start_element(self, name, extra):
        pass
    
    def c_end_element(self):
        pass
    
    def c_add_int_field(self, name, value, hint):
        pass
    
    def c_add_float_field(self, name, value, hint):
        pass
    
    def c_add_string_field(self, name, value, hint):
        pass
    
    def c_add_bytes_field(self, name, bytes, byte_count, hint):
        pass
    

class XmlInspector(PyInspector):
    
    def __init__(self):
        self.root = Element("Mp4File")
        self.current = (None, self.root) # parent, element
        super(XmlInspector, self).__init__(self)
        
    def c_start_element(self, name, extra):
        parent, element = self.current
        new_element = SubElement(element, "Atom", name=name[1:-1])
        if extra:
            a = extra.split('=')
            if len(a) == 2:
                new_element.attrib[a[0]] = a[1]
        self.current = ((parent, element), new_element)
        
    def c_end_element(self):
        (grandparent, parent), element = self.current
        self.current = (grandparent, parent)
    
    def c_add_int_field(self, name, value, hint):
        int_element = SubElement(self.current[1], "Field",
                                 name=name, type='int')
        int_element.text = str(value)
        
    def c_add_float_field(self, name, value, hint):
        float_element = SubElement(self.current[1], "Field",
                                   name=name, type='float')
        float_element.text = str(value)
            
    def c_add_string_field(self, name, value, hint):
        str_element = SubElement(self.current[1], "Field",
                                 name=name, type='string')
        str_element.text = value
        
    def c_add_bytes_field(self, name, bytes, byte_count, hint):
        bytes_element = SubElement(self.current[1], "Field",
                                   name=name, type='bytes')
        bytes_element.text = b16encode(string_at(bytes, byte_count))
        
        
        
    

    




