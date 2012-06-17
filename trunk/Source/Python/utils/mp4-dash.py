#!/usr/bin/env python

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2012 Axiomatic Systems, LLC.'

###
# NOTE: this script needs Bento4 command line binaries to run
# You must place the 'mp4info' 'mp4dump' and 'mp4split' binaries
# in a directory named 'bin/<platform>' at the same level as where
# this script is.
# <platform> depends on the platform you're running on:
# Mac OSX --> platform = macosx
# Linux   --> platform = linux
# Windows --> platform = win32

import sys
import os
import os.path as path
from optparse import OptionParser, make_option, OptionError
from subprocess import check_output, CalledProcessError
import urlparse
import random
import base64
import shutil
import tempfile
import json
import io
import struct
import xml.etree.ElementTree as xml
from xml.dom.minidom import parseString
import operator

# setup main options
SCRIPT_PATH = path.abspath(path.dirname(__file__))
sys.path += [SCRIPT_PATH]

VIDEO_MIMETYPE       = 'video/mp4'
AUDIO_MIMETYPE       = 'audio/mp4'
VIDEO_DIR            = 'video'
AUDIO_DIR            = 'audio'
MPD_NS               = 'urn:mpeg:DASH:schema:MPD:2011'
ISOFF_LIVE_PROFILE   = 'urn:mpeg:dash:profile:isoff-live:2011' 
INIT_SEGMENT_NAME    = 'init.mp4'
SEGMENT_PATTERN      = 'seg-%04d.m4f'
SEGMENT_TEMPLATE     = 'seg-$Number%04d$.m4f'
LINEAR_PATTERN       = 'media-%02d.mp4'
MARLIN_MAS_NAMESPACE = 'urn:marlin:mas:1-0:services:schemas:mpd'

def PrintErrorAndExit(message):
    sys.stderr.write(message+'\n')
    sys.exit(1)
    
def XmlDuration(d):
    h  = d/3600
    d -= h*3600
    m  = d/60
    s  = d-m*60
    xsd = 'PT'
    if h:
        xsd += str(h)+'H'
    if h or m:
        xsd += str(m)+'M'
    if s:
        xsd += str(s)+'S'
    return xsd
    
def Bento4Command(name, *args, **kwargs):
    cmd = [path.join(Options.exec_dir, name)]
    for kwarg in kwargs:
        arg = kwarg.replace('_', '-')
        cmd.append('--'+arg)
        if not isinstance(kwargs[kwarg], bool):
            cmd.append(kwargs[kwarg])
    cmd += args
    #print cmd
    try:
        return check_output(cmd) 
    except CalledProcessError, e:
        #print e
        raise Exception("binary tool failed with error %d" % e.returncode)
    
def Mp4Info(filename, **args):
    return Bento4Command('mp4info', filename, **args)

def Mp4Dump(filename, **args):
    return Bento4Command('mp4dump', filename, **args)

def Mp4Split(filename, **args):
    return Bento4Command('mp4split', filename, **args)

class Mp4Atom:
    def __init__(self, type, size, position):
        self.type     = type
        self.size     = size
        self.position = position
        
def WalkAtoms(filename):
    cursor = 0
    atoms = []
    file = io.FileIO(filename, "rb")
    while True:
        try:
            size = struct.unpack('>I', file.read(4))[0]
            type = file.read(4)
            if size == 1:
                size = struct.unpack('>Q', file.read(8))[0]
            #print type,size
            atoms.append(Mp4Atom(type, size, cursor))
            cursor += size
            file.seek(cursor)
        except:
            break
        
    return atoms
        
def FilterChildren(parent, type):
    if isinstance(parent, list):
        children = parent
    else:
        children = parent['children']
    return [child for child in children if child['name'] == type]

def FindChild(top, path):
    for entry in path:
        children = FilterChildren(top, entry)
        if len(children) == 0: return None
        top = children[0]
    return top

