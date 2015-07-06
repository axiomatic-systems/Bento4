#!/usr/bin/env python
import collections

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2015 Axiomatic Systems, LLC.'

import sys
import os
import os.path as path
from subprocess import check_output, CalledProcessError
import json
import io
import struct
import operator
import hashlib
import xml.sax.saxutils as saxutils

LanguageCodeMap = {
    'aar': 'aa', 'abk': 'ab', 'afr': 'af', 'aka': 'ak', 'alb': 'sq', 'amh': 'am', 'ara': 'ar', 'arg': 'an',
    'arm': 'hy', 'asm': 'as', 'ava': 'av', 'ave': 'ae', 'aym': 'ay', 'aze': 'az', 'bak': 'ba', 'bam': 'bm',
    'baq': 'eu', 'bel': 'be', 'ben': 'bn', 'bih': 'bh', 'bis': 'bi', 'bod': 'bo', 'bos': 'bs', 'bre': 'br',
    'bul': 'bg', 'bur': 'my', 'cat': 'ca', 'ces': 'cs', 'cha': 'ch', 'che': 'ce', 'chi': 'zh', 'chu': 'cu',
    'chv': 'cv', 'cor': 'kw', 'cos': 'co', 'cre': 'cr', 'cym': 'cy', 'cze': 'cs', 'dan': 'da', 'deu': 'de',
    'div': 'dv', 'dut': 'nl', 'dzo': 'dz', 'ell': 'el', 'eng': 'en', 'epo': 'eo', 'est': 'et', 'eus': 'eu',
    'ewe': 'ee', 'fao': 'fo', 'fas': 'fa', 'fij': 'fj', 'fin': 'fi', 'fra': 'fr', 'fre': 'fr', 'fry': 'fy',
    'ful': 'ff', 'geo': 'ka', 'ger': 'de', 'gla': 'gd', 'gle': 'ga', 'glg': 'gl', 'glv': 'gv', 'gre': 'el',
    'grn': 'gn', 'guj': 'gu', 'hat': 'ht', 'hau': 'ha', 'heb': 'he', 'her': 'hz', 'hin': 'hi', 'hmo': 'ho',
    'hrv': 'hr', 'hun': 'hu', 'hye': 'hy', 'ibo': 'ig', 'ice': 'is', 'ido': 'io', 'iii': 'ii', 'iku': 'iu',
    'ile': 'ie', 'ina': 'ia', 'ind': 'id', 'ipk': 'ik', 'isl': 'is', 'ita': 'it', 'jav': 'jv', 'jpn': 'ja',
    'kal': 'kl', 'kan': 'kn', 'kas': 'ks', 'kat': 'ka', 'kau': 'kr', 'kaz': 'kk', 'khm': 'km', 'kik': 'ki',
    'kin': 'rw', 'kir': 'ky', 'kom': 'kv', 'kon': 'kg', 'kor': 'ko', 'kua': 'kj', 'kur': 'ku', 'lao': 'lo',
    'lat': 'la', 'lav': 'lv', 'lim': 'li', 'lin': 'ln', 'lit': 'lt', 'ltz': 'lb', 'lub': 'lu', 'lug': 'lg',
    'mac': 'mk', 'mah': 'mh', 'mal': 'ml', 'mao': 'mi', 'mar': 'mr', 'may': 'ms', 'mkd': 'mk', 'mlg': 'mg',
    'mlt': 'mt', 'mon': 'mn', 'mri': 'mi', 'msa': 'ms', 'mya': 'my', 'nau': 'na', 'nav': 'nv', 'nbl': 'nr',
    'nde': 'nd', 'ndo': 'ng', 'nep': 'ne', 'nld': 'nl', 'nno': 'nn', 'nob': 'nb', 'nor': 'no', 'nya': 'ny',
    'oci': 'oc', 'oji': 'oj', 'ori': 'or', 'orm': 'om', 'oss': 'os', 'pan': 'pa', 'per': 'fa', 'pli': 'pi',
    'pol': 'pl', 'por': 'pt', 'pus': 'ps', 'que': 'qu', 'roh': 'rm', 'ron': 'ro', 'rum': 'ro', 'run': 'rn',
    'rus': 'ru', 'sag': 'sg', 'san': 'sa', 'sin': 'si', 'slk': 'sk', 'slo': 'sk', 'slv': 'sl', 'sme': 'se',
    'smo': 'sm', 'sna': 'sn', 'snd': 'sd', 'som': 'so', 'sot': 'st', 'spa': 'es', 'sqi': 'sq', 'srd': 'sc',
    'srp': 'sr', 'ssw': 'ss', 'sun': 'su', 'swa': 'sw', 'swe': 'sv', 'tah': 'ty', 'tam': 'ta', 'tat': 'tt',
    'tel': 'te', 'tgk': 'tg', 'tgl': 'tl', 'tha': 'th', 'tib': 'bo', 'tir': 'ti', 'ton': 'to', 'tsn': 'tn',
    'tso': 'ts', 'tuk': 'tk', 'tur': 'tr', 'twi': 'tw', 'uig': 'ug', 'ukr': 'uk', 'urd': 'ur', 'uzb': 'uz',
    'ven': 've', 'vie': 'vi', 'vol': 'vo', 'wel': 'cy', 'wln': 'wa', 'wol': 'wo', 'xho': 'xh', 'yid': 'yi',
    'yor': 'yo', 'zha': 'za', 'zho': 'zh', 'zul': 'zu', '```': 'und'
}
 
