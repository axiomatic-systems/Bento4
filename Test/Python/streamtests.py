import bento4.streams as bt4s
import os.path as path
import unittest

BENTO4_TEST_DATA_DIR = path.join(path.dirname(__file__), '..', 'Data')

class StreamTester(unittest.TestCase):
    
    def test_memorystream(self):
        stream = bt4s.MemoryByteStream()
        
        