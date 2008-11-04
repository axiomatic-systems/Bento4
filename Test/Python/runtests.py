import os.path as path
BENTO4_HOME = path.abspath(path.join(path.dirname(__file__), '..', '..'))

# add the bento4 module to the path
import sys
sys.path += [path.join(BENTO4_HOME, 'Source', 'Python')]

# import the tests
import unittest
from coretests import CoreTester
from streamtests import StreamTester

# launch the tests
if __name__ == '__main__':
    unittest.main()