def PrintErrorAndExit(message):
    sys.stderr.write(message+'\n')
    sys.exit(1)
    
def XmlDuration(d):
    h  = int(d)/3600
    d -= h*3600
    m  = int(d)/60
    s  = d-m*60
    xsd = 'PT'
    if h:
        xsd += str(h)+'H'
    if h or m:
        xsd += str(m)+'M'
    if s:
        xsd += ('%.2fS' % (s))
    return xsd
    
def Bento4Command(options, name, *args, **kwargs):
    cmd = [path.join(options.exec_dir, name)]
    for kwarg in kwargs:
        arg = kwarg.replace('_', '-')
        cmd.append('--'+arg)
        if not isinstance(kwargs[kwarg], bool):
            cmd.append(kwargs[kwarg])
    cmd += args
    if options.debug:
        print 'COMMAND: ', cmd
    try:
        return check_output(cmd) 
    except CalledProcessError, e:
        message = "binary tool failed with error %d" % e.returncode
        if options.verbose:
            message += " - " + str(cmd)
        raise Exception(message)
    
def Mp4Info(options, filename, **args):
    return Bento4Command(options, 'mp4info', filename, **args)

def Mp4Dump(options, filename, **args):
    return Bento4Command(options, 'mp4dump', filename, **args)

def Mp4Split(options, filename, **args):
    return Bento4Command(options, 'mp4split', filename, **args)

def Mp4Fragment(options, input_filename, output_filename, **args):
    return Bento4Command(options, 'mp4fragment', input_filename, output_filename, **args)

def Mp4Encrypt(options, input_filename, output_filename, **args):
    return Bento4Command(options, 'mp4encrypt', input_filename, output_filename, **args)

class Mp4Atom:
    def __init__(self, type, size, position):
        self.type     = type
        self.size     = size
        self.position = position

    def __str__(self):
        return 'ATOM: ' + self.type + ',' + str(self.size) + '@' + str(self.position)


