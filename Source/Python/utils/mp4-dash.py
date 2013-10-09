#!/usr/bin/env python

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2013 Axiomatic Systems, LLC.'

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
from mp4utils import *

# setup main options
VERSION = "1.3.0"
SVN_REVISION = "$Revision$"
SCRIPT_PATH = path.abspath(path.dirname(__file__))
sys.path += [SCRIPT_PATH]

VIDEO_MIMETYPE           = 'video/mp4'
AUDIO_MIMETYPE           = 'audio/mp4'
VIDEO_DIR                = 'video'
AUDIO_DIR                = 'audio'
MPD_NS_COMPAT            = 'urn:mpeg:DASH:schema:MPD:2011'
MPD_NS                   = 'urn:mpeg:dash:schema:mpd:2011'
ISOFF_LIVE_PROFILE       = 'urn:mpeg:dash:profile:isoff-live:2011' 
INIT_SEGMENT_NAME        = 'init.mp4'
SEGMENT_PATTERN          = 'seg-%04llu.m4f'
SEGMENT_URL_PATTERN      = 'seg-%04d.m4f'
SEGMENT_TEMPLATE         = 'seg-$Number%04d$.m4f'
MEDIA_FILE_PATTERN       = 'media-%02d.mp4'
MARLIN_SCHEME_ID_URI     = 'urn:uuid:5E629AF5-38DA-4063-8977-97FFBD9902D4'
MARLIN_MAS_NAMESPACE     = 'urn:marlin:mas:1-0:services:schemas:mpd'
PLAYREADY_SCHEME_ID_URI  = 'urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95'
PLAYREADY_MSPR_NAMESPACE = 'urn:microsoft:playready'
SMOOTH_INIT_FILE_PATTERN = 'init-%02d-%02d.mp4'
SMOOTH_DEFAULT_TIMESCALE = 10000000
SMIL_NAMESPACE           = 'http://www.w3.org/2001/SMIL20/Language'
             
TempFiles = []

#############################################
def AddSegmentList(options, container, subdir, track, use_byte_range=False):
    if subdir:
        prefix = subdir+'/'
    else:
        prefix = ''
    segment_list = xml.SubElement(container,
                                  'SegmentList',
                                  timescale='1000',
                                  duration=str(int(track.average_segment_duration*1000)))
    if use_byte_range:
        byte_range=str(track.parent.init_segment.position)+'-'+str(track.parent.init_segment.position+track.parent.init_segment.size-1)
        xml.SubElement(segment_list, 'Initialization', sourceURL=prefix+track.parent.media_name, range=byte_range)
    else:
        xml.SubElement(segment_list, 'Initialization', sourceURL=prefix+INIT_SEGMENT_NAME)
    i = 0
    for segment_index in track.moofs:
        segment = track.parent.segments[segment_index]
        segment_offset = segment[0].position
        segment_length = reduce(operator.add, [atom.size for atom in segment], 0)
        if use_byte_range:
            byte_range = str(segment_offset)+'-'+str(segment_offset+segment_length-1)
            xml.SubElement(segment_list, 'SegmentURL', media=prefix+track.parent.media_name, mediaRange=byte_range)
        else:
            xml.SubElement(segment_list, 'SegmentURL', media=prefix+(SEGMENT_URL_PATTERN % (i)))
        i += 1

