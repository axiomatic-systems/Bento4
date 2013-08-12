#!/usr/bin/env python

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2013 Axiomatic Systems, LLC.'

import sys
import os
import os.path as path
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
import hashlib

LanguageCodeMap = {'roh': 'rm', 'zul': 'zu', 'ron': 'ro', 'oss': 'os', 'ita': 'it', 'ssw': 'ss', 'nld': 'nl', 'oji': 'oj', 'oci': 'oc', 'tso': 'ts', 'ell': 'el', 'jav': 'jv', 'hrv': 'hr', 'nor': 'no', 'fij': 'fj', 'fin': 'fi', 'hau': 'ha', 'eus': 'eu', 'amh': 'am', 'bih': 'bh', 'fas': 'fa', 'dan': 'da', 'nob': 'nb', 'ces': 'cs', 'fao': 'fo', 'mol': 'mo', 'bis': 'bi', 'hin': 'hi', 'hye': 'hy', 'guj': 'gu', 'tir': 'ti', 'yor': 'yo', 'srd': 'sc', 'nep': 'ne', 'cre': 'cr', 'div': 'dv', 'bam': 'bm', 'bak': 'ba', 'tel': 'te', 'pan': 'pa', 'aze': 'az', 'ara': 'ar', 'mar': 'mr', 'arg': 'an', 'est': 'et', 'sun': 'su', 'abk': 'ab', 'kur': 'ku', 'smo': 'sm', 'wol': 'wo', 'lub': 'lu', 'lug': 'lg', 'sme': 'se', 'kua': 'kj', 'tsn': 'tn', 'run': 'rn', 'iku': 'iu', 'yid': 'yi', 'tur': 'tr', 'slk': 'sk', 'orm': 'om', 'que': 'qu', 'ori': 'or', 'rus': 'ru', 'asm': 'as', 'pus': 'ps', 'kas': 'ks', 'cos': 'co', 'ile': 'ie', 'ndo': 'ng', 'gla': 'gd', 'bos': 'bs', 'nde': 'nd', 'gle': 'ga', 'dzo': 'dz', 'glg': 'gl', 'ido': 'io', 'srp': 'sr', 'tuk': 'tk', 'wln': 'wa', 'isl': 'is', 'aka': 'ak', 'bod': 'bo', 'glv': 'gv', 'tat': 'tt', 'twi': 'tw', 'vie': 'vi', 'ipk': 'ik', 'por': 'pt', 'uzb': 'uz', 'pol': 'pl', 'sot': 'st', 'mah': 'mh', 'tgk': 'tg', 'bre': 'br', 'tgl': 'tl', 'aym': 'ay', 'cha': 'ch', 'fra': 'fr', 'mkd': 'mk', 'kom': 'kv', 'che': 'ce', 'her': 'hz', 'kon': 'kg', 'swa': 'sw', 'ltz': 'lb', 'swe': 'sv', 'ukr': 'uk', 'ton': 'to', 'chu': 'cu', 'chv': 'cv', 'fry': 'fy', 'kor': 'ko', 'msa': 'ms', 'pli': 'pi', 'heb': 'he', 'hun': 'hu', 'ven': 've', 'hmo': 'ho', 'bul': 'bg', 'iii': 'ii', 'cym': 'cy', 'ben': 'bn', 'mlg': 'mg', 'bel': 'be', 'ibo': 'ig', 'hat': 'ht', 'slv': 'sl', 'som': 'so', 'xho': 'xh', 'deu': 'de', 'cat': 'ca', 'zha': 'za', 'mlt': 'mt', 'aar': 'aa', 'ful': 'ff', 'zho': 'zh', 'nno': 'nn', 'san': 'sa', 'uig': 'ug', 'jpn': 'ja', 'vol': 'vo', 'nbl': 'nr', 'sag': 'sg', 'mya': 'my', 'khm': 'km', 'spa': 'es', 'ind': 'id', 'ave': 'ae', 'tah': 'ty', 'ava': 'av', 'sna': 'sn', 'eng': 'en', 'lim': 'li', 'lin': 'ln', 'ewe': 'ee', 'ina': 'ia', 'lit': 'lt', 'nav': 'nv', 'nau': 'na', 'grn': 'gn', 'nya': 'ny', 'sin': 'si', 'afr': 'af', 'tam': 'ta', 'snd': 'sd', 'lao': 'lo', 'cor': 'kw', 'kir': 'ky', 'kan': 'kn', 'kal': 'kl', 'kik': 'ki', 'sqi': 'sq', 'kin': 'rw', 'kau': 'kr', 'kat': 'ka', 'lat': 'la', 'kaz': 'kk', 'lav': 'lv', 'mal': 'ml', 'urd': 'ur', 'epo': 'eo', 'mri': 'mi', 'tha': 'th', 'mon': 'mn', 'und': ''  , '```': ''}
 
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
    
