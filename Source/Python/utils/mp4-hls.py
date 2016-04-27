#!/usr/bin/env python

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2015 Axiomatic Systems, LLC.'

###
# NOTE: this script needs Bento4 command line binaries to run
# You must place the 'mp4info' 'mp4dump', and 'mp42hls' binaries
# in a directory named 'bin/<platform>' at the same level as where
# this script is.
# <platform> depends on the platform you're running on:
# Mac OSX   --> platform = macosx
# Linux x86 --> platform = linux-x86
# Windows   --> platform = win32

from optparse import OptionParser
import shutil
import xml.etree.ElementTree as xml
from xml.dom.minidom import parseString
import tempfile
import fractions
import re
import platform
import sys
from mp4utils import *

# setup main options
VERSION = "1.0.0"
SDK_REVISION = '610'
SCRIPT_PATH = path.abspath(path.dirname(__file__))
sys.path += [SCRIPT_PATH]

#############################################
def OutputMuxedHls(options, media_sources):
    media_data = []

    # process media sources
    for media_source in media_sources:
        if options.verbose:
            print 'Processing', media_source.filename
        media_index = 1+len(media_data)
        data = {
            'source': media_source,
            'index': media_index,
            'dir': 'media-'+str(media_index)
        }
        out_dir = path.join(options.output_dir, data['dir'])
        MakeNewDir(out_dir)

        kwargs = {
            'index_filename':            path.join(out_dir, 'stream.m3u8'),
            'segment_filename_template': path.join(out_dir, 'segment-%d.ts'),
            'segment_url_template':      'segment-%d.ts',
            'show_info':                 True
        }

        if options.hls_version != 3:
            kwargs['hls_version'] = str(Options.hls_version)

        if options.hls_version >= 4:
            kwargs['iframe_index_filename'] = path.join(out_dir, 'iframes.m3u8')

        if options.output_single_file:
            kwargs['segment_filename_template'] = path.join(out_dir, 'media.ts')
            kwargs['segment_url_template']      = 'media.ts'
            kwargs['output_single_file']        = True

        for option in ['encryption_mode', 'encryption_key', 'encryption_iv_mode', 'encryption_key_uri', 'encryption_key_format', 'encryption_key_format_versions']:
            if getattr(options, option):
                kwargs[option] = getattr(options, option)

        json_info = Mp42Hls(options,
                            media_source.filename,
                            **kwargs)
        data['info'] = json.loads(json_info, strict=False)
        media_data.append(data)
        if options.verbose:
            print json_info

        # output the encryption key if needed
        if options.output_encryption_key:
            open(path.join(out_dir, 'key.bin'), 'wb+').write(options.encryption_key.decode('hex')[:16])

    # start the master playlist
    master_playlist = open(path.join(options.output_dir, options.master_playlist_name), "w+")
    master_playlist.write("#EXTM3U\r\n")
    master_playlist.write('# Created with Bento4 mp4-hls.py version '+VERSION+'r'+SDK_REVISION+'\r\n')
    master_playlist.write('\r\n')

    if options.hls_version >= 4:
        master_playlist.write('#EXT-X-VERSION:'+str(options.hls_version)+'\r\n')

    master_playlist.write('\r\n')
    master_playlist.write('# Media Playlists\r\n')
    for data in media_data:
        media_info = data['info']
        codecs = ''
        codec_sep = ''
        if 'audio' in media_info:
            codecs += media_info['audio']['codec']
            codec_sep = ', '
        if 'video' in media_info:
            codecs += codec_sep+media_info['video']['codec']
        ext_x_stream_inf = '#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=%d,BANDWIDTH=%d,CODECS="%s"' % (
                            int(media_info['stats']['avg_segment_bitrate']),
                            int(media_info['stats']['max_segment_bitrate']),
                            codecs)
        if 'video' in media_info:
            ext_x_stream_inf += ',RESOLUTION='+str(int(media_info['video']['width']))+'x'+str(int(media_info['video']['height']))
        master_playlist.write(ext_x_stream_inf+'\r\n')
        master_playlist.write(data['dir']+'/stream.m3u8\r\n')

    # write the I-FRAME playlist info
    if options.hls_version >= 4:
        master_playlist.write('\r\n')
        master_playlist.write('# I-Frame Playlists\r\n')
        for data in media_data:
            media_info = data['info']
            if not 'video' in media_info: continue
            ext_x_i_frame_stream_inf = '#EXT-X-I-FRAME-STREAM-INF:AVERAGE-BANDWIDTH=%d,BANDWIDTH=%d,CODECS="%s",RESOLUTION=%dx%d,URI="%s"' % (
                                        int(media_info['stats']['avg_iframe_bitrate']),
                                        int(media_info['stats']['max_iframe_bitrate']),
                                        media_info['video']['codec'],
                                        int(media_info['video']['width']),
                                        int(media_info['video']['height']),
                                        data['dir']+'/iframes.m3u8')
            master_playlist.write(ext_x_i_frame_stream_inf+'\r\n')