#############################################
def AddSegmentTemplate(options, container, subdir, track, stream_name):
    if subdir:
        prefix = subdir+'/'
    else:
        prefix = ''
    if options.use_segment_timeline:
        url_template=prefix+SEGMENT_TEMPLATE
        init_segment_url=prefix+INIT_SEGMENT_NAME
        use_template_numbers = True
        if options.smooth:
            url_base = path.basename(options.smooth_server_manifest_filename)
            url_template=url_base+"/QualityLevels($Bandwidth$)/Fragments(%s=$Time$)" % (stream_name)
            init_segment_url=SMOOTH_INIT_FILE_PATTERN%(track.parent.index, track.id)
            use_template_numbers = False
        args = [container, 'SegmentTemplate']
        kwargs = {'timescale': str(track.timescale),
                  'initialization': init_segment_url,
                  'media': url_template}
        if use_template_numbers:
            kwargs['startNumber'] = '0'
        segment_template = xml.SubElement(*args, **kwargs)
        segment_timeline = xml.SubElement(segment_template, 'SegmentTimeline')
        repeat_count = 0
        for i in range(len(track.segment_scaled_durations)):
            duration = track.segment_scaled_durations[i]
            if i+1 < len(track.segment_scaled_durations) and duration == track.segment_scaled_durations[i+1]:
                repeat_count += 1
            else:
                args = [segment_timeline, 'S']
                kwargs = {'d':str(duration)}
                if repeat_count:
                    kwargs['r'] = str(repeat_count)

                xml.SubElement(*args, **kwargs)
                repeat_count = 0
    else:
        xml.SubElement(container,
                       'SegmentTemplate',
                       timescale='1000',
                       duration=str(int(track.average_segment_duration*1000)),
                       startNumber='0',
                       initialization=prefix+INIT_SEGMENT_NAME,
                       media=prefix+SEGMENT_TEMPLATE)

#############################################
def AddSegments(options, container, subdir, track, use_byte_range, stream_name):
    if options.use_segment_list:
        AddSegmentList(options, container, subdir, track, use_byte_range)
    else:
        AddSegmentTemplate(options, container, subdir, track, stream_name)
    
#############################################
def AddContentProtection(options, container, tracks):
    kids = []
    for track in tracks:
        kid = track.kid
        if kid is None:
            PrintErrorAndExit('ERROR: no encryption info found in track '+str(track))
        if kid not in kids:
            kids.append(kid)
    xml.register_namespace('mas', MARLIN_MAS_NAMESPACE)
    xml.register_namespace('mspr', PLAYREADY_MSPR_NAMESPACE)
    #xml.SubElement(container, 'ContentProtection', schemeIdUri='urn:mpeg:dash:mp4protection:2011', value='cenc')
    
    if options.marlin:
        cp = xml.SubElement(container, 'ContentProtection', schemeIdUri=MARLIN_SCHEME_ID_URI)
        cids = xml.SubElement(cp, '{'+MARLIN_MAS_NAMESPACE+'}MarlinContentIds')
        for kid in kids:
            cid = xml.SubElement(cids, '{'+MARLIN_MAS_NAMESPACE+'}MarlinContentId')
            cid.text = 'urn:marlin:kid:'+kid
    if options.playready_header:
        header_b64 = ComputePlayReadyHeader(options.playready_header)        
        cp = xml.SubElement(container, 'ContentProtection', schemeIdUri=PLAYREADY_SCHEME_ID_URI)
        pro = xml.SubElement(cp, '{'+PLAYREADY_MSPR_NAMESPACE+'}pro')
        pro.text = header_b64
                
