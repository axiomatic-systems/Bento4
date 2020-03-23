#! /usr/bin/env python3

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
import re

#############################################################
# GetVersion
#############################################################
def GetVersion():
    f = open(BENTO4_HOME+'/Source/C++/Core/Ap4Version.h')
    lines = f.readlines()
    f.close()
    for line in lines:
        m = re.match(r'.*AP4_VERSION_STRING *"([0-9]*)\.([0-9]*)\.([0-9]*).*"', line)
        if m:
            return m.group(1) + '-' + m.group(2) + '-' + m.group(3)
    return '0-0-0'

#############################################################
# Main
#############################################################
# parse the command line
if len(sys.argv) < 2:
    print('ERROR: SDK revision # expected as first argument')
    sys.exit(1)

SDK_REVISION = sys.argv[1]

if len(sys.argv) >= 3:
    BENTO4_HOME = sys.argv[2]
else:
    script_dir  = os.path.abspath(os.path.dirname(__file__))
    BENTO4_HOME = os.path.join(script_dir,'..')

# ensure that BENTO4_HOME has been set and exists
if not os.path.exists(BENTO4_HOME) :
    print('ERROR: BENTO4_HOME ('+BENTO4_HOME+') does not exist')
    sys.exit(1)
else :
    print('BENTO4_HOME = ' + BENTO4_HOME)

# patch util files
for util_file in ["mp4-dash.py", "mp4-hls.py"]:
    filename = os.path.join(BENTO4_HOME, "Source", "Python", "utils", util_file)
    print("Patching", filename)
    file_lines = open(filename).readlines()
    file_out = open(filename, "w")
    for line in file_lines:
        if line.startswith("SDK_REVISION = "):
            line = "SDK_REVISION = '{}'\n".format(SDK_REVISION)
        file_out.write(line)

# path docker file
filename = os.path.join(BENTO4_HOME, "Build", "Docker", "Dockerfile")
print("Patching", filename)
file_lines = open(filename).readlines()
file_out = open(filename, "w")
for line in file_lines:
    if line.startswith("ENV BENTO4_VERSION "):
        line = "ENV BENTO4_VERSION {}-{}\n".format(GetVersion(), SDK_REVISION)
    file_out.write(line)