class Mp4File:
    def __init__(self, filename):
        self.filename       = filename
        self.work_filename  = filename
        self.video_track_id = 0
        self.audio_track_id = 0
        
        # walk the atom structure
        self.atoms = WalkAtoms(filename)
        self.segments = []
        for atom in self.atoms:
            if atom.type == 'moov':
                self.init_segment = atom
            elif atom.type == 'moof':
                self.segments.append([atom])
            else:
                if len(self.segments):
                    self.segments[-1].append(atom)
        #print self.segments
        
        # get the mp4 file info
        json_info = Mp4Info(filename, format='json')
        #print json_info
        self.info = json.loads(json_info, strict=False)

        for track in self.info['tracks']:
            if track['type'] == 'Video':
                self.video_track_id = track['id']
            elif track['type'] == 'Audio':
                self.audio_track_id = track['id']

        # get a complete file dump
        json_dump = Mp4Dump(filename, format='json')
        #print json_dump
        self.tree = json.loads(json_dump, strict=False)
        
        # look for KIDs
        if Options.marlin:
            self.kids = {}
            moov = FilterChildren(self.tree, 'moov')[0]
            traks = FilterChildren(moov, 'trak')
            for trak in traks:
                tkhd = FindChild(trak, ['tkhd'])
                track_id = tkhd['id']
                tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'encv', 'sinf', 'schi', 'tenc'))
                if tenc is None:
                    tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'enca', 'sinf', 'schi', 'tenc'))
                if tenc and 'default_KID' in tenc:
                    kid = tenc['default_KID'].strip('[]').replace(' ', '')
                    self.kids[track_id] = kid
                    
            if len(self.kids) == 0:
                PrintErrorAndExit('ERROR: no encryption info found')
                
        # partition the segments
        segment_index = 0
        track_id      = 0
        self.moofs         = { self.audio_track_id:[],  self.video_track_id:[]}
        self.sample_counts = { self.audio_track_id:[],  self.video_track_id: []}
        self.segment_sizes = { self.audio_track_id:[],  self.video_track_id: []}
        for atom in self.tree:
            if atom['name'] == 'moof':
                trafs = FilterChildren(atom, 'traf')
                if len(trafs) != 1:
                    PrintErrorAndExit('ERROR: unsupported input file, more than one "traf" box in fragment')
                tfhd = FilterChildren(trafs[0], 'tfhd')[0]
                track_id = tfhd['track ID']
                self.moofs[track_id].append(segment_index)
                self.segment_sizes[track_id].append(0)
                for trun in FilterChildren(trafs[0], 'trun'):
                    self.sample_counts[track_id].append(trun['sample count'])
                segment_index += 1
            else:
                if track_id and len(self.segment_sizes[track_id]):
                    self.segment_sizes[track_id][-1] += atom['size']
                
        self.media_size = {
            self.audio_track_id: reduce(operator.add, self.segment_sizes[self.audio_track_id], 0),
            self.video_track_id: reduce(operator.add, self.segment_sizes[self.video_track_id], 0)
        }
        self.average_segment_bitrate = {
            self.audio_track_id:0,
            self.video_track_id:0
        }
        self.max_segment_bitrate = {
            self.audio_track_id:0,
            self.video_track_id:0
        }
        self.total_samples = {
            self.audio_track_id: reduce(operator.add, self.sample_counts[self.audio_track_id], 0),
            self.video_track_id: reduce(operator.add, self.sample_counts[self.video_track_id], 0)
        }

        if self.total_samples[self.audio_track_id] and Options.audio_sample_rate:
            self.average_segment_bitrate[self.audio_track_id] = int(8*self.media_size[self.audio_track_id]/(self.total_samples[self.audio_track_id]/(Options.audio_sample_rate/1024)))
        if self.total_samples[self.video_track_id] and Options.video_frame_rate:
            self.average_segment_bitrate[self.video_track_id] = int(8*self.media_size[self.video_track_id]/(self.total_samples[self.video_track_id]/Options.video_frame_rate))
        
        self.segment_duration = { self.audio_track_id:0, self.video_track_id:0 }
        if len(self.sample_counts[self.audio_track_id]) > 1 and Options.audio_sample_rate:
            self.segment_duration[self.audio_track_id] = float(reduce(operator.add, self.sample_counts[self.audio_track_id][:-1]))/float(len(self.sample_counts[self.audio_track_id])-1)/(Options.audio_sample_rate/1024.0)
        if len(self.sample_counts[self.video_track_id]) > 1 and Options.video_frame_rate:
            self.segment_duration[self.video_track_id] = float(reduce(operator.add, self.sample_counts[self.video_track_id][:-1]))/float(len(self.sample_counts[self.video_track_id])-1)/Options.video_frame_rate

        gap = self.segment_duration[self.audio_track_id]-self.segment_duration[self.video_track_id]
        if gap > 0.1:
            print 'WARNING: audio and video segment durations are not equal ('+str(self.segment_duration[self.audio_track_id])+' and '+str(self.segment_duration[self.video_track_id])+'), check sample and frame rates'

        self.max_segment_bitrate = { self.audio_track_id:0.0, self.video_track_id:0.0 }
        if self.segment_duration[self.audio_track_id]:
            self.max_segment_bitrate[self.audio_track_id] = 8*int(float(max(self.segment_sizes[self.audio_track_id][:-1]))/self.segment_duration[self.audio_track_id])
        if self.segment_duration[self.video_track_id]:
            self.max_segment_bitrate[self.video_track_id] = 8*int(float(max(self.segment_sizes[self.video_track_id][:-1]))/self.segment_duration[self.video_track_id])
        