#############################################
def OutputDash(options, audio_tracks, video_tracks):
    # compute the total duration (we take the duration of the video)
    presentation_duration = video_tracks[0].total_duration
        
    # create the MPD
    if options.use_compat_namespace:
      mpd_ns = MPD_NS_COMPAT
    else:
      mpd_ns = MPD_NS
    mpd = xml.Element('MPD', 
                      xmlns=mpd_ns, 
                      profiles=ISOFF_LIVE_PROFILE, 
                      minBufferTime="PT%.02fS" % (options.min_buffer_time), 
                      mediaPresentationDuration=XmlDuration(int(presentation_duration)),
                      type='static')
    mpd.append(xml.Comment(' Created with Bento4 mp4-dash.py, VERSION='+VERSION+'-'+SVN_REVISION[11:-1]+' '))
    period = xml.SubElement(mpd, 'Period')

    # process the audio tracks
    for (language, audio_track) in audio_tracks.iteritems():
        args = [period, 'AdaptationSet']
        kwargs = {'mimeType': AUDIO_MIMETYPE, 'startWithSAP':'1', 'segmentAlignment':'true'}
        if language:
            id_ext = '.'+language
            kwargs['lang'] = language
        else:
            id_ext = ''
        adaptation_set = xml.SubElement(*args, **kwargs)
        if options.marlin or options.playready_header:
            AddContentProtection(options, adaptation_set, [audio_track])
        representation = xml.SubElement(adaptation_set, 'Representation', id='audio'+id_ext, codecs=audio_track.codec, bandwidth=str(audio_track.bandwidth))
        if language:
            subdir = '/'+language
            stream_name='audio_'+language
        else:
            subdir = ''
            stream_name='audio'
        if options.split:
            AddSegments(options, representation, 'audio'+subdir, audio_track, False, stream_name)
        else:
            AddSegments(options, representation, None, audio_track, True, stream_name)
        
    # process all the video tracks
    adaptation_set = xml.SubElement(period, 'AdaptationSet', mimeType=VIDEO_MIMETYPE, segmentAlignment='true', startWithSAP='1')
    if options.marlin or options.playready_header:
        AddContentProtection(options, adaptation_set, video_tracks)
    for video_track in video_tracks:
        video_desc = video_track.info['sample_descriptions'][0]

        representation = xml.SubElement(adaptation_set, 'Representation', id='video.'+str(video_track.parent.index), codecs=video_track.codec, width=str(video_track.width), height=str(video_track.height), bandwidth=str(video_track.bandwidth))
        if options.split:
            AddSegments(options, representation, 'video/'+str(video_track.parent.index), video_track, False, 'video')
        else:
            AddSegments(options, representation, None, video_track, True, 'video')           
        
    # save the MPD
    if options.mpd_filename:
        open(path.join(options.output_dir, options.mpd_filename), "wb").write(parseString(xml.tostring(mpd)).toprettyxml("  "))

