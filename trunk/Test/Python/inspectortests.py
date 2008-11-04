import bento4.core as bt4core
import bento4.inspectors as bt4inspect
import os.path as path
import xml.etree.ElementTree

BENTO4_TEST_DATA_DIR = path.join(path.dirname(__file__), '..', 'Data')

file = bt4core.File(path.join(BENTO4_TEST_DATA_DIR, 'test-001.mp4'))
inspector = bt4inspect.XmlInspector()
file.inspect(inspector)
print xml.etree.ElementTree.tostring(inspector.root)