# Setup default setting
class Settings:
    output_dir = 'output'

def MakeNewDir(dir, is_warning=False):
    if os.path.exists(dir):
        if is_warning:
            print 'WARNING: ',
        else:
            print 'ERROR: ',
        print 'directory "'+dir+'" already exists'
        if not is_warning:
            sys.exit(1)
    else:
        os.mkdir(dir)
        
def AddSegmentList(container, subdir, media_file, track_id, use_byte_range=False):
    if subdir:
        prefix = subdir+'/'
    else:
        prefix = ''
    segment_list = xml.SubElement(container,
                                  'SegmentList',
                                  timescale='1000',
                                  duration=str(int(media_file.segment_duration[track_id]*1000)))
    if use_byte_range:
        byte_range=str(media_file.init_segment.position)+'-'+str(media_file.init_segment.position+media_file.init_segment.size-1)
        xml.SubElement(segment_list, 'Initialization', sourceURL=prefix+(LINEAR_PATTERN % (media_file.index)), range=byte_range)
    else:
        xml.SubElement(segment_list, 'Initialization', sourceURL=prefix+INIT_SEGMENT_NAME)
    i = 0
    for segment_index in media_file.moofs[track_id]:
        segment = media_file.segments[segment_index]
        segment_offset = segment[0].position
        segment_length = reduce(operator.add, [atom.size for atom in segment], 0)
        if use_byte_range:
            byte_range = str(segment_offset)+'-'+str(segment_offset+segment_length-1)
            xml.SubElement(segment_list, 'SegmentURL', media=prefix+(LINEAR_PATTERN % (media_file.index)), mediaRange=byte_range)
        else:
            xml.SubElement(segment_list, 'SegmentURL', media=prefix+(SEGMENT_PATTERN % (i)))
        i += 1

def AddSegmentTemplate(container, subdir, media_file, track_id):
    if subdir:
        prefix = subdir+'/'
    else:
        prefix = ''
    segment_list = xml.SubElement(container,
                                  'SegmentTemplate',
                                  timescale='1000',
                                  duration=str(int(media_file.segment_duration[track_id]*1000)),
                                  startNumber='0',
                                  initialization=prefix+INIT_SEGMENT_NAME,
                                  media=prefix+SEGMENT_TEMPLATE)

def AddSegments(container, subdir, media_file, track_id, use_byte_range=False):
    if Options.use_segment_list:
        AddSegmentList(container, subdir, media_file, track_id, use_byte_range)
    else:
        AddSegmentTemplate(container, subdir, media_file, track_id)
    
def AddContentProtection(container, media_files, media_type):
    kids = []
    for media_file in media_files:
        if media_type == 'audio':
            track_id = media_file.audio_track_id
        else:
            track_id = media_file.video_track_id
        kid = media_file.kids[track_id]
        if kid not in kids:
            kids.append(kid)
    xml.register_namespace('mas', MARLIN_MAS_NAMESPACE)
    #xml.SubElement(container, 'ContentProtection', schemeIdUri='urn:mpeg:dash:mp4protection:2011', value='cenc')
    cp = xml.SubElement(container, 'ContentProtection', schemeIdUri='urn:uuid:5E629AF5-38DA-4063-8977-97FFBD9902D4')
    cids = xml.SubElement(cp, '{'+MARLIN_MAS_NAMESPACE+'}MarlinContentIds')
    for kid in kids:
        cid = xml.SubElement(cids, '{'+MARLIN_MAS_NAMESPACE+'}MarlinContentId')
        cid.text = 'urn:marlin:kid:'+kid
        
