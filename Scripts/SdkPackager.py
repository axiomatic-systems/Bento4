#! /usr/bin/python

########################################################################
#      
#      Packaging Script for the Bento4 SDK
#
#      Original author:  Gilles Boccon-Gibod
#
#      Copyright (c) 2009 by Axiomatic Systems, LLC. All rights reserved.
#
#######################################################################

import sys
import os
import shutil
import glob
import datetime
import fnmatch
import zipfile
import re

#############################################################
# GetVersion
#############################################################
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
# GetSvnRevision
#############################################################
def GetSvnRevision():
    cmd = 'svn info'
    revision = 0
    lines = os.popen(cmd).readlines()
    for line in lines:
        if line.startswith('Revision: '):
            revision = line[10:] 
    if revision == 0:
        raise "unable to obtain SVN revision"
    return revision.strip()

#############################################################
# File Copy
#############################################################
def CopyFiles(file_patterns, configs=[''], rename_map={}):
    for config in configs:
        for (source,pattern,dest) in file_patterns:
            source_dir = BENTO4_HOME+'/'+source+'/'+config
            file_list = [os.path.join(source_dir, file) for file in fnmatch.filter(os.listdir(source_dir), pattern)]
            for file in file_list:
                dest_dir = SDK_ROOT+'/'+dest+'/'+config
                if not os.path.exists(dest_dir):
                    os.makedirs(dest_dir)
                filename = os.path.basename(file)
                if rename_map.has_key(filename):
                    dest_name = dest_dir+'/'+rename_map[filename]
                else:
                    dest_name = dest_dir
                print 'COPY: '+file+' -> '+dest_name
                shutil.copy2(file, dest_name)
        
#############################################################
# ZIP support
#############################################################
def ZipDir(top, archive, dir) :
    #print 'ZIP: ',top,dir
    entries = os.listdir(top)
    for entry in entries: 
        path = os.path.join(top, entry)
        if os.path.isdir(path):
            ZipDir(path, archive, os.path.join(dir, entry))
        else:
            zip_name = os.path.join(dir, entry)
            #print 'ZIP: adding '+path+' as '+zip_name
            archive.write(path, zip_name)

def ZipIt(basename, dir) :
    path = basename+'/'+dir
    zip_filename = path+'.zip'
    print 'ZIP: '+path+' -> '+zip_filename
   
    if os.path.exists(zip_filename):
        os.remove(zip_filename)

    archive = zipfile.ZipFile(zip_filename, "w", zipfile.ZIP_DEFLATED)
    if os.path.isdir(path):
        ZipDir(path, archive, dir)
    else:
        archive.write(path)
    archive.close()
    
#############################################################
# Main
#############################################################
# parse the command line
BENTO4_HOME = sys.argv[1]
SDK_TARGET  = sys.argv[2]

# ensure that BENTO4_HOME has been set and exists
if not os.path.exists(BENTO4_HOME) :
    print 'ERROR: BENTO4_HOME ('+BENTO4_HOME+' does not exist'
    sys.exit(1)
else :
    print 'BENTO4_HOME = ' + BENTO4_HOME

BENTO4_VERSION = GetVersion()

# compute paths
SDK_REVISION = GetSvnRevision()
SDK_NAME='Bento4-SDK-'+BENTO4_VERSION+'-'+SDK_REVISION
SDK_BUILD_ROOT=BENTO4_HOME+'/SDK'
SDK_ROOT=SDK_BUILD_ROOT+'/'+SDK_NAME
SDK_TARGET_DIR='Build/Targets/'+SDK_TARGET
SDK_TARGET_ROOT=BENTO4_HOME+'/'+SDK_TARGET_DIR
    
# special case for Xcode builds        
if SDK_TARGET == 'universal-apple-macosx':
    SDK_TARGET_DIR='Build/Targets/universal-apple-macosx/build'
    
print SDK_NAME

# remove any previous SDK directory
if os.path.exists(SDK_ROOT):
    shutil.rmtree(SDK_ROOT)
    
# copy single-config files
single_config_files = [
    ('Source/C++/Core','*.h','include'),
    ('Source/C++/Adapters','*.h','include'),
    ('Source/C++/CApi','*.h','include'),
    ('Source/C++/Codecs','*.h','include'),
    ('Source/C++/MetaData','*.h','include'),
    ('Source/C++/Crypto','*.h','include'),
    ('Documents','*.txt','docs'),
    ('Documents/Doxygen','*.chm','docs'),
    ('Documents/Doxygen','*.zip','docs'),
    ('Documents/Misc','*.doc','docs'),
    ('Documents/SDK','*.doc','docs'),
    ('Documents/SDK','*.pdf','docs')
]
CopyFiles(single_config_files)

# copy multi-config files
multi_config_files = [
    (SDK_TARGET_DIR,'*.exe','bin'),
    (SDK_TARGET_DIR,'mp42aac','bin'),
    (SDK_TARGET_DIR,'mp4dcfpackager','bin'),
    (SDK_TARGET_DIR,'mp4decrypt','bin'),
    (SDK_TARGET_DIR,'mp4dump','bin'),
    (SDK_TARGET_DIR,'mp4edit','bin'),
    (SDK_TARGET_DIR,'mp4encrypt','bin'),
    (SDK_TARGET_DIR,'mp4extract','bin'),
    (SDK_TARGET_DIR,'mp4info','bin'),
    (SDK_TARGET_DIR,'mp4rtphintinfo','bin'),
    (SDK_TARGET_DIR,'mp4tag','bin'),
    (SDK_TARGET_DIR,'aac2mp4','bin'),
    (SDK_TARGET_DIR,'*.a','lib'),
    (SDK_TARGET_DIR,'*.dll','bin'),        
    (SDK_TARGET_DIR,'*.dylib','bin'),
    (SDK_TARGET_DIR,'*.so','bin')
]
CopyFiles(multi_config_files, configs=['Debug','Release'])

# remove any previous zip file
ZipIt(SDK_BUILD_ROOT, SDK_NAME)
