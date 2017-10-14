#! /usr/bin/python

########################################################################
#
#      Packaging Script for the Bento4 SDK
#
#      Original author:  Gilles Boccon-Gibod
#
#      Copyright (c) 2009-2013 by Axiomatic Systems, LLC. All rights reserved.
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
import platform

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
# GetSdkRevision
#############################################################
def GetSdkRevision():
    cmd = 'git status --porcelain -b'
    lines = os.popen(cmd).readlines()
    if not lines[0].startswith('## master'):
        print 'ERROR: not on master branch'
        return None
    if len(lines) > 1:
        print 'ERROR: git status not empty'
        print ''.join(lines)
        return None

    cmd = 'git tag --contains HEAD'
    tags = os.popen(cmd).readlines()
    if len(tags) != 1:
        print 'ERROR: expected exactly one tag for HEAD, found', len(tags), ':', tags
        return None
    version = tags[0].strip()
    sep = version.find('-')
    if sep < 0:
        print 'ERROR: unrecognized version string format:', version
    return version[sep+1:]

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
if len(sys.argv) > 1:
    SDK_TARGET = sys.argv[1]
else:
    SDK_TARGET = None

if len(sys.argv) > 2:
    BENTO4_HOME = sys.argv[1]
else:
    script_dir  = os.path.abspath(os.path.dirname(__file__))
    BENTO4_HOME = os.path.join(script_dir,'..')

# ensure that BENTO4_HOME has been set and exists
if not os.path.exists(BENTO4_HOME) :
    print 'ERROR: BENTO4_HOME ('+BENTO4_HOME+') does not exist'
    sys.exit(1)
else :
    print 'BENTO4_HOME = ' + BENTO4_HOME

# compute the target if it is not specified
if SDK_TARGET is None:
    targets_dir = BENTO4_HOME+'/Build/Targets'
    targets_dirs = os.listdir(targets_dir)
    target_platforms = [x for x in targets_dirs if os.path.exists(targets_dir +'/'+x+'/Config.scons')]
    platform_id = sys.platform
    if platform.system() == 'Linux':
        if (platform.machine() == 'i386' or
            platform.machine() == 'i486' or
            platform.machine() == 'i586' or
            platform.machine() == 'i686'):
            platform_id = 'linux-i386'
        if (platform.machine() == 'x86_64'):
            platform_id = 'linux-x86_64'
        if (platform.machine().startswith('arm')):
            platform_id = 'linux-arm'

    platform_to_target_map = {
        'linux-i386'  : 'x86-unknown-linux',
        'linux-x86_64': 'x86_64-unknown-linux',
        'linux2'      : 'x86-unknown-linux',
        'win32'       : 'x86-microsoft-win32-vs2010',
        'darwin'      : 'universal-apple-macosx'
    }

    if platform_to_target_map.has_key(platform_id):
        SDK_TARGET = platform_to_target_map[platform_id]
    else:
        print 'ERROR: SDK_TARGET is not set and cannot be detected'
        sys.exit(1)

print "TARGET = " + SDK_TARGET

BENTO4_VERSION = GetVersion()

# compute paths
SDK_REVISION = GetSdkRevision()
if SDK_REVISION is None:
    sys.exit(1)
SDK_NAME='Bento4-SDK-'+BENTO4_VERSION+'-'+SDK_REVISION+'.'+SDK_TARGET
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

# copy headers, docs and utils
misc_files = [
    ('Source/C++/Core','*.h','include'),
    ('Source/C++/Adapters','*.h','include'),
    ('Source/C++/CApi','*.h','include'),
    ('Source/C++/Codecs','*.h','include'),
    ('Source/C++/MetaData','*.h','include'),
    ('Source/C++/Crypto','*.h','include'),
    ('Documents','*.txt','docs'),
    ('Documents/Doxygen','*.chm','docs'),
    ('Documents/Doxygen','*.zip','docs'),
    ('Documents/SDK','*.doc','docs'),
    ('Documents/SDK','*.pdf','docs'),
    ('Source/Python/utils', '*.py', 'utils')
]
CopyFiles(misc_files)

if SDK_TARGET == 'universal-apple-macosx':
    script_bin_dir = 'macosx'
elif SDK_TARGET.startswith('x86-microsoft-win32'):
    script_bin_dir = 'win32'
elif SDK_TARGET == 'x86-unknown-linux':
    script_bin_dir = 'linux-x86'
elif SDK_TARGET == 'x86-unknown-linux':
    script_bin_dir = 'linux-x86_64'
else:
    script_bin_dir = None

# binaries
bin_in = SDK_TARGET_DIR+'/Release'
bin_files = [
    (bin_in,'mp4*.exe','bin'),
    (bin_in,'aac2mp4.exe','bin'),
    (bin_in,'mp42aac','bin'),
    (bin_in,'mp42avc','bin'),
    (bin_in,'mp42hevc','bin'),
    (bin_in,'mp4dcfpackager','bin'),
    (bin_in,'mp4decrypt','bin'),
    (bin_in,'mp4dump','bin'),
    (bin_in,'mp4edit','bin'),
    (bin_in,'mp4encrypt','bin'),
    (bin_in,'mp4extract','bin'),
    (bin_in,'mp4fragment','bin'),
    (bin_in,'mp4split','bin'),
    (bin_in,'mp4compact','bin'),
    (bin_in,'mp4info','bin'),
    (bin_in,'mp4rtphintinfo','bin'),
    (bin_in,'mp4tag','bin'),
    (bin_in,'mp4mux','bin'),
    (bin_in,'aac2mp4','bin'),
    (bin_in,'mp42ts','bin'),
    (bin_in,'mp42hls','bin'),
    (bin_in,'mp4iframeindex','bin'),
    (bin_in,'*.a','lib'),
    (bin_in,'*.dll','bin'),
    (bin_in,'*.dylib','bin'),
    (bin_in,'*.so','bin')
]
CopyFiles(bin_files)

# wrappers
if '-microsoft-' in SDK_TARGET:
    wrapper_files = [
        ('Source/Python/wrappers', 'mp4dash.bat','bin'),
        ('Source/Python/wrappers', 'mp4dashclone.bat','bin'),
        ('Source/Python/wrappers', 'mp4hls.bat','bin')
    ]
else:
    wrapper_files = [
        ('Source/Python/wrappers', 'mp4dash','bin'),
        ('Source/Python/wrappers', 'mp4dashclone','bin'),
        ('Source/Python/wrappers', 'mp4hls','bin')
    ]
CopyFiles(wrapper_files)

# remove any previous zip file
ZipIt(SDK_BUILD_ROOT, SDK_NAME)