def Bento4Command(options, name, *args, **kwargs):
    cmd = [path.join(options.exec_dir, name)]
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
    
def Mp4Info(options, filename, **args):
    return Bento4Command(options, 'mp4info', filename, **args)

def Mp4Dump(options, filename, **args):
    return Bento4Command(options, 'mp4dump', filename, **args)

def Mp4Split(options, filename, **args):
    return Bento4Command(options, 'mp4split', filename, **args)

def Mp4Encrypt(options, input_filename, output_filename, **args):
    return Bento4Command(options, 'mp4encrypt', input_filename, output_filename, **args)

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

class Mp4Track:
    def __init__(self, parent, info):
        self.parent = parent
        self.info   = info
        self.default_sample_duration  = 0
        self.timescale                = 0
        self.moofs                    = []
        self.kid                      = None
        self.sample_counts            = []
        self.segment_sizes            = []
        self.segment_durations        = []
        self.segment_scaled_durations = []
        self.segment_bitrates         = []
        self.total_sample_count       = 0
        self.total_duration           = 0
        self.average_segment_duration = 0
        self.average_segment_bitrate  = 0
        self.max_segment_bitrate      = 0
        self.bandwidth                = 0
        self.language                 = ''
        self.id = info['id']
        if info['type'] == 'Audio':
            self.type = 'audio'
        elif info['type'] == 'Video':
            self.type = 'video'
        else:
            self.type = 'other'
        
        sample_desc = info['sample_descriptions'][0]
        if self.type == 'video':
            # get the width and height
            self.width  = sample_desc['width']
            self.height = sample_desc['height']

        if self.type == 'audio':
            self.sample_rate = sample_desc['sample_rate']
            self.channels = sample_desc['channels']
                        
        self.language = info['language']
        
    def update(self, options):
        # compute the total number of samples
        self.total_sample_count = reduce(operator.add, self.sample_counts, 0)
        
        # compute the total duration
        self.total_duration = reduce(operator.add, self.segment_durations, 0)
        
        # compute the average segment durations
        segment_count = len(self.segment_durations)
        if segment_count >= 1:
            # do not count the last segment, which could be shorter
            self.average_segment_duration = reduce(operator.add, self.segment_durations[:-1], 0)/float(segment_count-1)
        elif segment_count == 1:
            self.average_segment_duration = self.segment_durations[0]
    
        # compute the average segment bitrates
        self.media_size = reduce(operator.add, self.segment_sizes, 0)
        if self.total_duration:
            self.average_segment_bitrate = int(8.0*float(self.media_size)/self.total_duration)

        # compute the max segment bitrates
        if len(self.segment_bitrates) > 1:
            self.max_segment_bitrate = max(self.segment_bitrates[:-1])            
        
        # compute bandwidth
        if options.min_buffer_time == 0.0:
            options.min_buffer_time = self.average_segment_duration
        self.bandwidth = ComputeBandwidth(options.min_buffer_time, self.segment_sizes, self.segment_durations)

    def compute_kid(self):
        moov = FilterChildren(self.parent.tree, 'moov')[0]
        traks = FilterChildren(moov, 'trak')
        for trak in traks:
            tkhd = FindChild(trak, ['tkhd'])
            track_id = tkhd['id']
            tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'encv', 'sinf', 'schi', 'tenc'))
            if tenc is None:
                tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'enca', 'sinf', 'schi', 'tenc'))
            if tenc and 'default_KID' in tenc:
                kid = tenc['default_KID'].strip('[]').replace(' ', '')
                self.kid = kid
    
    def __repr__(self):
        return 'File '+str(self.parent.index)+'#'+str(self.id)
    
