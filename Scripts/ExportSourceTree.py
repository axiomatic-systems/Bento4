#! /usr/bin/python

import sys
import os
import shutil
import re
   
#############################################################
# GetSdkRevision
#############################################################
def GetSdkRevision():
    cmd = 'git rev-list HEAD --count'
    revision = 0
    return os.popen(cmd).readlines()[0].strip()

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
script_dir  = os.path.abspath(os.path.dirname(__file__))
BENTO4_HOME = os.path.join(script_dir,'..')
BENTO4_VERSION = GetVersion()

SDK_REVISION = GetSdkRevision()
print "Exporting Revision", SDK_REVISION

# compute paths
SDK_NAME='Bento4-SRC-'+BENTO4_VERSION+'-'+SDK_REVISION
SDK_OUTPUT_ROOT=BENTO4_HOME+'/SDK'
SDK_ROOT=SDK_OUTPUT_ROOT+'/'+SDK_NAME
            
print SDK_NAME

# remove any previous SDK directory
if os.path.exists(SDK_ROOT):
    shutil.rmtree(SDK_ROOT)

# ensure that the output dir exists
if not os.path.exists(SDK_OUTPUT_ROOT):
    os.makedirs(SDK_OUTPUT_ROOT)

### export
cmd = 'git archive --format=zip HEAD -o '+SDK_ROOT+'.zip' 
print cmd
#cmd = 'svn export -r'+SDK_REVISION+' https://zebulon.bok.net/svn/Bento4/trunk '+SDK_NAME
os.system(cmd)