#############################################
def OutputSmooth(options, audio_tracks, video_tracks):
    # compute the total duration (we take the duration of the video)
    presentation_duration = video_tracks[0].total_duration

    # create the Client Manifest
    client_manifest = xml.Element('SmoothStreamingMedia', 
                                  MajorVersion="2", 
                                  MinorVersion="0",
                                  Duration=str(presentation_duration))
    client_manifest.append(xml.Comment(' Created with Bento4 mp4-dash.py, VERSION='+VERSION+'-'+SVN_REVISION[11:-1]+' '))
    
    # process the audio tracks
    audio_index = 0
    for (language, audio_track) in audio_tracks.iteritems():
        if language:
            id_ext = "."+language
            stream_name = "audio_"+language
        else:
            id_ext = ''
            stream_name = "audio"
        audio_url_pattern="QualityLevels({bitrate})/Fragments(%s={start time})" % (stream_name)
        stream_index = xml.SubElement(client_manifest, 
                                      'StreamIndex', 
                                      Chunks=str(len(audio_track.moofs)), 
                                      Url=audio_url_pattern, 
                                      Type="audio", 
                                      Name=stream_name, 
                                      QualityLevels="1",
                                      TimeScale=str(audio_track.timescale))
        if language:
            stream_index.set('Language', language)
        quality_level = xml.SubElement(stream_index, 
                                       'QualityLevel', 
                                       Bitrate=str(audio_track.bandwidth), 
                                       SamplingRate=str(audio_track.sample_rate),
                                       Channels=str(audio_track.channels), 
                                       BitsPerSample="16", 
                                       PacketSize="4", 
                                       AudioTag="255", 
                                       FourCC="AACL",
                                       Index="0",
                                       CodecPrivateData=audio_track.info['sample_descriptions'][0]['decoder_info'])

        for duration in audio_track.segment_scaled_durations:
            xml.SubElement(stream_index, "c", d=str(duration))
        
    # process all the video tracks
    max_width  = max([track.width  for track in video_tracks])
    max_height = max([track.height for track in video_tracks])
    video_url_pattern="QualityLevels({bitrate})/Fragments(video={start time})"
    stream_index = xml.SubElement(client_manifest, 
                                  'StreamIndex',
                                   Chunks=str(len(video_tracks[0].moofs)), 
                                   Url=video_url_pattern, 
                                   Type="video", 
                                   Name="video", 
                                   QualityLevels=str(len(video_tracks)),
                                   TimeScale=str(video_tracks[0].timescale),
                                   MaxWidth=str(max_width),
                                   MaxHeight=str(max_height))
    qindex = 0
    for video_track in video_tracks:
        sample_desc = video_track.info['sample_descriptions'][0]
        codec_private_data = '00000001'+sample_desc['avc_sps'][0]+'00000001'+sample_desc['avc_pps'][0]
        quality_level = xml.SubElement(stream_index, 
                                       'QualityLevel', 
                                       Bitrate=str(video_track.bandwidth),
                                       MaxWidth=str(video_track.width), 
                                       MaxHeight=str(video_track.height),
                                       FourCC="H264",
                                       CodecPrivateData=codec_private_data,
                                       Index=str(qindex))
        qindex += 1

    for duration in video_tracks[0].segment_scaled_durations:
        xml.SubElement(stream_index, "c", d=str(duration))
                
    if options.playready_header:
        header_b64 = ComputePlayReadyHeader(options.playready_header)
        protection = xml.SubElement(client_manifest, 'Protection')
        protection_header = xml.SubElement(protection, 'ProtectionHeader', SystemID='9a04f079-9840-4286-ab92-e65be0885f95')
        protection_header.text = header_b64
        
    # save the Client Manifest
    if options.smooth_client_manifest_filename != '':
        open(path.join(options.output_dir, options.smooth_client_manifest_filename), "wb").write(parseString(xml.tostring(client_manifest)).toprettyxml("  "))
        
    # create the Server Manifest file
    server_manifest = xml.Element('smil', xmlns=SMIL_NAMESPACE)
    server_manifest_head = xml.SubElement(server_manifest , 'head')
    xml.SubElement(server_manifest_head, 'meta', name='clientManifestRelativePath', content=path.basename(options.smooth_client_manifest_filename))
    server_manifest_body = xml.SubElement(server_manifest , 'body')
    server_manifest_switch = xml.SubElement(server_manifest_body, 'switch')
    for (language, audio_track) in audio_tracks.iteritems():
        audio_entry = xml.SubElement(server_manifest_switch, 'audio', src=audio_track.parent.media_name, systemBitrate=str(audio_track.bandwidth))
        xml.SubElement(audio_entry, 'param', name='trackID', value=str(audio_track.id), valueType='data')
        if language:
            xml.SubElement(audio_entry, 'param', name='trackName', value="audio_"+language, valueType='data')
        if audio_track.timescale != SMOOTH_DEFAULT_TIMESCALE:
            xml.SubElement(audio_entry, 'param', name='timeScale', value=str(audio_track.timescale), valueType='data')
        
    for video_track in video_tracks:
        video_entry = xml.SubElement(server_manifest_switch, 'video', src=video_track.parent.media_name, systemBitrate=str(video_track.bandwidth))
        xml.SubElement(video_entry, 'param', name='trackID', value=str(video_track.id), valueType='data')
        if video_track.timescale != SMOOTH_DEFAULT_TIMESCALE:
            xml.SubElement(video_entry, 'param', name='timeScale', value=str(video_track.timescale), valueType='data')
    
    # save the Manifest
    if options.smooth_server_manifest_filename != '':
        open(path.join(options.output_dir, options.smooth_server_manifest_filename), "wb").write(parseString(xml.tostring(server_manifest)).toprettyxml("  "))
    
