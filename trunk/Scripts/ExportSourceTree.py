#! /usr/bin/python

import sys
import os
import shutil
import zipfile
import re

#############################################################
# ZIP support
#############################################################
def ZipDir(top, archive, dir) :
    entries = os.listdir(top)
    for entry in entries: 
        path = os.path.join(top, entry)
        if os.path.isdir(path):
            ZipDir(path, archive, os.path.join(dir, entry))
        else:
            zip_name = os.path.join(dir, entry)
            archive.write(path, zip_name)

def ZipIt(dir) :
    zip_filename = dir+'.zip'
   
    if os.path.exists(zip_filename):
        os.remove(zip_filename)

    archive = zipfile.ZipFile(zip_filename, "w", zipfile.ZIP_DEFLATED)
    ZipDir(dir, archive, dir)
    archive.close()
    
def GetVersion():
    f = open(BENTO4_HOME+'/Source/C++/Core/Ap4Version.h')
    lines = f.readlines()
    f.close()
    for line in lines:
        m = re.match(r'.*AP4_VERSION_STRING *"([0-9]*)\.([0-9]*)\.([0-9]*)"', line)
        if m:
            return m.group(1) + '-' + m.group(2) + '-' + m.group(3)
    return '0-0-0'


#############################################################
# Main
#############################################################

if not os.environ.has_key('BENTO4_HOME'):
    print 'ERROR: BENTO4_HOME not set'
    sys.exit(1)
BENTO4_HOME = os.environ['BENTO4_HOME']

BENTO4_VERSION = GetVersion()

SDK_REVISION = sys.argv[1]
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
cmd = 'svn export -r'+SDK_REVISION+' https://zebulon.bok.net/svn/Bento4/trunk '+SDK_NAME
os.system(cmd)

### zip it
ZipIt(SDK_NAME)

### move the output
shutil.move(SDK_NAME+'.zip', SDK_OUTPUT_ROOT)
shutil.move(SDK_NAME, SDK_ROOT)