def WalkAtoms(filename, until=None):
    cursor = 0
    atoms = []
    file = io.FileIO(filename, "rb")
    while True:
        try:
            size = struct.unpack('>I', file.read(4))[0]
            type = file.read(4)
            if type == until:
                break
            if size == 1:
                size = struct.unpack('>Q', file.read(8))[0]
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
        self.media_size               = 0
        self.average_segment_duration = 0
        self.average_segment_bitrate  = 0
        self.max_segment_bitrate      = 0
        self.bandwidth                = 0
        self.language                 = ''
        self.order_index              = 0
        self.id = info['id']
        if info['type'] == 'Audio':
            self.type = 'audio'
        elif info['type'] == 'Video':
            self.type = 'video'
        elif info['type'] == 'Subtitles':
            self.type = 'subtitles'
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
        if segment_count > 2:
            # do not count the last two segments, which could be shorter
            self.average_segment_duration = reduce(operator.add, self.segment_durations[:-2], 0)/float(segment_count-2)
        elif segment_count > 0:
            self.average_segment_duration = self.segment_durations[0]
        else:
            self.average_segment_duration = 0
            
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
            tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'encv', 'sinf', 'schi', 'tenc'))
            if tenc is None:
                tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'enca', 'sinf', 'schi', 'tenc'))
            if tenc and 'default_KID' in tenc:
                self.kid = tenc['default_KID'].strip('[]').replace(' ', '')

    def __repr__(self):
        return 'File '+str(self.parent.file_list_index)+'#'+str(self.id)
    
class Mp4File:
    def __init__(self, options, media_source):
        self.media_source    = media_source
        self.tracks          = {}
        self.file_list_index = 0 # used to keep a sequence number just amongst all sources

        filename = media_source.filename
        if options.debug:
            print 'Processing MP4 file', filename

        # by default, the media name is the basename of the source file
        self.media_name = os.path.basename(filename)

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
        if options.debug:
            print '  found', len(self.segments), 'segments'
                        
        # get the mp4 file info
        json_info = Mp4Info(options, filename, format='json', fast=True)
        self.info = json.loads(json_info, strict=False, object_pairs_hook=collections.OrderedDict)

        for track in self.info['tracks']:
            self.tracks[track['id']] = Mp4Track(self, track)

        # get a complete file dump
        json_dump = Mp4Dump(options, filename, format='json', verbosity='1')
        #print json_dump
        self.tree = json.loads(json_dump, strict=False, object_pairs_hook=collections.OrderedDict)
        
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
                        if name[0] in '0123456789':
                            sample_duration = -1
                            fields = value.split(',')
                            for field in fields:
                                if field.startswith('d:'):
                                    sample_duration = int(field[2:])
                            if sample_duration == -1:
                                sample_duration = default_sample_duration
                            segment_duration += sample_duration
                track.segment_scaled_durations.append(segment_duration)
                segment_duration_sec = float(segment_duration) / float(track.timescale)
                track.segment_durations.append(segment_duration_sec)
                segment_index += 1

                # remove the 'trun' entries to save some memory
                for traf in trafs:
                    traf['children'] = [x for x in traf['children'] if x['name'] != 'trun']

            elif atom['name'] == 'mdat':
                # end of fragment on 'mdat' atom
                if track:
                    track.segment_sizes.append(segment_size)
                    if segment_duration_sec > 0.0:
                        segment_bitrate = int((8.0 * float(segment_size)) / segment_duration_sec)
                    else:
                        segment_bitrate = 0
                    track.segment_bitrates.append(segment_bitrate)
                segment_size = 0
                                                
        # parse the 'mfra' index if there is one and update segment durations.
        # this is needed to deal with input files that have an 'mfra' index that
        # does not exactly match the sample durations (because of rounding errors),
        # which will make the Smooth Streaming URL mapping fail since the IIS Smooth Streaming
        # server uses the 'mfra' index to locate the segments in the source .ismv file
        mfra = FindChild(self.tree, ['mfra'])
        if mfra:
            for tfra in FilterChildren(mfra, 'tfra'):
                track_id = tfra['track_ID']
                if track_id not in self.tracks:
                    continue
                track = self.tracks[track_id]
                moof_pointers = []
                for (name, value) in tfra.items():
                    if name.startswith('['):
                        attributes = value.split(',')
                        attribute_dict = {}
                        for attribute in attributes:
                            (attribute_name, attribute_value) = attribute.strip().split('=')
                            attribute_dict[attribute_name] = int(attribute_value)
                        if attribute_dict['traf_number'] == 1 and attribute_dict['trun_number'] == 1 and attribute_dict['sample_number'] == 1:
                            # this points to the first sample of the first trun of the first traf, use it as a start time indication
                            moof_pointers.append(attribute_dict)
                if len(moof_pointers) > 1:
                    for i in range(len(moof_pointers)-1):
                        if i+1 >= len(track.moofs):
                            break

                        moof1 = self.segments[track.moofs[i]][0]
                        moof2 = self.segments[track.moofs[i+1]][0]
                        if moof1.position == moof_pointers[i]['moof_offset'] and moof2.position == moof_pointers[i+1]['moof_offset']:
                            # pointers match two consecutive moofs
                            moof_duration = moof_pointers[i+1]['time'] - moof_pointers[i]['time']
                            moof_duration_sec = float(moof_duration) / float(track.timescale)
                            track.segment_durations[i] = moof_duration_sec
                            track.segment_scaled_durations[i] = moof_duration

        # compute the total numer of samples for each track
        for track_id in self.tracks:
            self.tracks[track_id].update(options)
                                                   
        # print debug info if requested
        if options.debug:
            for track in self.tracks.itervalues():
                print 'Track ID                     =', track.id
                print '    Segment Count            =', len(track.segment_durations)
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
            
        if 'type'           not in self.spec: self.spec['type']     = ''
        if 'track'          not in self.spec: self.spec['track']    = 0
        if 'language'       not in self.spec: self.spec['language'] = ''
        
        # keep a record of our original filename in case it gets changed later
        self.original_filename = self.filename
        
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