class Mp4File:
    def __init__(self, options, filename):
        self.filename = filename
        self.tracks   = {}
                
        if options.debug: print 'Processing MP4 file', filename

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
        if options.debug: print '  found', len(self.segments), 'segments'
                        
        # get the mp4 file info
        json_info = Mp4Info(options, filename, format='json')
        self.info = json.loads(json_info, strict=False)

        for track in self.info['tracks']:
            self.tracks[track['id']] = Mp4Track(self, track)

        # get a complete file dump
        json_dump = Mp4Dump(options, filename, format='json', verbosity='1')
        #print json_dump
        self.tree = json.loads(json_dump, strict=False)
        
        # look for KIDs
        for track in self.tracks.itervalues():
            track.compute_kid()
                
        # compute default sample durations and timescales
        for atom in self.tree:
            if atom['name'] == 'moov':
                for c1 in atom['children']:
                    if c1['name'] == 'mvex':
                        for c2 in c1['children']:
                            if c2['name'] == 'trex':
                                self.tracks[c2['track id']].default_sample_duration = c2['default sample duration']
                    elif c1['name'] == 'trak':
                        track_id = 0
                        for c2 in c1['children']:
                            if c2['name'] == 'tkhd':
                                track_id = c2['id']
                        for c2 in c1['children']:
                            if c2['name'] == 'mdia':
                                for c3 in c2['children']:
                                    if c3['name'] == 'mdhd':
                                        self.tracks[track_id].timescale = c3['timescale']

        # partition the segments
        segment_index = 0
        track = None
        segment_size = 0
        segment_duration_sec = 0.0
        for atom in self.tree:
            segment_size += atom['size']
            if atom['name'] == 'moof':
                trafs = FilterChildren(atom, 'traf')
                if len(trafs) != 1:
                    PrintErrorAndExit('ERROR: unsupported input file, more than one "traf" box in fragment')
                tfhd = FilterChildren(trafs[0], 'tfhd')[0]
                track = self.tracks[tfhd['track ID']]
                track.moofs.append(segment_index)
                segment_duration = 0
                default_sample_duration = tfhd.get('default sample duration', track.default_sample_duration)
                for trun in FilterChildren(trafs[0], 'trun'):
                    track.sample_counts.append(trun['sample count'])
                    for (name, value) in trun.items():
                        if name.startswith('entry '):
                            f = value.find('duration:')
                            if f >= 0:
                                f += 9
                                g = value.find(' ', f)
                                if g >= 0:
                                    sample_duration = int(value[f:g])
                            else:
                                sample_duration = default_sample_duration
                            segment_duration += sample_duration
                track.segment_scaled_durations.append(segment_duration)
                segment_duration_sec = float(segment_duration)/float(track.timescale)
                track.segment_durations.append(segment_duration_sec)
                segment_index += 1
            elif atom['name'] == 'mdat':
                # end of fragment on 'mdat' atom
                if track:
                    track.segment_sizes.append(segment_size)
                    if segment_duration_sec > 0.0:
                        segment_bitrate = int((8.0*float(segment_size))/segment_duration_sec)
                    else:
                        segment_bitrate = 0
                    track.segment_bitrates.append(segment_bitrate)
                segment_size = 0
                                                
        # compute the total numer of samples for each track
        for track_id in self.tracks:
            self.tracks[track_id].update(options)
                                                   
        # print debug info if requested
        if options.debug:
            for track in self.tracks.itervalues():
                print '    ID                       =', track.id
                print '    Type                     =', track.type
                print '    Sample Count             =', track.total_sample_count
                print '    Average segment bitrate  =', track.average_segment_bitrate
                print '    Max segment bitrate      =', track.max_segment_bitrate
                print '    Required bandwidth       =', int(track.bandwidth)
                print '    Average segment duration =', track.average_segment_duration

    def find_track_by_id(self, track_id_to_find):
        for track_id in self.tracks:
            if track_id_to_find == 0 or track_id_to_find == track_id:
                return self.tracks[track_id]
        
        return None

    def find_tracks_by_type(self, track_type_to_find):
        return [track for track in self.tracks.values() if track_type_to_find == '' or track_type_to_find == track.type]
            