#############################################
Options = None            
def main():
    # determine the platform binary name
    platform = sys.platform
    if platform.startswith('linux'):
        platform = 'linux-x86'
    elif platform.startswith('darwin'):
        platform = 'macosx'
                
    # parse options
    parser = OptionParser(usage="%prog [options] <media-file> [<media-file> ...]",
                          description="Each <media-file> is the path to a fragmented MP4 file, optionally prefixed with a stream selector delimited by [ and ]. The same input MP4 file may be repeated, provided that the stream selector prefixes select different streams")
    parser.add_option('', '--verbose', dest="verbose", action='store_true', default=False,
                      help="Be verbose")
    parser.add_option('', '--debug', dest="debug", action='store_true', default=False,
                      help="Print out debugging information")
    parser.add_option('-o', '--output-dir', dest="output_dir",
                      help="Output directory", metavar="<output-dir>", default='output')
    parser.add_option('-f', '--force', dest="force_output", action="store_true",
                      help="Allow output to an existing directory", default=False)
    parser.add_option('', '--init-segment', dest="init_segment",
                      help="Initialization segment name", metavar="<filename>", default='init.mp4')
    parser.add_option('-m', '--mpd-name', dest="mpd_filename",
                      help="MPD file name", metavar="<filename>", default='stream.mpd')
    parser.add_option('', '--no-media', dest="no_media", action='store_true', default=False,
                      help="Do not output media files (MPD/Manifests only)")
    parser.add_option('', '--rename-media', dest='rename_media', action='store_true', default=False,
                      help = 'Use a file name pattern instead of the base name of input files for output media files.')
    parser.add_option('', "--no-split", action="store_false", dest="split", default=True,
                      help="Do not split the file into segments")
    parser.add_option('', "--use-segment-list", action="store_true", dest="use_segment_list", default=False,
                      help="Use segment lists instead of segment templates")
    parser.add_option('', "--use-segment-timeline", action="store_true", dest="use_segment_timeline", default=False,
                      help="Use segment timelines (necessary if segment durations vary)")
    parser.add_option('', "--min-buffer-time", metavar='<duration>', dest="min_buffer_time", type="float", default=0.0,
                      help="Minimum buffer time (in seconds)")
    parser.add_option('', "--video-codec", metavar='<codec>', dest="video_codec", default=None,
                      help="Video codec string")
    parser.add_option('', "--audio-codec", metavar='<codec>', dest="audio_codec", default=None,
                      help="Audio codec string")
    parser.add_option('', "--smooth", dest="smooth", default=False, action="store_true", 
                      help="Produce a Smooth Streaming compatible output")
    parser.add_option('', "--smooth-piff-cenc", dest="smooth_piff_cenc", default=False, action="store_true", 
                      help="When using encryption, produce a variant of PIFF compatible with CENC")
    parser.add_option('', '--smooth-client-manifest-name', dest="smooth_client_manifest_filename",
                      help="Smooth Streaming Client Manifest file name", metavar="<filename>", default='stream.ismc')
    parser.add_option('', '--smooth-server-manifest-name', dest="smooth_server_manifest_filename",
                      help="Smooth Streaming Server Manifest file name", metavar="<filename>", default='stream.ism')
    parser.add_option('', "--encryption-key", dest="encryption_key", metavar='<KID>:<key>', default=None,
                      help="Encrypt all audio and video tracks with AES key <key> (in hex) and KID <KID> (in hex)")
    parser.add_option('', "--use-compat-namespace", dest="use_compat_namespace", action="store_true", default=False,
                      help="Use the original DASH MPD namespace as it was specified in the first published specification")
    parser.add_option('', "--marlin", dest="marlin", action="store_true", default=False,
                      help="Add Marlin signaling to the MPD (requires an encrypted input, or the --encryption-key option)")
    parser.add_option('', "--playready-header", dest="playready_header", metavar='<playready-header-object>', default=None,
                      help="Add PlayReady signaling to the MPD (requires an encrypted input, or the --encryption-key option). The <playready-header-object> argument can be the name of a file containing either a PlayReady XML Rights Management Header or a PlayReady Header Object (PRO),  or a header object itself encoded in Base64")
    parser.add_option('', "--exec-dir", metavar="<exec_dir>", dest="exec_dir", default=path.join(SCRIPT_PATH, 'bin', platform),
                      help="Directory where the Bento4 executables are located")
    (options, args) = parser.parse_args()
    if len(args) == 0:
        parser.print_help()
        sys.exit(1)
    global Options
    Options = options
    
    # parse media sources syntax
    media_sources = [MediaSource(source) for source in args]
    
    # check the consistency of the options
    if options.smooth:
        options.split = False
        options.use_segment_timeline = True
        if options.use_segment_list:
            raise Exception('ERROR: --smooth and --use-segment-list are mutually exclusive')
    if not options.split:
        if not options.smooth and not options.use_segment_list:
            sys.stderr.write('WARNING: --no-split requires --use-segment-list, which will be enabled automatically\n')
            options.use_segment_list = True
                    
    if not path.exists(Options.exec_dir):
        PrintErrorAndExit('Executable directory does not exist ('+Options.exec_dir+'), use --exec-dir')
            
    # create the output directory
    severity = 'ERROR'
    if options.no_media: severity = 'WARNING'
    if options.force_output: severity = None
    MakeNewDir(dir=options.output_dir, exit_if_exists = not (options.no_media or options.force_output), severity=severity)

    # keep track of media file names (in case we use temporary files when encrypting)
    file_name_map = {}
    for media_source in media_sources:
        file_name_map[media_source.filename] = media_source.filename

    # encrypt the input files if needed
    encrypted_files = {}
    if not options.no_media and options.encryption_key:
        if ':' not in options.encryption_key:
            raise Exception('Invalid argument syntax for --encryption-key option')
        kid_b64, key_b64 = options.encryption_key.split(':')
        if len(kid_b64) != 32 or len(key_b64) != 32:
            raise Exception('Invalid argument format for --encryption-key option')
            
        track_ids = []
        for media_source in media_sources:
            media_file = media_source.filename
            
            # check if we have already encrypted this file
            if media_file in encrypted_files:
                media_source.filename = encrypted_files[media_file].name
                continue
                
            # get the mp4 file info
            json_info = Mp4Info(Options, media_file, format='json')
            info = json.loads(json_info, strict=False)

            track_ids = [track['id'] for track in info['tracks'] if track['type'] in ['Audio', 'Video']]
            print 'Encrypting track IDs '+str(track_ids)+' in '+ media_file
            encrypted_file = tempfile.NamedTemporaryFile(dir = options.output_dir, delete=False)
            encrypted_files[media_file] = encrypted_file 
            TempFiles.append(encrypted_file.name)
            encrypted_file.close() # necessary on Windows
            file_name_map[encrypted_file.name] = encrypted_file.name + ' (Encrypted ' + media_file + ')'
            if (options.smooth_piff_cenc):
                args = ['--method', 'PIFF-CTR', '--global-option', 'piff.cenc-compatible:true']
            else:
                args = ['--method', 'MPEG-CENC']
            args.append(media_file)
            args.append(encrypted_file.name)
            for track_id in track_ids:
                args += ['--key', str(track_id)+':'+key_b64+':random', '--property', str(track_id)+':KID:'+kid_b64] 
            cmd = [path.join(Options.exec_dir, 'mp4encrypt')] + args
            try:
                check_output(cmd) 
            except CalledProcessError, e:
                message = "binary tool failed with error %d" % e.returncode
                if options.verbose:
                    message += " - " + str(cmd)
                raise Exception(message)
            media_source.filename = encrypted_file.name
            
    # parse the media files
    index = 1
    mp4_files = {}
    mp4_media_names = []
    for media_source in media_sources:
        media_file = media_source.filename
        
        # check if we have already parsed this file
        if media_file in mp4_files:
            media_source.mp4_file = mp4_files[media_file]
            continue
        
        # parse the file
        print 'Parsing media file', str(index)+':', file_name_map[media_file]
        if not os.path.exists(media_file):
            PrintErrorAndExit('ERROR: media file ' + media_file + ' does not exist')
            
        # get the file info
        mp4_file = Mp4File(Options, media_file)
        
        # set some metadata properties for this file
        mp4_file.index = index
        if options.rename_media:
            mp4_file.media_name = MEDIA_FILE_PATTERN % (mp4_file.index)
        elif 'media' in media_source.spec:
            mp4_file.media_name = media_source.spec['media']
        else:
            mp4_file.media_name = path.basename(media_source.original_filename)
            
        if not options.split:
            if mp4_file.media_name in mp4_media_names:
                PrintErrorAndExit('ERROR: output media name %s is not unique, consider using --rename-media'%mp4_file.media_name)
        
        # check the file
        if mp4_file.info['movie']['fragments'] != True:
            PrintErrorAndExit('ERROR: file '+str(mp4_file.index)+' is not fragmented (use mp4fragment to fragment it)')
            
        # set the source property
        media_source.mp4_file = mp4_file
        mp4_files[media_file] = mp4_file
        mp4_media_names.append(mp4_file.media_name)
        
        index += 1
        
    # select the audio and video tracks
    audio_tracks = {}
    video_tracks = []
    for media_source in media_sources:
        track_id       = media_source.spec['track']
        track_type     = media_source.spec['type']
        track_language = media_source.spec['language']
        tracks         = []
        
        if track_type not in ['', 'audio', 'video']:
            sys.stderr.write('WARNING: ignoring source '+media_source.name+', unknown type')

        if track_id and track_type:
            PrintErrorAndExit('ERROR: track ID and track type selections are mutually exclusive')

        if track_id:
            tracks = [media_source.mp4_file.find_track_by_id(track_id)]
            if not tracks:
                PrintErrorAndExit('ERROR: track id not found for media file '+media_source.name)
        
        if track_type:
            tracks = media_source.mp4_file.find_tracks_by_type(track_type)
            if not tracks:
                PrintErrorAndExit('ERROR: no ' + track_type + ' found for media file '+media_source.name)
        
        if not tracks:
            tracks = media_source.mp4_file.tracks.values()
            
        # process audio tracks
        for track in [t for t in tracks if t.type == 'audio']:
            language = LanguageCodeMap.get(track.language, track.language)
            if track_language and track_language != language:
                continue
            if language not in audio_tracks:
                audio_tracks[language] = track
                    
        # audio tracks with languages don't mix with language-less tracks
        if len(audio_tracks) > 1 and '' in audio_tracks:
            sys.stderr.write('WARNING: removing audio tracks with an unspecified language\n')
            del audio_tracks['']
            
        # process video tracks
        video_tracks += [t for t in tracks if t.type == 'video']
        
    # check that we have at least one audio and one video
    if not audio_tracks:
        PrintErrorAndExit('ERROR: no audio track selected')
    if not video_tracks:
        PrintErrorAndExit('ERROR: no video track selected')
        
    if Options.verbose:
        print 'Audio:', audio_tracks
        print 'Video:', video_tracks
        
    # check that segments are consistent between files
    prev_track = None
    for track in video_tracks:
        if prev_track:
            if track.total_sample_count != prev_track.total_sample_count:
                sys.stderr.write('WARNING: video sample count mismatch between "'+str(track)+'" and "'+str(prev_track)+'"\n')
        prev_track = track
        
    # check that the video segments match
    for track in video_tracks:
        if track.sample_counts[:-1] != video_tracks[0].sample_counts[:-1]:
            PrintErrorAndExit('ERROR: video tracks are not aligned ("'+str(track)+'" differs)')
               
    # check that the video segment durations are almost all equal
    if not options.use_segment_timeline:
        for video_track in video_tracks:
            for segment_duration in video_track.segment_durations[:-1]:
                ratio = segment_duration/video_track.average_segment_duration
                if ratio > 1.1 or ratio < 0.9:
                    sys.stderr.write('WARNING: video segment durations for "' + str(video_track) + '" vary by more than 10% (consider using --use-segment-timeline)\n')
                    break;
        for audio_track in audio_tracks.values():
            for segment_duration in audio_track.segment_durations[:-1]:
                ratio = segment_duration/audio_track.average_segment_duration
                if ratio > 1.1 or ratio < 0.9:
                    sys.stderr.write('WARNING: audio segment durations for "' + str(audio_track) + '" vary by more than 10% (consider using --use-segment-timeline)\n')
                    break;
    
    # compute the audio codecs
    for audio_track in audio_tracks.values(): 
        audio_codec = options.audio_codec
        if audio_codec is None:
            audio_desc = audio_track.info['sample_descriptions'][0]
            if audio_desc['coding'] == 'mp4a':
                audio_codec = 'mp4a.%02x' % (audio_desc['object_type'])
                if audio_desc['object_type'] == 64:
                    audio_codec += '.%02x' % (audio_desc['mpeg_4_audio_object_type'])
        if audio_codec is None:
            PrintErrorAndExit('ERROR: unable to determine the audio codec for "'+str(audio_track)+'"')
        audio_track.codec = audio_codec
        
    # compute the video codecs and dimensions
    for video_track in video_tracks:
        video_codec = options.video_codec
        if video_codec is None:
            video_desc = video_track.info['sample_descriptions'][0]
            if video_desc['coding'].startswith('avc'):
                video_codec = video_desc['coding']+'.%02x%02x%02x'%(video_desc['avc_profile'], video_desc['avc_profile_compat'], video_desc['avc_level'])
        if video_codec is None:
            PrintErrorAndExit('ERROR: unable to determine the video codec for "'+str(video_track)+'"')
        video_track.codec = video_codec

        # get the width and height
        video_track.width  = video_desc['width']
        video_track.height = video_desc['height']
        
    # compute some values if not set
    if options.min_buffer_time == 0.0:
        options.min_buffer_time = video_tracks[0].average_segment_duration
                
    # print info about the tracks
    if options.verbose:
        for audio_track in audio_tracks.itervalues():
            print '  Audio Track: '+str(audio_track)+' - max bitrate=%d, avg bitrate=%d, req bandwidth=%d' % (audio_track.max_segment_bitrate, audio_track.average_segment_bitrate, audio_track.bandwidth)
        for video_track in video_tracks:
            print '  Video Track: '+str(video_track)+' - max bitrate=%d, avg bitrate=%d, req bandwidth=%d' % (video_track.max_segment_bitrate, video_track.average_segment_bitrate, video_track.bandwidth)

    # output the DASH MPD
    OutputDash(options, audio_tracks, video_tracks)

    # output the Smooth Manifests
    if options.smooth:
        OutputSmooth(options, audio_tracks, video_tracks)
    
    # create the directories and split the media
    if not options.no_media:
        if options.split:
            MakeNewDir(path.join(options.output_dir, 'audio'))
            for (language, audio_track) in audio_tracks.iteritems():
                out_dir = path.join(options.output_dir, 'audio')
                if language:
                    out_dir = path.join(out_dir, language)
                    MakeNewDir(out_dir)
                print 'Processing media file (audio)', file_name_map[audio_track.parent.filename]
                Mp4Split(options,
                         audio_track.parent.filename,
                         track_id               = str(audio_track.id),
                         pattern_parameters     = 'N',
                         init_segment           = path.join(out_dir, INIT_SEGMENT_NAME),
                         media_segment          = path.join(out_dir, SEGMENT_PATTERN))
        
            MakeNewDir(path.join(options.output_dir, 'video'))
            for video_track in video_tracks:
                out_dir = path.join(options.output_dir, 'video', str(video_track.parent.index))
                MakeNewDir(out_dir)
                print 'Processing media file (video)', file_name_map[video_track.parent.filename]
                Mp4Split(Options,
                         video_track.parent.filename,
                         track_id               = str(video_track.id),
                         pattern_parameters     = 'N',
                         init_segment           = path.join(out_dir, INIT_SEGMENT_NAME),
                         media_segment          = path.join(out_dir, SEGMENT_PATTERN))
        else:
            for mp4_file in mp4_files.values():
                print 'Processing media file', file_name_map[mp4_file.filename]
                media_filename = path.join(options.output_dir, mp4_file.media_name)
                if not options.force_output and path.exists(media_filename):
                    PrintErrorAndExit('ERROR: file '+media_filename+' already exists')
                    
                shutil.copyfile(mp4_file.filename, media_filename)
            if options.smooth:
                for track in audio_tracks.values() + video_tracks:
                    Mp4Split(options,
                             track.parent.filename,
                             track_id     = str(track.id),
                             init_only    = True,
                             init_segment = path.join(options.output_dir, SMOOTH_INIT_FILE_PATTERN%(track.parent.index, track.id)))

###########################    
if __name__ == '__main__':
    try:
        main()
    except Exception, err:
        if Options.debug:
            raise
        else:
            PrintErrorAndExit('ERROR: %s\n' % str(err))
    finally:
        for f in TempFiles:
            os.unlink(f)