Options = None            
def main():
    # determine the platform binary name
    platform = sys.platform
    if platform.startswith('linux'):
        platform = 'linux'
    elif platform.startswith('darwin'):
        platform = 'macosx'
                
    # parse options
    parser = OptionParser(usage="%prog [options] <filename> [<filename> ...]")
    parser.add_option('', '--verbose', dest="verbose",
                      action='store_true', default=False,
                      help="Be verbose")
    parser.add_option('-o', '--output-dir', dest="output_dir",
                      help="Output directory", metavar="<output-dir>", default='output')
    parser.add_option('', '--init-segment', dest="init_segment",
                      help="Initialization segment name", metavar="<filename>", default='init.mp4')
    parser.add_option('-m', '--mpd-name', dest="mpd_filename",
                      help="MPD file name", metavar="<filename>", default='mpd.xml')
    parser.add_option('', '--mpd-only', dest="mpd_only",
                      action='store_true', default=False,
                      help="Only output the MPD file (no media processing)")
    parser.add_option('', "--no-split",
                      action="store_false", dest="split", default=True,
                      help="Do not split the file into segments (use byte ranges instead)")
    parser.add_option('', "--use-segment-list",
                      action="store_true", dest="use_segment_list", default=False,
                      help="Use segment lists instead of segment templates")
    parser.add_option('', "--min-buffer-time", metavar='<duration>',
                      dest="min_buffer_time", type="float", default=0.0,
                      help="Mininum buffer time (in seconds)")
    parser.add_option('', "--video-frame-rate", metavar='<rate>',
                      dest="video_frame_rate", type="float", default=23.976,
                      help="Video frame rate (in frames per second)")
    parser.add_option('', "--video-codec", metavar='<codec>',
                      dest="video_codec", default='avc1.42c015',
                      help="Video codec string")
    parser.add_option('', "--audio-sample-rate", metavar='<rate>',
                      dest="audio_sample_rate", type="float", default=44100,
                      help="Audio sample rate (in samples per second)")
    parser.add_option('', "--audio-codec", metavar='<codec>',
                      dest="audio_codec", default='mp4a.40.2',
                      help="Audio codec string")
    parser.add_option('', "--marlin",
                      dest="marlin", action="store_true", default=False,
                      help="Add Marlin signaling to the MPD")
    parser.add_option('', "--exec-dir", metavar="<exec_dir>",
                      dest="exec_dir", default=path.join(SCRIPT_PATH, 'bin', platform),
                      help="Directory where the Bento4 executables are located")
    (options, args) = parser.parse_args()
    if len(args) == 0:
        parser.print_help()
        sys.exit(1)
    global Options
    Options = options
    media_file_names = args
    
    # check the consistency of the options
    if not options.split:
        if not options.use_segment_list:
            print 'WARNING: --no-split requires --use-segment-list, which will be enabled automatically'
            options.use_segment_list = True
            
    settings = Settings()
    #if len(sys.argv) > 1: execfile(sys.argv[1])
        
    if not path.exists(Options.exec_dir):
        PrintErrorAndExit('Executable directory does not exist ('+Options.exec_dir+'), use --exec-dir')

    # parse the media files
    media_files = []
    for media_file in media_file_names:
        print 'Parsing media file', str(1+len(media_files))+':', media_file
        if not os.path.exists(media_file):
            print 'ERROR: media file', media_file, ' does not exist'
            sys.exit(1)
            
        # get the file info
        mp4_file = Mp4File(media_file)
        mp4_file.index = 1+len(media_files)
        #print mp4_file.media_size
        #print mp4_file.bitrate
        
        # check the file
        if mp4_file.info['movie']['fragments'] != True:
            PrintErrorAndExit('file '+str(mp4_file.index)+' is not fragmented (use mp4fragment to fragment it)')
        if mp4_file.video_track_id == 0:
            PrintErrorAndExit('file '+str(mp4_file.index)+' does not have any video track')
            
        # add the file to the file
        media_files.append(mp4_file)
        
    # check that segments are aligned
    audio_track_id = media_files[0].audio_track_id
    video_track_id = media_files[0].video_track_id
    for media_file in media_files:
        if media_file.audio_track_id != audio_track_id:
            PrintErrorAndExit('ERROR: audio track ID mismatch between file '+str(media_files[0].index)+' and '+str(media_file.index))
        if media_file.video_track_id != video_track_id:
            PrintErrorAndExit('ERROR: video track ID mismatch between file '+str(media_files[0].index)+' and '+str(media_file.index))
        if media_files[0].sample_counts[video_track_id] != media_file.sample_counts[video_track_id]:
            PrintErrorAndExit('ERROR: video sample count mismatch between file '+str(media_files[0].index)+' and '+str(media_file.index))
        
    # compute some values if not set
    if options.min_buffer_time == 0.0:
        options.min_buffer_time = media_files[0].segment_duration[media_files[0].video_track_id]
        
    # create the MPD
    mpd = xml.Element('MPD', 
                      xmlns=MPD_NS, 
                      profiles=ISOFF_LIVE_PROFILE, 
                      minBufferTime="PT%.02fS" % (options.min_buffer_time), 
                      mediaPresentationDuration=XmlDuration(int(mp4_file.info['movie']['duration_ms'])/1000),
                      type='static')
    period = xml.SubElement(mpd, 'Period')

    # use the first media file for audio
    adaptation_set = xml.SubElement(period, 'AdaptationSet', mimeType=AUDIO_MIMETYPE, segmentAlignment='true')
    if options.marlin:
        AddContentProtection(adaptation_set, [media_files[0]], 'audio')
    bandwidth = media_files[0].max_segment_bitrate[media_files[0].audio_track_id]
    representation = xml.SubElement(adaptation_set, 'Representation', id='audio', codecs=options.audio_codec, bandwidth=str(bandwidth))
    if options.split:
        AddSegments(representation, 'audio', media_files[0], media_files[0].audio_track_id)
    else:
        AddSegments(representation, None, media_files[0], media_files[0].audio_track_id, use_byte_range=True)
        

    # process all the video
    adaptation_set = xml.SubElement(period, 'AdaptationSet', mimeType=VIDEO_MIMETYPE, segmentAlignment='true', startWithSAP='1')
    if options.marlin:
        AddContentProtection(adaptation_set, media_files, 'video')
    for media_file in media_files:
        bandwidth = media_file.max_segment_bitrate[media_file.video_track_id]
        representation = xml.SubElement(adaptation_set, 'Representation', id='video.'+str(media_file.index), codecs=options.video_codec, bandwidth=str(bandwidth))
        if options.split:
            AddSegments(representation, 'video/'+str(media_file.index), media_file, media_file.video_track_id)
        else:
            AddSegments(representation, None, media_file, media_file.video_track_id, use_byte_range=True)           
    
    if options.verbose:
        for media_file in media_files:
            print 'Media File %d:' % (media_file.index)
            if media_file.audio_track_id:
                print '  Audio Track: max bitrate=%d, avg bitrate=%d' % (media_file.max_segment_bitrate[media_file.audio_track_id], media_file.average_segment_bitrate[media_file.audio_track_id])
            if media_file.video_track_id:
                print '  Video Track: max bitrate=%d, avg bitrate=%d' % (media_file.max_segment_bitrate[media_file.video_track_id], media_file.average_segment_bitrate[media_file.video_track_id])
        
    # create the directories and split the media
    MakeNewDir(options.output_dir, is_warning=options.mpd_only)
    if not options.mpd_only:
        if options.split:
            out_dir = path.join(options.output_dir, 'audio')
            MakeNewDir(out_dir)
            print 'Processing media file (audio)', media_files[0].filename
            Mp4Split(media_file.work_filename,
                     no_track_id   = True,
                     init_segment  = path.join(out_dir, INIT_SEGMENT_NAME),
                     media_segment = path.join(out_dir, SEGMENT_PATTERN),
                     audio         = True)
            MakeNewDir(path.join(options.output_dir, 'video'))
        for media_file in media_files:
            if options.split:
                out_dir = path.join(options.output_dir, 'video', str(media_file.index))
                MakeNewDir(out_dir)
                print 'Processing media file (video)', media_file.filename
                Mp4Split(media_file.work_filename,
                         no_track_id   = True,
                         init_segment  = path.join(out_dir, INIT_SEGMENT_NAME),
                         media_segment = path.join(out_dir, SEGMENT_PATTERN),
                         video         = True)
            else:
                print 'Processing media file', media_file.filename
                shutil.copyfile(media_file.work_filename,
                                path.join(options.output_dir, LINEAR_PATTERN%(media_file.index)))
    
    # save the MPD
    open(path.join(options.output_dir, options.mpd_filename), "wb").write(parseString(xml.tostring(mpd)).toprettyxml("  "))

    #    MakeNewDir(media_output_dir, is_warning=True)
        # create an output directory for the file
    #    media_output_dir = path.join(options.output_dir, str(mp4_file.index))
        

###########################    
if __name__ == '__main__':
    main()
    
    