def MakePsshBox(system_id, payload):
    pssh_size = 12+16+4+len(payload)
    return struct.pack('>I', pssh_size)+'pssh'+struct.pack('>I',0)+system_id+struct.pack('>I', len(payload))+payload

def GetEncryptionKey(options, spec):
    if options.debug:
        print 'Resolving KID and Key from spec:', spec
    if spec.startswith('skm:'):
        import skm
        return skm.ResolveKey(options, spec[4:])
    else:
        raise Exception('Key Locator scheme not supported')

def ComputeMarlinPssh(options):
    # create a dummy (empty) Marlin PSSH
    return struct.pack('>I4sI4sII', 24, 'marl', 16, 'mkid', 0, 0)

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

def ComputePlayReadyChecksum(kid, key):
    import aes
    return aes.rijndael(key).encrypt(kid)[:8]

def WrapPlayreadyHeaderXml(header_xml):
    # encode the XML header into UTF-16 little-endian
    header_utf16_le = header_xml.encode('utf-16-le')
    rm_record = struct.pack('<HH', 1, len(header_utf16_le))+header_utf16_le
    return struct.pack('<IH', len(rm_record)+6, 1)+rm_record

def ComputePlayReadyHeader(header_spec, kid_hex, key_hex):
    # construct the base64 header
    if header_spec is None:
        header_spec = ''
    if header_spec.startswith('#'):
        header_b64 = header_spec[1:]
        header = header_b64.decode('base64')
        if len(header) == 0:
            raise Exception('invalid base64 encoding')
        return header
    elif os.path.exists(header_spec):
        # read the header from the file
        header = open(header_spec, 'rb').read()
        header_xml = None
        if (ord(header[0]) == 0xff and ord(header[1]) == 0xfe) or (ord(header[0]) == 0xfe and ord(header[1]) == 0xff):
            # this is UTF-16 XML
            header_xml = header.decode('utf-16')
        elif header[0] == '<' and ord(header[1]) != 0x00:
            # this is ASCII or UTF-8 XML
            header_xml = header.decode('utf-8')
        elif header[0] == '<' and ord(header[1]) == 0x00:
            # this UTF-16LE XML without charset header
            header_xml = header.decode('utf-16-le')
        if header_xml is not None:
            header = WrapPlayreadyHeaderXml(header_xml)
        return header
    else:
        try:
            pairs = header_spec.split('#')
            fields = {}
            for pair in pairs:
                if len(pair) == 0: continue
                name, value = pair.split(':', 1)
                fields[name] = value
        except:
            raise Exception('invalid syntax for argument')

        header_xml = '<WRMHEADER xmlns="http://schemas.microsoft.com/DRM/2007/03/PlayReadyHeader" version="4.0.0.0"><DATA><PROTECTINFO><KEYLEN>16</KEYLEN><ALGID>AESCTR</ALGID></PROTECTINFO>'

        kid = kid_hex.decode('hex')
        kid = kid[3]+kid[2]+kid[1]+kid[0]+kid[5]+kid[4]+kid[7]+kid[6]+kid[8:]
        header_xml += '<KID>'+kid.encode('base64').replace('\n', '')+'</KID>'
        if key_hex:
            header_xml += '<CHECKSUM>'+ComputePlayReadyChecksum(kid, key_hex.decode('hex')).encode('base64').replace('\n', '')+'</CHECKSUM>'

        if 'CUSTOMATTRIBUTES' in fields:
            header_xml += '<CUSTOMATTRIBUTES>'+fields['CUSTOMATTRIBUTES'].decode('base64').replace('\n', '')+'</CUSTOMATTRIBUTES>'
        if 'LA_URL' in fields:
            header_xml += '<LA_URL>'+saxutils.escape(fields['LA_URL'])+'</LA_URL>'
        if 'LUI_URL' in fields:
            header_xml += '<LUI_URL>'+saxutils.escape(fields['LUI_URL'])+'</LUI_URL>'
        if 'DS_ID' in fields:
            header_xml += '<DS_ID>'+saxutils.escape(fields['DS_ID'])+'</DS_ID>'

        header_xml += '</DATA></WRMHEADER>'
        return WrapPlayreadyHeaderXml(header_xml)

    return ""

