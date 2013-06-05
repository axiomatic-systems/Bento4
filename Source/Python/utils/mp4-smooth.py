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
VERSION = "1.0.0"
SCRIPT_PATH = path.abspath(path.dirname(__file__))
sys.path += [SCRIPT_PATH]

VIDEO_MIMETYPE           = 'video/mp4'
AUDIO_MIMETYPE           = 'audio/mp4'
VIDEO_DIR                = 'video'
AUDIO_DIR                = 'audio'
SMOOTH_DEFAULT_TIMESCALE = 10000000
SMIL_NAMESPACE           = 'http://www.w3.org/2001/SMIL20/Language'
LINEAR_PATTERN           = 'media-%02d.ismv'
        
Options = None            
def main():
    # determine the platform binary name
    platform = sys.platform
    if platform.startswith('linux'):
        platform = 'linux-x86'
    elif platform.startswith('darwin'):
        platform = 'macosx'
                
    # parse options
    parser = OptionParser(usage="%prog [options] <media-file> [<media-file> ...]")
    parser.add_option('', '--verbose', dest="verbose",
                      action='store_true', default=False,
                      help="Be verbose")
    parser.add_option('', '--debug', dest="debug",
                      action='store_true', default=False,
                      help="Print out debugging information")
    parser.add_option('-o', '--output-dir', dest="output_dir",
                      help="Output directory", metavar="<output-dir>", default='output')
    parser.add_option('-f', '--force', dest="force_output", action="store_true",
                      help="Allow output to an existing directory", default=False)
    parser.add_option('-m', '--client-manifest-name', dest="client_manifest_filename",
                      help="Client Manifest file name", metavar="<filename>", default='stream.ismc')
    parser.add_option('-s', '--server-manifest-name', dest="server_manifest_filename",
                      help="Server Manifest file name", metavar="<filename>", default='stream.ism')
    parser.add_option('', '--no-media', dest="no_media",
                      action='store_true', default=False,
                      help="Do not output media files")
    parser.add_option('', "--split",
                      action="store_true", dest="split", default=False,
                      help="Split the file into segments")
    parser.add_option('', "--encryption-key", dest="encryption_key", metavar='<KID>:<key-or-key-seed>', default=None,
                      help="Encrypt all audio and video tracks with an AES (in hex) or the key derived from the key seed (base64), and set the KID to <KID> (in hex)")
    parser.add_option('', "--playready-header",
                      dest="playready_header_filename", metavar="<filename>", default=None,
                      help="Add PlayReady signaling to the client Manifest, using the PlayReady Header Object in XML read from <filename> (requires an encrypted input, or the --encryption-key option), ")
    parser.add_option('', "--exec-dir", metavar="<exec_dir>",
                      dest="exec_dir", default=path.join(SCRIPT_PATH, 'bin', platform),
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
    encrypted_files = []
    if options.encryption_key:
        if ':' not in options.encryption_key:
            raise Exception('Invalid argument syntax for --encryption-key option')
        kid_hex, key_hex_or_b64 = options.encryption_key.split(':')
        if len(kid_hex) != 32 or (len(key_hex_or_b64) != 32 and len(key_hex_or_b64) < 40):
            raise Exception('Invalid argument format for --encryption-key option')
            
        if len(key_hex_or_b64) != 32:
            # this is a seed, derive the key from it
            key_hex = DerivePlayReadyKey(key_hex_or_b64.decode('base64'), kid_hex.decode('hex')).encode('hex')
        else:
            key_hex = key_hex_or_b64
            
        track_ids = []
        for media_source in media_sources:
            media_file = media_source.filename
            # get the mp4 file info
            json_info = Mp4Info(Options, media_file, format='json')
            info = json.loads(json_info, strict=False)

            track_ids = [track['id'] for track in info['tracks'] if track['type'] in ['Audio', 'Video']]
            if Options.debug:
                print 'KID='+kid_hex+', KEY='+key_hex
            print 'Encrypting track IDs '+str(track_ids)+' in '+ media_file
            encrypted_file = tempfile.NamedTemporaryFile(dir = options.output_dir)
            encrypted_files.append(encrypted_file) # keep a ref so that it does not get deleted
            file_name_map[encrypted_file.name] = encrypted_file.name + ' (Encrypted ' + media_file + ')'
            args = ['--method', 'PIFF-CTR']
            args.append(media_file)
            args.append(encrypted_file.name)
            for track_id in track_ids:
                args += ['--key', str(track_id)+':'+key_hex+':random', '--property', str(track_id)+':KID:'+kid_hex] 
            cmd = [path.join(Options.exec_dir, 'mp4encrypt')] + args
            try:
                check_output(cmd) 
            except CalledProcessError, e:
                raise Exception("binary tool failed with error %d" % e.returncode)
            media_source.filename = encrypted_file.name
            
    # parse the media files
    index = 1
    for media_source in media_sources:
        media_file = media_source.filename
        print 'Parsing media file', str(index)+':', file_name_map[media_file]
        if not os.path.exists(media_file):
            PrintErrorAndExit('ERROR: media file ' + media_file + ' does not exist')
            
        # get the file info
        mp4_file = Mp4File(Options, media_file)
        mp4_file.index = index
        
        # check the file
        if mp4_file.info['movie']['fragments'] != True:
            PrintErrorAndExit('ERROR: file '+str(mp4_file.index)+' is not fragmented (use mp4fragment to fragment it)')
            
        # add the file to the file
        media_source.mp4_file = mp4_file
        
        index += 1
        
    # select the audio and video tracks
    audio_tracks = {}
    video_tracks = []
    for media_source in media_sources:
        track_id   = media_source.spec['track']
        track_type = media_source.spec['type']
        track      = None
        
        if track_type not in ['', 'audio', 'video']:
            sys.stderr.write('WARNING: ignoring source '+media_source.name+', unknown type')

        if track_id:
            track = media_source.mp4_file.find_track_by_id(track_id)
            if not track:
                PrintErrorAndExit('ERROR: track id not found for media file '+media_source.name)

        if track and track_type and track.type != track_type:
            PrintErrorAndExit('ERROR: track type mismatch for media file '+media_source.name)

        audio_track = track
        if track_type == 'audio' or track_type == '':
            if audio_track is None:
                audio_track = media_source.mp4_file.find_track_by_type('audio')
            if audio_track:
                language = media_source.spec['language']
                if language not in audio_tracks:
                    audio_tracks[language] = audio_track
            else:
                if track_type:
                    sys.stderr.write('WARNING: no audio track found in '+media_source.name+'\n')
                    
        # audio tracks with languages don't mix with language-less tracks
        if len(audio_tracks) > 1 and '' in audio_tracks:
            del audio_tracks['']
            
        video_track = track
        if track_type == 'video' or track_type == '':
            if video_track is None:
                video_track = media_source.mp4_file.find_track_by_type('video')
            if video_track:
                video_tracks.append(video_track)
            else:
                if track_type:
                    sys.stderr.write('WARNING: no video track found in '+media_source.name+'\n')
        
    # check that we have at least one audio and one video
    if len(audio_tracks) == 0:
        PrintErrorAndExit('ERROR: no audio track selected')
    if len(video_tracks) == 0:
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
    for video_track in video_tracks:
        for segment_duration in video_track.segment_durations[:-1]:
            ratio = segment_duration/video_track.average_segment_duration
            if ratio > 1.1 or ratio < 0.9:
                sys.stderr.write('WARNING: video segment durations for "' + str(video_track) + '" vary by more than 10%\n')
                break;
    for audio_track in audio_tracks.values():
        for segment_duration in audio_track.segment_durations[:-1]:
            ratio = segment_duration/audio_track.average_segment_duration
            if ratio > 1.1 or ratio < 0.9:
                sys.stderr.write('WARNING: audio segment durations for "' + str(audio_track) + '" vary by more than 10%\n')
                break;
                    
    # compute the total duration (we take the duration of the video)
    presentation_duration = int(float(SMOOTH_DEFAULT_TIMESCALE)*video_tracks[0].total_duration)
        
    # create the Client Manifest
    client_manifest = xml.Element('SmoothStreamingMedia', 
                                  MajorVersion="2", 
                                  MinorVersion="0",
                                  Duration=str(presentation_duration))
    client_manifest.append(xml.Comment('Created with Bento4 mp4-smooth.py, VERSION='+VERSION))
    
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
    
    if options.verbose:
        for audio_track in audio_tracks.itervalues():
            print '  Audio Track: '+str(audio_track)+' - max bitrate=%d, avg bitrate=%d' % (audio_track.max_segment_bitrate, audio_track.average_segment_bitrate)
        for video_track in video_tracks:
            print '  Video Track: '+str(video_track)+' - max bitrate=%d, avg bitrate=%d' % (video_track.max_segment_bitrate, video_track.average_segment_bitrate)
            
    # save the Client Manifest
    if options.client_manifest_filename != '':
        open(path.join(options.output_dir, options.client_manifest_filename), "wb").write(parseString(xml.tostring(client_manifest)).toprettyxml("  "))
        

    # create the Server Manifest file
    server_manifest = xml.Element('smil', xmlns=SMIL_NAMESPACE)
    server_manifest_head = xml.SubElement(server_manifest , 'head')
    xml.SubElement(server_manifest_head, 'meta', name='clientManifestRelativePath', content=path.basename(options.client_manifest_filename))
    server_manifest_body = xml.SubElement(server_manifest , 'body')
    server_manifest_switch = xml.SubElement(server_manifest_body, 'switch')
    for (language, audio_track) in audio_tracks.iteritems():
        audio_entry = xml.SubElement(server_manifest_switch, 'audio', src=LINEAR_PATTERN%(audio_track.parent.index), systemBitrate=str(audio_track.max_segment_bitrate))
        xml.SubElement(audio_entry, 'param', name='trackID', value=str(audio_track.id), valueType='data')
        if language:
            xml.SubElement(audio_entry, 'param', name='trackName', value="audio_"+language, valueType='data')
        if audio_track.timescale != SMOOTH_DEFAULT_TIMESCALE:
            xml.SubElement(audio_entry, 'param', name='timeScale', value=str(audio_track.timescale), valueType='data')
        
    for video_track in video_tracks:
        video_entry = xml.SubElement(server_manifest_switch, 'video', src=LINEAR_PATTERN%(video_track.parent.index), systemBitrate=str(video_track.max_segment_bitrate))
        xml.SubElement(video_entry, 'param', name='trackID', value=str(video_track.id), valueType='data')
        if video_track.timescale != SMOOTH_DEFAULT_TIMESCALE:
            xml.SubElement(video_entry, 'param', name='timeScale', value=str(video_track.timescale), valueType='data')
    
    # save the Manifest
    if options.server_manifest_filename != '':
        open(path.join(options.output_dir, options.server_manifest_filename), "wb").write(parseString(xml.tostring(server_manifest)).toprettyxml("  "))
    
    # copy the media files
    if not options.no_media:
        if options.split:
            MakeNewDir(path.join(options.output_dir, 'audio'))
            for (language, audio_track) in audio_tracks.iteritems():
                out_dir = path.join(options.output_dir, 'audio')
                if len(audio_tracks) > 1:
                    out_dir = path.join(out_dir, language)
                    MakeNewDir(out_dir, severity=None)
                print 'Processing media file (audio)', file_name_map[audio_track.parent.filename]
                Mp4Split(Options, audio_track.parent.filename,
                         track_id               = str(audio_track.id),
                         no_track_id_in_pattern = True,
                         init_segment           = path.join(out_dir, INIT_SEGMENT_NAME),
                         media_segment          = path.join(out_dir, SEGMENT_PATTERN))
        
            MakeNewDir(path.join(options.output_dir, 'video'))
            for video_track in video_tracks:
                out_dir = path.join(options.output_dir, 'video', str(video_track.parent.index))
                MakeNewDir(out_dir)
                print 'Processing media file (video)', file_name_map[video_track.parent.filename]
                Mp4Split(Options, video_track.parent.filename,
                         track_id               = str(video_track.id),
                         no_track_id_in_pattern = True,
                         init_segment           = path.join(out_dir, INIT_SEGMENT_NAME),
                         media_segment          = path.join(out_dir, SEGMENT_PATTERN))
        else:
            for media_source in media_sources:
                print 'Processing media file', file_name_map[media_source.mp4_file.filename]
                shutil.copyfile(media_source.mp4_file.filename,
                                path.join(options.output_dir, LINEAR_PATTERN%(media_source.mp4_file.index)))


###########################    
if __name__ == '__main__':
    try:
        main()
    except Exception, err:
        if Options.debug:
            raise
        else:
            PrintErrorAndExit('ERROR: %s\n' % str(err))
