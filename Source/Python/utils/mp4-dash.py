#!/usr/bin/env python

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2012 Axiomatic Systems, LLC.'

###
# NOTE: this script needs Bento4 command line binaries to run
# You must place the 'mp4info' 'mp4dump', 'mp4encrypt' and 'mp4split' binaries
# in a directory named 'bin/<platform>' at the same level as where
# this script is.
# <platform> depends on the platform you're running on:
# Mac OSX   --> platform = macosx
# Linux x86 --> platform = linux-x86
# Windows   --> platform = win32

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
import tempfile

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

def Mp4Encrypt(input_filename, output_filename, **args):
    return Bento4Command('mp4encrypt', input_filename, output_filename, **args)

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
        self.filename          = filename
        self.work_filename     = filename
        self.video_track       = None
        self.video_track_id    = 0
        self.video_width       = 0
        self.video_height      = 0
        self.audio_track       = None
        self.audio_track_id    = 0
        
        if Options.debug: print 'Processing MP4 file', filename

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
        if Options.debug: print '  found', len(self.segments), 'segments'
                        
        # get the mp4 file info
        json_info = Mp4Info(filename, format='json')
        self.info = json.loads(json_info, strict=False)

        for track in self.info['tracks']:
            if track['type'] == 'Video' and self.video_track is None:
                self.video_track = track
                self.video_track_id = track['id']
            elif track['type'] == 'Audio' and self.audio_track is None:
                self.audio_track = track
                self.audio_track_id = track['id']

        # get a complete file dump
        json_dump = Mp4Dump(filename, format='json', verbosity='1')
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
                
        # compute default sample durations and timescales
        self.default_sample_duration = {
            self.audio_track_id:0,
            self.video_track_id:0
        }
        self.timescale = {
            self.audio_track_id:0,
            self.video_track_id:0
        }
        for atom in self.tree:
            if atom['name'] == 'moov':
                for c1 in atom['children']:
                    if c1['name'] == 'mvex':
                        for c2 in c1['children']:
                            if c2['name'] == 'trex':
                                self.default_sample_duration[c2['track id']] = c2['default sample duration']
                    elif c1['name'] == 'trak':
                        track_id = 0
                        for c2 in c1['children']:
                            if c2['name'] == 'tkhd':
                                track_id = c2['id']
                        for c2 in c1['children']:
                            if c2['name'] == 'mdia':
                                for c3 in c2['children']:
                                    if c3['name'] == 'mdhd':
                                        self.timescale[track_id] = c3['timescale']

        # partition the segments
        segment_index = 0
        track_id      = 0
        self.moofs             = { self.audio_track_id:[],  self.video_track_id:[]}
        self.sample_counts     = { self.audio_track_id:[],  self.video_track_id:[]}
        self.segment_sizes     = { self.audio_track_id:[],  self.video_track_id:[]}
        self.segment_durations = { self.audio_track_id:[],  self.video_track_id:[]}
        for atom in self.tree:
            if atom['name'] == 'moof':
                trafs = FilterChildren(atom, 'traf')
                if len(trafs) != 1:
                    PrintErrorAndExit('ERROR: unsupported input file, more than one "traf" box in fragment')
                tfhd = FilterChildren(trafs[0], 'tfhd')[0]
                track_id = tfhd['track ID']
                self.moofs[track_id].append(segment_index)
                self.segment_sizes[track_id].append(0)
                segment_duration = 0
                for trun in FilterChildren(trafs[0], 'trun'):
                    self.sample_counts[track_id].append(trun['sample count'])
                    for (name, value) in trun.items():
                        if name.startswith('entry '):
                            sample_duration = self.default_sample_duration[track_id]
                            f = value.find('duration:')
                            if f >= 0:
                                f += 9
                                g = value.find(' ', f)
                                if g >= 0:
                                    sample_duration = int(value[f:g])    
                            segment_duration += sample_duration
                self.segment_durations[track_id].append(float(segment_duration)/float(self.timescale[track_id]))
                segment_index += 1
            else:
                if track_id and len(self.segment_sizes[track_id]):
                    self.segment_sizes[track_id][-1] += atom['size']
                                                
        # check that we have at least one segment
        if len(self.sample_counts[self.audio_track_id]) == 0 or len(self.sample_counts[self.video_track_id]) == 0:
            return
            
        # compute the total numer of samples for each track
        self.total_samples = {
            self.audio_track_id: reduce(operator.add, self.sample_counts[self.audio_track_id], 0),
            self.video_track_id: reduce(operator.add, self.sample_counts[self.video_track_id], 0)
        }

        # compute the total duration for each track
        self.total_duration = {
            self.audio_track_id: reduce(operator.add, self.segment_durations[self.audio_track_id], 0),
            self.video_track_id: reduce(operator.add, self.segment_durations[self.video_track_id], 0)
        }
                           
        # compute the average segment durations
        self.average_segment_duration = { self.audio_track_id:0, self.video_track_id:0 }
        for track_id in [self.audio_track_id, self.video_track_id]:
            segment_count = len(self.segment_durations[track_id])
            if segment_count >= 1:
                # do not count the last segment, which could be shorter
                self.average_segment_duration[track_id] = reduce(operator.add, self.segment_durations[track_id][:-1], 0)/float(segment_count-1)
            elif segment_count == 1:
                self.average_segment_duration[track_id] = self.segment_durations[track_id][0]
            
        # check the difference between audio and video segment durations
        gap = self.average_segment_duration[self.audio_track_id]-self.average_segment_duration[self.video_track_id]
        if gap > 0.1:
            print 'WARNING: audio and video segment durations are not equal ('+str(self.average_segment_duration[self.audio_track_id])+' and '+str(self.average_segment_duration[self.video_track_id])+')'

        # compute the average segment bitrates
        self.media_size = {
            self.audio_track_id: reduce(operator.add, self.segment_sizes[self.audio_track_id], 0),
            self.video_track_id: reduce(operator.add, self.segment_sizes[self.video_track_id], 0)
        }
        self.average_segment_bitrate = { self.audio_track_id:0, self.video_track_id:0 }
        for track_id in [self.audio_track_id, self.video_track_id]:
            if self.total_samples[track_id]:
                self.average_segment_bitrate[track_id] = int(8.0*float(self.media_size[track_id])/self.total_duration[track_id])

        # compute the max segment bitrates
        self.max_segment_bitrate = { self.audio_track_id:0.0, self.video_track_id:0.0 }
        for track_id in [self.audio_track_id, self.video_track_id]:
            if self.average_segment_duration[track_id]:
                self.max_segment_bitrate[track_id] = 8*int(float(max(self.segment_sizes[track_id][:-1]))/self.average_segment_duration[track_id])

        # print debug info if requested
        if Options.debug:
            for t in [('Audio track', self.audio_track_id), ('Video track', self.video_track_id)]:
                print t[0]+':'
                print '    ID                       =', t[1]
                print '    Sample Count             =', self.total_samples[t[1]]
                print '    Average segment bitrate  =', self.average_segment_bitrate[t[1]]
                print '    Max segment bitrate      =', self.average_segment_bitrate[t[1]]
                print '    Average segment duration =', self.average_segment_duration[t[1]]

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
                                  duration=str(int(media_file.average_segment_duration[track_id]*1000)))
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
                                  duration=str(int(media_file.average_segment_duration[track_id]*1000)),
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
        platform = 'linux-x86'
    elif platform.startswith('darwin'):
        platform = 'macosx'
                
    # parse options
    parser = OptionParser(usage="%prog [options] <filename> [<filename> ...]")
    parser.add_option('', '--verbose', dest="verbose",
                      action='store_true', default=False,
                      help="Be verbose")
    parser.add_option('', '--debug', dest="debug",
                      action='store_true', default=False,
                      help="Print out debugging information")
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
                      help="Minimum buffer time (in seconds)")
    parser.add_option('', "--video-codec", metavar='<codec>',
                      dest="video_codec", default=None,
                      help="Video codec string")
    parser.add_option('', "--audio-codec", metavar='<codec>',
                      dest="audio_codec", default=None,
                      help="Audio codec string")
    parser.add_option('', "--encryption-key", dest="encryption_key", metavar='<KID>:<key>', default=None,
                      help="Encrypt all audio and video tracks with AES key <key> (in hex) with KID <KID> (in hex)")
    parser.add_option('', "--marlin",
                      dest="marlin", action="store_true", default=False,
                      help="Add Marlin signaling to the MPD (requires an encrypted input, or the --encryption-key option)")
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
                    
    if not path.exists(Options.exec_dir):
        PrintErrorAndExit('Executable directory does not exist ('+Options.exec_dir+'), use --exec-dir')

    # create the output directory
    MakeNewDir(options.output_dir, is_warning=options.mpd_only)

    # keep track of media file names (in case we use temporary files when encrypting)
    file_name_map = {}
    for media_file_name in media_file_names:
        file_name_map[media_file_name] = media_file_name

    # encrypt the input files if needed
    encrypted_files = []
    if options.encryption_key:
        if ':' not in options.encryption_key:
            raise Exception('Invalid argument syntax for --encryption-key option')
        kid_b64, key_b64 = options.encryption_key.split(':')
        if len(kid_b64) != 32 or len(key_b64) != 32:
            raise Exception('Invalid argument format for --encryption-key option')
            
        track_ids = []
        for media_file in media_file_names:
            # get the mp4 file info
            json_info = Mp4Info(media_file, format='json')
            info = json.loads(json_info, strict=False)

            track_ids = [track['id'] for track in info['tracks'] if track['type'] in ['Audio', 'Video']]
            print 'Encrypting track IDs '+str(track_ids)+' in '+ media_file
            encrypted_file = tempfile.NamedTemporaryFile(dir = options.output_dir)
            encrypted_files.append(encrypted_file)
            file_name_map[encrypted_file.name] = encrypted_file.name + ' (Encrypted ' + media_file + ')'
            args = ['--method', 'MPEG-CENC']
            args.append(media_file)
            args.append(encrypted_file.name)
            for track_id in track_ids:
                args += ['--key', str(track_id)+':'+key_b64+':random', '--property', str(track_id)+':KID:'+kid_b64] 
            cmd = [path.join(Options.exec_dir, 'mp4encrypt')] + args
            try:
                check_output(cmd) 
            except CalledProcessError, e:
                raise Exception("binary tool failed with error %d" % e.returncode)
    
        # point to the encrypted files
        media_file_names = [f.name for f in encrypted_files]
            
    # parse the media files
    media_files = []
    for media_file in media_file_names:
        print 'Parsing media file', str(1+len(media_files))+':', file_name_map[media_file]
        if not os.path.exists(media_file):
            print 'ERROR: media file', media_file, ' does not exist'
            sys.exit(1)
            
        # get the file info
        mp4_file = Mp4File(media_file)
        mp4_file.index = 1+len(media_files)
        
        # check the file
        if mp4_file.info['movie']['fragments'] != True:
            PrintErrorAndExit('file '+str(mp4_file.index)+' is not fragmented (use mp4fragment to fragment it)')
        if mp4_file.video_track_id == 0:
            PrintErrorAndExit('file '+str(mp4_file.index)+' does not have any video track')
            
        # add the file to the file
        media_files.append(mp4_file)
        
    # select the file that we will use as the audio and video references
    audio_ref = None
    video_ref = None
    for media_file in media_files:
        if audio_ref is None and media_file.audio_track is not None:
            audio_ref = media_file
        if video_ref is None and media_file.video_track is not None:
            video_ref = media_file
    if audio_ref is None:
        PrintErrorAndExit('ERROR: none of the files contain an audio track')        
    if video_ref is None:
        PrintErrorAndExit('ERROR: none of the files contain a video track')        
    
    # check that segments are consistent between files
    audio_track_id = audio_ref.audio_track_id
    video_track_id = video_ref.video_track_id
    for media_file in media_files:
        if media_file.video_track_id != video_track_id:
            PrintErrorAndExit('ERROR: video track ID mismatch between file '+str(video_ref.index)+' and '+str(media_file.index))
        if video_ref.sample_counts[video_track_id] != media_file.sample_counts[video_track_id]:
            PrintErrorAndExit('ERROR: video sample count mismatch between file '+str(video_ref.index)+' and '+str(media_file.index))
        
    # check that the segment durations are almost all equal
    average_video_segment_duration = video_ref.average_segment_duration[video_track_id]
    for media_file in media_files:
        for segment_duration in media_file.segment_durations[video_track_id][:-1]:
            ratio = segment_duration/average_video_segment_duration
            if ratio > 1.1 or ratio < 0.9:
                print 'WARNING: segment durations for', media_file.index, 'vary by more than 10%'
    average_audio_segment_duration = audio_ref.average_segment_duration[audio_track_id]
    for segment_duration in audio_ref.segment_durations[audio_track_id][:-1]:
        ratio = segment_duration/average_audio_segment_duration
        if ratio > 1.1 or ratio < 0.9:
            print 'WARNING: segment durations for', media_file.index, 'vary by more than 10%'
    
    # compute the audio codec
    audio_codec = options.audio_codec
    if audio_codec is None and audio_ref:
        audio_desc = audio_ref.audio_track['sample_descriptions'][0]
        if audio_desc['coding'] == 'mp4a':
            audio_codec = 'mp4a.%02x' % (audio_desc['object_type'])
            if audio_desc['object_type'] == 64:
                audio_codec += '.%02x' % (audio_desc['mpeg_4_audio_object_type'])
    if audio_codec is None:
        PrintErrorAndExit('ERROR: unable to determine the audio codec')
                    
    # compute the total duration
    presentation_duration = 0.0
    for media_file in media_files:
        if media_file.total_duration[video_track_id] > presentation_duration:
            presentation_duration = media_file.total_duration[video_track_id]
        
    # compute some values if not set
    if options.min_buffer_time == 0.0:
        options.min_buffer_time = video_ref.average_segment_duration[video_track_id]
        
    # create the MPD
    mpd = xml.Element('MPD', 
                      xmlns=MPD_NS, 
                      profiles=ISOFF_LIVE_PROFILE, 
                      minBufferTime="PT%.02fS" % (options.min_buffer_time), 
                      mediaPresentationDuration=XmlDuration(int(presentation_duration)),
                      type='static')
    period = xml.SubElement(mpd, 'Period')

    # process the audio
    adaptation_set = xml.SubElement(period, 'AdaptationSet', mimeType=AUDIO_MIMETYPE, segmentAlignment='true')
    if options.marlin:
        AddContentProtection(adaptation_set, [audio_ref], 'audio')
    bandwidth = audio_ref.max_segment_bitrate[audio_track_id]
    representation = xml.SubElement(adaptation_set, 'Representation', id='audio', codecs=audio_codec, bandwidth=str(bandwidth))
    if options.split:
        AddSegments(representation, 'audio', audio_ref, audio_track_id)
    else:
        AddSegments(representation, None, audio_ref, audio_track_id, use_byte_range=True)
        
    # process all the video
    adaptation_set = xml.SubElement(period, 'AdaptationSet', mimeType=VIDEO_MIMETYPE, segmentAlignment='true', startWithSAP='1')
    if options.marlin:
        AddContentProtection(adaptation_set, media_files, 'video')
    for media_file in media_files:
        video_desc = media_file.video_track['sample_descriptions'][0]

        # compute the codecs
        video_codec = options.video_codec
        if video_codec is None:
            if video_desc['coding'].startswith('avc'):
                video_codec = video_desc['coding']+'.%02x%02x%02x'%(video_desc['avc_profile'], video_desc['avc_profile_compat'], video_desc['avc_level'])
        if video_codec is None:
            PrintErrorAndExit('ERROR: unable to determine the video codec')

        # get the width and height
        video_width  = video_desc['width']
        video_height = video_desc['height']
            
        
        bandwidth = media_file.max_segment_bitrate[media_file.video_track_id]
        representation = xml.SubElement(adaptation_set, 'Representation', id='video.'+str(media_file.index), codecs=video_codec, width=str(video_width), height=str(video_height), bandwidth=str(bandwidth))
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
    if not options.mpd_only:
        if options.split:
            out_dir = path.join(options.output_dir, 'audio')
            MakeNewDir(out_dir)
            print 'Processing media file (audio)', file_name_map[audio_ref.filename]
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
                print 'Processing media file (video)', file_name_map[media_file.filename]
                Mp4Split(media_file.work_filename,
                         no_track_id   = True,
                         init_segment  = path.join(out_dir, INIT_SEGMENT_NAME),
                         media_segment = path.join(out_dir, SEGMENT_PATTERN),
                         video         = True)
            else:
                print 'Processing media file', file_name_map[media_file.filename]
                shutil.copyfile(media_file.work_filename,
                                path.join(options.output_dir, LINEAR_PATTERN%(media_file.index)))
    
    # save the MPD
    open(path.join(options.output_dir, options.mpd_filename), "wb").write(parseString(xml.tostring(mpd)).toprettyxml("  "))
        

###########################    
if __name__ == '__main__':
    main()
    
    
