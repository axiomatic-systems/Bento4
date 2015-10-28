#! /usr/bin/python

########################################################################
#
#      Release update script for the Bento4 SDK
#
#      Original author:  Gilles Boccon-Gibod
#
#      Copyright (c) 2009-2015 by Axiomatic Systems, LLC. All rights reserved.
#
#######################################################################

import sys
import os
import shutil
import platform

#############################################################
# Main
#############################################################
# parse the command line
if len(sys.argv) < 2:
    print 'ERROR: SDK revision # expected as first argument'
    sys.exit(1)

SDK_REVISION = sys.argv[1]

if len(sys.argv) >= 3:
    BENTO4_HOME = sys.argv[2]
else:
    script_dir  = os.path.abspath(os.path.dirname(__file__))
    BENTO4_HOME = os.path.join(script_dir,'..')

# ensure that BENTO4_HOME has been set and exists
if not os.path.exists(BENTO4_HOME) :
    print 'ERROR: BENTO4_HOME ('+BENTO4_HOME+') does not exist'
    sys.exit(1)
else :
    print 'BENTO4_HOME = ' + BENTO4_HOME

# patch files
filename = os.path.join(BENTO4_HOME, "Source", "Python", "utils", "mp4-dash.py")
print "Patching", filename
file_lines = open(filename).readlines()
file_out = open(filename, "wb")
for line in file_lines:
    if line.startswith("SDK_REVISION = "):
        line = "SDK_REVISION = '"+SDK_REVISION+"'\n"
    file_out.write(line)

filename = os.path.join(BENTO4_HOME, "Source", "Python", "utils", "mp4-hls.py")
print "Patching", filename
file_lines = open(filename).readlines()
file_out = open(filename, "wb")
for line in file_lines:
    if line.startswith("SDK_REVISION = "):
        line = "SDK_REVISION = '"+SDK_REVISION+"'\n"
    file_out.write(line)