def WidevineVarInt(value):
    parts = [value % 128]
    value >>= 7
    while value:
        parts.append(value%128)
        value >>= 7
    varint = ''
    for i in range(len(parts)-1):
        parts[i] |= (1<<7)
    varint = ''
    for x in parts:
        varint += chr(x)
    return varint

def WidevineMakeHeader(fields):
    buffer = ''
    for (field_num, field_val) in fields:
        if type(field_val) == int and field_val < 256:
            wire_type = 0 # varint
            wire_val = WidevineVarInt(field_val)
        elif type(field_val) == str:
            wire_type = 2
            wire_val = WidevineVarInt(len(field_val))+field_val
        buffer += chr(field_num<<3 | wire_type) + wire_val
    return buffer

def ComputeWidevineHeader(header_spec, kid_hex, key_hex):
    # construct the base64 header
    if header_spec.startswith('#'):
        header_b64 = header_spec[1:]
        header = header_b64.decode('base64')
        if len(header) == 0:
            raise Exception('invalid base64 encoding')
        return header
    else:
        try:
            pairs = header_spec.split('#')
            fields = {}
            for pair in pairs:
                name, value = pair.split(':', 1)
                fields[name] = value
        except:
            raise Exception('invalid syntax for argument')

        protobuf_fields = [(1, 1), (2, kid_hex.decode('hex'))]
        if 'provider' in fields:
            protobuf_fields.append((3, fields['provider']))
        if 'content_id' in fields:
            protobuf_fields.append((4, fields['content_id'].decode('hex')))
        if 'policy' in fields:
            protobuf_fields.append((6, fields['policy']))
        return WidevineMakeHeader(protobuf_fields)
    
    return ""    