#############################################
Options = None
def main():
    # determine the platform binary name
    host_platform = ''
    if platform.system() == 'Linux':
        if platform.processor() == 'x86_64':
            host_platform = 'linux-x86_64'
        else:
            host_platform = 'linux-x86'
    elif platform.system() == 'Darwin':
        host_platform = 'macosx'
    elif platform.system() == 'Windows':
        host_platform = 'win32'
    default_exec_dir = path.join(SCRIPT_PATH, 'bin', host_platform)
    if not path.exists(default_exec_dir):
        default_exec_dir = path.join(SCRIPT_PATH, 'bin')
    if not path.exists(default_exec_dir):
        default_exec_dir = path.join(SCRIPT_PATH, '..', 'bin')

    # parse options
    parser = OptionParser(usage="%prog [options] <media-file> [<media-file> ...]",
                          description="Each <media-file> is the path to an MP4 file, optionally prefixed with a stream selector delimited by [ and ]. The same input MP4 file may be repeated, provided that the stream selector prefixes select different streams. Version " + VERSION + " r" + SDK_REVISION)
    parser.add_option('-v', '--verbose', dest="verbose", action='store_true', default=False,
                      help="Be verbose")
    parser.add_option('-d', '--debug', dest="debug", action='store_true', default=False,
                      help="Print out debugging information")
    parser.add_option('-o', '--output-dir', dest="output_dir",
                      help="Output directory", metavar="<output-dir>", default='output')
    parser.add_option('-f', '--force', dest="force_output", action="store_true", default=False,
                      help="Allow output to an existing directory")
    parser.add_option('', '--hls-version', dest="hls_version", type="int", metavar="<version>", default=3,
                      help="HLS Version (default: 3)")
    parser.add_option('', '--master-playlist-name', dest="master_playlist_name", metavar="<filename>", default='master.m3u8',
                      help="Master Playlist name")
    parser.add_option('', '--output-single-file', dest="output_single_file", action='store_true', default=False,
                      help="Store segment data in a single output file per input file")
    parser.add_option('', '--encryption-mode', dest="encryption_mode", metavar="<mode>",
                      help="Encryption mode (only used when --encryption-key is specified). AES-128 or SAMPLE-AES (default: AES-128)")
    parser.add_option('', '--encryption-key', dest="encryption_key", metavar="<key>",
                      help="Encryption key in hexadecimal (default: no encryption)")
    parser.add_option('', '--encryption-iv-mode', dest="encryption_iv_mode", metavar="<mode>",
                      help="Encryption IV mode: 'sequence', 'random' or 'fps' (Fairplay Streaming) (default: sequence). When the mode is 'fps', the encryption key must be 32 bytes: 16 bytes for the key followed by 16 bytes for the IV.")
    parser.add_option('', '--encryption-key-uri', dest="encryption_key_uri", metavar="<uri>",
                      help="Encryption key URI (may be a realtive or absolute URI). (default: key.bin)")
    parser.add_option('', '--encryption-key-format', dest="encryption_key_format", metavar="<format>",
                      help="Encryption key format. (default: 'identity')")
    parser.add_option('', '--encryption-key-format-versions', dest="encryption_key_format_versions", metavar="<versions>",
                      help="Encryption key format versions.")
    parser.add_option('', '--output-encryption-key', dest="output_encryption_key", action="store_true", default=False,
                      help="Output the encryption key to a file (default: don't output the key). This option is only valid when the encryption key format is 'identity'")
    parser.add_option('', "--exec-dir", metavar="<exec_dir>", dest="exec_dir", default=default_exec_dir,
                      help="Directory where the Bento4 executables are located")
    (options, args) = parser.parse_args()
    if len(args) == 0:
        parser.print_help()
        sys.exit(1)
    global Options
    Options = options

    # set some mandatory options that utils rely upon
    options.min_buffer_time = 0.0

    if not path.exists(Options.exec_dir):
        PrintErrorAndExit('Executable directory does not exist ('+Options.exec_dir+'), use --exec-dir')

    # check options
    if options.output_encryption_key:
        if options.encryption_key_uri:
            sys.stderr.write("WARNING: the encryption key will not be output because a non-default key URI was specified\n")
            options.output_encryption_key = False
        if not options.encryption_key:
            sys.stderr.write("ERROR: --output-encryption-key requires --encryption-key to be specified\n")
            sys.exit(1)
        if options.encryption_key_format != None and options.encryption_key_format != 'identity':
            sys.stderr.write("ERROR: --output-encryption-key requires --encryption-key-format to be omitted or set to 'identity'\n")
            sys.exit(1)

    # parse media sources syntax
    media_sources = [MediaSource(source) for source in args]

    # create the output directory
    severity = 'ERROR'
    if options.force_output: severity = None
    MakeNewDir(dir=options.output_dir, exit_if_exists = not options.force_output, severity=severity)

    # output the media playlists
    OutputMuxedHls(options, media_sources)

###########################
if sys.version_info[0] != 2:
    sys.stderr.write("ERROR: This tool must be run with Python 2.x\n")
    sys.stderr.write("You are running Python version: "+sys.version+"\n")
    exit(1)

if __name__ == '__main__':
    try:
        main()
    except Exception, err:
        if Options and Options.debug:
            raise
        else:
            PrintErrorAndExit('ERROR: %s\n' % str(err))