class MediaSource:
    def __init__(self, name):
        self.name = name
        if name.startswith('[') and ']' in name:
            try:
                params = name[1:name.find(']')]
                self.filename = name[2+len(params):]
                self.spec = dict([x.split('=') for x in params.split(',')])
                for int_param in ['track']:
                    if int_param in self.spec: self.spec[int_param] = int(self.spec[int_param])
            except:
                raise Exception('Invalid syntax for media file spec "'+name+'"')
        else:
            self.filename = name
            self.spec = {}
            
        if 'type'     not in self.spec: self.spec['type']     = ''
        if 'track'    not in self.spec: self.spec['track']    = 0
        if 'language' not in self.spec: self.spec['language'] = ''
        
    def __repr__(self):
        return self.name

def ComputeBandwidth(buffer_time, sizes, durations):
    bandwidth = 0.0
    for i in range(len(sizes)):
        accu_size     = 0
        accu_duration = 0
        buffer_size = (buffer_time*bandwidth)/8.0
        for j in range(i, len(sizes)):
            accu_size     += sizes[j]
            accu_duration += durations[j]
            max_avail = buffer_size+accu_duration*bandwidth/8.0
            if accu_size > max_avail:
                bandwidth = 8.0*(accu_size-buffer_size)/accu_duration
                break
    return int(bandwidth)
    
def MakeNewDir(dir, exit_if_exists=False, severity=None):
    if os.path.exists(dir):
        if severity:
            sys.stderr.write(severity+': ')
            sys.stderr.write('directory "'+dir+'" already exists\n')
        if exit_if_exists:
            sys.exit(1)
    else:
        os.mkdir(dir)        

def DerivePlayReadyKey(seed, kid, swap=True):
    if len(seed) < 30:
        raise Exception('seed must be  >= 30 bytes')
    if len(kid) != 16:
        raise Exception('kid must be 16 bytes')
    
    if swap:
        kid = kid[3]+kid[2]+kid[1]+kid[0]+kid[5]+kid[4]+kid[7]+kid[6]+kid[8:]

    seed = seed[:30]

    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha_A = [ord(x) for x in sha.digest()]
    
    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha.update(seed)
    sha_B = [ord(x) for x in sha.digest()]
    
    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha.update(seed)
    sha.update(kid)
    sha_C = [ord(x) for x in sha.digest()]
        
    content_key = ""
    for i in range(16):
        content_key += chr(sha_A[i] ^ sha_A[i+16] ^ sha_B[i] ^ sha_B[i+16] ^ sha_C[i] ^ sha_C[i+16])

    return content_key

def ComputePlayReadyHeader(header_spec):
    # construct the base64 header
    if os.path.exists(header_spec):
        # read the header from the file
        header = open(header_spec, 'rb').read()
        header_xml = None
        if (ord(header[0]) == 0xff and ord(header[1]) == 0xfe) or (ord(header[0]) == 0xfe and ord(header[1]) == 0xff):
            # this is UTF-16 XML
            header_xml = header.decode('utf-16')
        elif header[0] == '<':
            # this is ASCII or UTF-8 XML
            header_xml = header.decode('utf-8')
        if header_xml is not None:
            # encode the XML header into UTF-16 little-endian
            header_utf16_le = header_xml.encode('utf-16-le')
            rm_record = struct.pack('<HH', 1, len(header_utf16_le))+header_utf16_le
            header = struct.pack('<IH', len(rm_record)+6, 1)+rm_record
        header_b64 = header.encode('base64')
    else:
        header_b64 = header_spec
        header = header_b64.decode('base64')
        if len(header) == 0:
            raise Exception('invalid base64 encoding')

    return header_b64