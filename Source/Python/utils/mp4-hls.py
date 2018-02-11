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
from subtitles import *

# setup main options
VERSION = "1.1.0"
SDK_REVISION = '622'
SCRIPT_PATH = path.abspath(path.dirname(__file__))
sys.path += [SCRIPT_PATH]

#############################################
def CreateSubtitlesPlaylist(playlist_filename, webvtt_filename, duration):
    playlist = open(playlist_filename, 'wb+')
    playlist.write('#EXTM3U\r\n')
    playlist.write('#EXT-X-TARGETDURATION:%d\r\n' % (duration))
    playlist.write('#EXT-X-VERSION:3\r\n')
    playlist.write('#EXT-X-MEDIA-SEQUENCE:0\r\n')
    playlist.write('#EXT-X-PLAYLIST-TYPE:VOD\r\n')
    playlist.write('#EXTINF:%d,\r\n' % (duration))
    playlist.write(webvtt_filename+'\r\n')
    playlist.write('#EXT-X-ENDLIST\r\n')


#############################################
def ComputeCodecName(codec_family):
    name = codec_family
    if codec_family == 'mp4a':
        name = 'aac'
    elif codec_family == 'ac-3':
        name = 'ac3'
    elif codec_family == 'ec-3':
        name = 'ec3'
    return name

#############################################
def SplitArgs(args):
    try:
        pairs = args.split('#')
        fields = {}
        for pair in pairs:
            name, value = pair.split(':', 1)
            fields[name] = value
        return fields
    except:
        raise Exception('invalid syntax for argument')

#############################################
def ComputeWidevineKeyLine(params):
    json_param = '{ "provider": "%(provider)s", "content_id": "%(content_id)s", "key_ids": ["%(kid)s"] }' % params
    key_line   = 'URI="data:text/plain;base64,'+json_param.encode('base64').replace('\n','')+'",KEYFORMAT="com.widevine",KEYFORMATVERSIONS="1"'

    return key_line

#############################################
def ComputeFairplayKeyLine(params):
    # start with a '!' to specify we want to skip the IV (since it is not needed on the key line for Fairplay)
    return '!URI="'+params['uri']+'",KEYFORMAT="com.apple.streamingkeydelivery",KEYFORMATVERSIONS="1"'

#############################################
def AnalyzeSources(options, media_sources):
    # parse the media files
    mp4_files = {}
    for media_source in media_sources:
        if media_source.format != 'mp4': continue

        media_file = media_source.filename

        # check if we have already parsed this file
        if media_file in mp4_files:
            media_source.mp4_file = mp4_files[media_file]
            continue

        # parse the file
        if not os.path.exists(media_file):
            PrintErrorAndExit('ERROR: media file ' + media_file + ' does not exist')

        # get the file info
        print 'Parsing media file', media_file
        mp4_file = Mp4File(Options, media_source)
        media_source.mp4_file = mp4_file

        # remember we have parsed this file
        mp4_files[media_file] = mp4_file

    # analyze the media sources
    for media_source in media_sources:
        track_id       = media_source.spec['track']
        track_type     = media_source.spec['type']
        track_language = media_source.spec['language']
        tracks         = []

        if media_source.format != 'mp4':
            if track_id or track_type:
                PrintErrorAndExit('ERROR: track ID and track type selections only apply to MP4 media sources')
            continue

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
            for track in media_source.mp4_file.tracks.values():
                language = LanguageCodeMap.get(track.language, track.language)
                if track_language and track_language != language and track_language != track.language:
                    continue
                tracks.append(track)

        # remember if this media source has a video or audio track
        for track in tracks:
            if track.type == 'video':
                media_source.has_video = True
            if track.type == 'audio':
                media_source.has_audio = True

        media_source.tracks = tracks

#############################################
def SelectAudioTracks(options, media_sources):
    # select tracks grouped by codec
    audio_tracks = {}
    for media_source in media_sources:

        # pre-process the track metadata
        for track in media_source.tracks:
            # track group
            track.group_id = ComputeCodecName(track.codec_family)

            # track language
            remap_language = media_source.spec.get('+language')
            if remap_language:
                track.language = remap_language
            language_name = LanguageNames.get(track.language, track.language)
            track.language_name = media_source.spec.get('+language_name', language_name).decode('utf-8')

        # process audio tracks
        for track in [t for t in media_source.tracks if t.type == 'audio']:
            group_id = track.group_id
            group = audio_tracks.get(group_id, [])
            audio_tracks[group_id] = group
            if len([x for x in group if x.language == track.language]):
                continue # only accept one track for each language per group
            group.append(track)

    return audio_tracks

#############################################
def ProcessSource(options, media_info, out_dir):
    if options.verbose:
        print 'Processing', media_info['source'].filename

    file_extension = media_info.get('file_extension', 'ts')

    kwargs = {
        'index_filename':            path.join(out_dir, options.media_playlist_name),
        'segment_filename_template': path.join(out_dir, 'segment-%d.'+file_extension),
        'segment_url_template':      'segment-%d.'+file_extension,
        'show_info':                 True
    }

    if options.base_url != "":
        kwargs["segment_url_template"] = options.base_url+media_info["dir"]+'/'+'segment-%d.'+file_extension

    if options.hls_version != 3:
        kwargs['hls_version'] = str(options.hls_version)

    if options.hls_version >= 4:
        kwargs['iframe_index_filename'] = path.join(out_dir, options.iframe_playlist_name)

    if options.output_single_file:
        kwargs['segment_filename_template'] = path.join(out_dir, 'media.'+file_extension)
        kwargs['segment_url_template']      = 'media.'+file_extension
        kwargs['output_single_file']        = True

    if 'audio_format' in media_info and media_info.get('audio_track_id') != 0:
        kwargs['audio_format'] = media_info['audio_format']

    for option in ['encryption_mode', 'encryption_key', 'encryption_iv_mode', 'encryption_key_uri', 'encryption_key_format', 'encryption_key_format_versions']:
        if getattr(options, option):
            kwargs[option] = getattr(options, option)

    key_lines = []

    # Fairplay
    if options.fairplay:
        key_lines.append(ComputeFairplayKeyLine(options.fairplay))

    # Widevine
    if options.widevine:
        key_lines.append(ComputeWidevineKeyLine(options.widevine))

    if len(key_lines):
        kwargs['encryption_key_line'] = key_lines

    # deal with track IDs
    if 'audio_track_id' in media_info:
        kwargs['audio_track_id'] = str(media_info['audio_track_id'])
    if 'video_track_id' in media_info:
        kwargs['video_track_id'] = str(media_info['video_track_id'])

    # other options
    if options.segment_duration:
        kwargs['segment_duration'] = options.segment_duration

    # convert to HLS/TS
    json_info = Mp42Hls(options,
                        media_info['source'].filename,
                        **kwargs)

    media_info['info'] = json.loads(json_info, strict=False)
    if options.verbose:
        print json_info

    # output the encryption key if needed
    if options.output_encryption_key:
        open(path.join(out_dir, 'key.bin'), 'wb+').write(options.encryption_key.decode('hex')[:16])

#############################################
def OutputHls(options, media_sources):
    mp4_sources = [media_source for media_source in media_sources if media_source.format == 'mp4']

    # analyze the media sources
    AnalyzeSources(options, media_sources)

    # select audio tracks
    audio_media = []
    audio_tracks = SelectAudioTracks(options, [media_source for media_source in mp4_sources if not media_source.spec.get('+audio_fallback')])

    # check if this is an audio-only presentation
    audio_only = True
    for media_source in mp4_sources:
        if media_source.has_video:
            audio_only = False
            break

    # check if the video has muxed audio
    video_has_muxed_audio = False
    for media_source in mp4_sources:
        if media_source.has_video and media_source.has_audio:
            video_has_muxed_audio = True
            break

    # audio-only presentations don't need alternate audio tracks
    if audio_only:
        audio_tracks = {}

    # we only need alternate audio tracks if there are more than one or if the audio and video are not muxed
    if video_has_muxed_audio and not audio_only and len(audio_tracks) == 1 and len(audio_tracks.values()[0]) == 1:
        audio_tracks = {}

    # process main media sources
    total_duration = 0
    main_media = []
    for media_source in mp4_sources:
        if not audio_only and not media_source.spec.get('+audio_fallback') and not media_source.has_video:
            continue
        media_index = 1+len(main_media)
        media_info = {
            'source':      media_source,
            'dir':         'media-'+str(media_index)
        }
        if audio_only:
            media_info['video_track_id'] = 0
            if options.audio_format == 'packed':
                source_audio_tracks = media_source.mp4_file.find_tracks_by_type('audio')
                if len(source_audio_tracks):
                    media_info['audio_format']   = options.audio_format
                    if options.audio_format == 'packed':
                        media_info['file_extension'] = ComputeCodecName(source_audio_tracks[0].codec_family)

        # no audio if there's a type filter for video
        if media_source.spec.get('type') == 'video':
            media_info['audio_track_id'] = 0

        # deal with audio-fallback streams
        if media_source.spec.get('+audio_fallback') == 'yes':
            media_info['video_track_id'] = 0

        # process the source
        out_dir = path.join(options.output_dir, media_info['dir'])
        MakeNewDir(out_dir)
        ProcessSource(options, media_info, out_dir)

        # update the duration
        duration_s = int(media_info['info']['stats']['duration'])
        if duration_s > total_duration:
            total_duration = duration_s

        main_media.append(media_info)

    # process audio tracks
    if len(audio_tracks):
        MakeNewDir(path.join(options.output_dir, 'audio'))
    for group_id in audio_tracks:
        group = audio_tracks[group_id]
        MakeNewDir(path.join(options.output_dir, 'audio', group_id))
        for audio_track in group:
            audio_track.media_info = {
                'source':         audio_track.parent.media_source,
                'audio_format':   options.audio_format,
                'dir':            'audio/'+group_id+'/'+audio_track.language,
                'language':       audio_track.language,
                'language_name':  audio_track.language_name,
                'audio_track_id': audio_track.id,
                'video_track_id': 0
            }
            if options.audio_format == 'packed':
                audio_track.media_info['file_extension'] = ComputeCodecName(audio_track.codec_family)

            # process the source
            out_dir = path.join(options.output_dir, 'audio', group_id, audio_track.language)
            MakeNewDir(out_dir)
            ProcessSource(options, audio_track.media_info, out_dir)

    # start the master playlist
    master_playlist = open(path.join(options.output_dir, options.master_playlist_name), "wb+")
    master_playlist.write("#EXTM3U\r\n")
    master_playlist.write('# Created with Bento4 mp4-hls.py version '+VERSION+'r'+SDK_REVISION+'\r\n')

    if options.hls_version >= 4:
        master_playlist.write('\r\n')
        master_playlist.write('#EXT-X-VERSION:'+str(options.hls_version)+'\r\n')

    # optional session key
    if options.signal_session_key:
        ext_x_session_key_line = '#EXT-X-SESSION-KEY:METHOD='+options.encryption_mode+',URI="'+options.encryption_key_uri+'"'
        if options.encryption_key_format:
            ext_x_session_key_line += ',KEYFORMAT="'+options.encryption_key_format+'"'
        if options.encryption_key_format_versions:
            ext_x_session_key_line += ',KEYFORMATVERSIONS="'+options.encryption_key_format_versions+'"'
        master_playlist.write(ext_x_session_key_line+'\r\n')

    # process subtitles sources
    subtitles_files = [SubtitlesFile(options, media_source) for media_source in media_sources if media_source.format in ['ttml', 'webvtt']]
    if len(subtitles_files):
        master_playlist.write('\r\n')
        master_playlist.write('# Subtitles\r\n')
        MakeNewDir(path.join(options.output_dir, 'subtitles'))
        for subtitles_file in subtitles_files:
            out_dir = path.join(options.output_dir, 'subtitles', subtitles_file.language)
            MakeNewDir(out_dir)
            media_filename = path.join(out_dir, subtitles_file.media_name)
            shutil.copyfile(subtitles_file.media_source.filename, media_filename)
            relative_url = 'subtitles/'+subtitles_file.language+'/subtitles.m3u8'
            playlist_filename = path.join(out_dir, 'subtitles.m3u8')
            CreateSubtitlesPlaylist(playlist_filename, subtitles_file.media_name, total_duration)

            master_playlist.write('#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="subtitles",NAME="%s",LANGUAGE="%s",URI="%s"\r\n' % (subtitles_file.language_name, subtitles_file.language, relative_url))

    # process audio sources
    audio_groups = []
    if len(audio_tracks):
        master_playlist.write('\r\n')
        master_playlist.write('# Audio\r\n')
        for group_id in audio_tracks:
            group = audio_tracks[group_id]
            group_name = 'audio_'+group_id
            group_codec = group[0].codec
            default = True
            group_avg_segment_bitrate = 0
            group_max_segment_bitrate = 0
            for audio_track in group:
                avg_segment_bitrate = int(audio_track.media_info['info']['stats']['avg_segment_bitrate'])
                max_segment_bitrate = int(audio_track.media_info['info']['stats']['max_segment_bitrate'])
                if avg_segment_bitrate > group_avg_segment_bitrate:
                    group_avg_segment_bitrate = avg_segment_bitrate
                if max_segment_bitrate > group_max_segment_bitrate:
                    group_max_segment_bitrate = max_segment_bitrate
                extra_info = 'AUTOSELECT=YES,'
                if default:
                    extra_info += 'DEFAULT=YES,'
                    default = False
                master_playlist.write(('#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="%s",NAME="%s",LANGUAGE="%s",%sURI="%s"\r\n' % (
                                      group_name,
                                      audio_track.media_info['language_name'],
                                      audio_track.media_info['language'],
                                      extra_info,
                                      options.base_url+audio_track.media_info['dir']+'/'+options.media_playlist_name)).encode('utf-8'))
            audio_groups.append({
                'name':                group_name,
                'codec':               group_codec,
                'avg_segment_bitrate': group_avg_segment_bitrate,
                'max_segment_bitrate': group_max_segment_bitrate
            })

        if options.debug:
            print 'Audio Groups:'
            print audio_groups

    else:
        audio_groups = [{
            'name':                None,
            'codec':               None,
            'avg_segment_bitrate': 0,
            'max_segment_bitrate': 0
        }]

    # media playlists
    master_playlist.write('\r\n')
    master_playlist.write('# Media Playlists\r\n')
    for media in main_media:
        media_info = media['info']

        for group_info in audio_groups:
            group_name  = group_info['name']
            group_codec = group_info['codec']

            # stream inf
            codecs = []
            if 'video' in media_info:
                codecs.append(media_info['video']['codec'])
            if 'audio' in media_info:
                codecs.append(media_info['audio']['codec'])
            elif group_name and group_codec:
                codecs.append(group_codec)

            ext_x_stream_inf = '#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=%d,BANDWIDTH=%d,CODECS="%s"' % (
                                int(media_info['stats']['avg_segment_bitrate'])+group_info['avg_segment_bitrate'],
                                int(media_info['stats']['max_segment_bitrate'])+group_info['max_segment_bitrate'],
                                ','.join(codecs))
            if 'video' in media_info:
                ext_x_stream_inf += ',RESOLUTION='+str(int(media_info['video']['width']))+'x'+str(int(media_info['video']['height']))

            # audio info
            if group_name:
                ext_x_stream_inf += ',AUDIO="'+group_name+'"'

            # subtitles info
            if len(subtitles_files):
                ext_x_stream_inf += ',SUBTITLES="subtitles"'

            master_playlist.write(ext_x_stream_inf+'\r\n')
            master_playlist.write(options.base_url+media['dir']+'/'+options.media_playlist_name+'\r\n')

    # write the I-FRAME playlist info
    if not audio_only and options.hls_version >= 4:
        master_playlist.write('\r\n')
        master_playlist.write('# I-Frame Playlists\r\n')
        for media in main_media:
            media_info = media['info']
            if not 'video' in media_info: continue
            ext_x_i_frame_stream_inf = '#EXT-X-I-FRAME-STREAM-INF:AVERAGE-BANDWIDTH=%d,BANDWIDTH=%d,CODECS="%s",RESOLUTION=%dx%d,URI="%s"' % (
                                        int(media_info['stats']['avg_iframe_bitrate']),
                                        int(media_info['stats']['max_iframe_bitrate']),
                                        media_info['video']['codec'],
                                        int(media_info['video']['width']),
                                        int(media_info['video']['height']),
                                        options.base_url+media['dir']+'/'+options.iframe_playlist_name)
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
    parser.add_option('', '--hls-version', dest="hls_version", type="int", metavar="<version>", default=4,
                      help="HLS Version (default: 4)")
    parser.add_option('', '--master-playlist-name', dest="master_playlist_name", metavar="<filename>", default='master.m3u8',
                      help="Master Playlist name")
    parser.add_option('', '--media-playlist-name', dest="media_playlist_name", metavar="<name>", default='stream.m3u8',
                      help="Media Playlist name")
    parser.add_option('', '--iframe-playlist-name', dest="iframe_playlist_name", metavar="<name>", default='iframes.m3u8',
                      help="I-frame Playlist name")
    parser.add_option('', '--output-single-file', dest="output_single_file", action='store_true', default=False,
                      help="Store segment data in a single output file per input file")
    parser.add_option('', '--audio-format', dest="audio_format", default='packed',
                      help="Format for audio segments (packed or ts) (default: packed)")
    parser.add_option('', '--segment-duration', dest="segment_duration",
                      help="Segment duration (default: 6)")
    parser.add_option('', '--encryption-mode', dest="encryption_mode", metavar="<mode>",
                      help="Encryption mode (only used when --encryption-key is specified). AES-128 or SAMPLE-AES (default: AES-128)")
    parser.add_option('', '--encryption-key', dest="encryption_key", metavar="<key>",
                      help="Encryption key in hexadecimal (default: no encryption)")
    parser.add_option('', '--encryption-iv-mode', dest="encryption_iv_mode", metavar="<mode>",
                      help="Encryption IV mode: 'sequence', 'random' or 'fps' (Fairplay Streaming) (default: sequence). When the mode is 'fps', the encryption key must be 32 bytes: 16 bytes for the key followed by 16 bytes for the IV.")
    parser.add_option('', '--encryption-key-uri', dest="encryption_key_uri", metavar="<uri>", default="key.bin",
                      help="Encryption key URI (may be a relative or absolute URI). (default: key.bin)")
    parser.add_option('', '--encryption-key-format', dest="encryption_key_format", metavar="<format>",
                      help="Encryption key format. (default: 'identity')")
    parser.add_option('', '--encryption-key-format-versions', dest="encryption_key_format_versions", metavar="<versions>",
                      help="Encryption key format versions.")
    parser.add_option('', '--signal-session-key', dest='signal_session_key', action='store_true', default=False,
                      help="Signal an #EXT-X-SESSION-KEY tag in the master playlist")
    parser.add_option('', '--fairplay', dest="fairplay", metavar="<fairplay-parameters>", help="Enable Fairplay Key Delivery. The <fairplay-parameters> argument is one or more <name>:<value> pair(s) (separated by '#' if more than one). Names include 'uri' [string] (required)")
    parser.add_option('', '--widevine', dest="widevine", metavar="<widevine-parameters>", help="Enable Widevine Key Delivery. The <widevine-parameters> argument is one or more <name>:<value> pair(s) (separated by '#' if more than one). Names include 'provider' [string] (required), 'content_id' [byte array in hex] (optional), 'kid' [16-byte array in hex] (required)")
    parser.add_option('', '--output-encryption-key', dest="output_encryption_key", action="store_true", default=False,
                      help="Output the encryption key to a file (default: don't output the key). This option is only valid when the encryption key format is 'identity'")
    parser.add_option('', "--exec-dir", metavar="<exec_dir>", dest="exec_dir", default=default_exec_dir,
                      help="Directory where the Bento4 executables are located")
    parser.add_option('', "--base-url", metavar="<base_url>", dest="base_url", default="",
                      help="The base URL for the Media Playlists and TS files listed in the playlists. This is the prefix for the files.")
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
        if options.encryption_key_uri != "key.bin":
            sys.stderr.write("WARNING: the encryption key will not be output because a non-default key URI was specified\n")
            options.output_encryption_key = False
        if not options.encryption_key:
            sys.stderr.write("ERROR: --output-encryption-key requires --encryption-key to be specified\n")
            sys.exit(1)
        if options.encryption_key_format != None and options.encryption_key_format != 'identity':
            sys.stderr.write("ERROR: --output-encryption-key requires --encryption-key-format to be omitted or set to 'identity'\n")
            sys.exit(1)

    # Fairplay option
    if options.fairplay:
        if not options.encryption_key_format:
            options.encryption_key_format = 'com.apple.streamingkeydelivery'
        if not options.encryption_key_format_versions:
            options.encryption_key_format_versions = '1'

        if options.encryption_iv_mode:
            if options.encryption_iv_mode != 'fps':
                sys.stderr.write("ERROR: --fairplay requires --encryption-iv-mode to be 'fps'\n")
                sys.exit(1)
        else:
            options.encryption_iv_mode = 'fps'
        if not options.encryption_key:
            sys.stderr.write("ERROR: --fairplay requires --encryption-key to be specified\n")
            sys.exit(1)
        if options.encryption_mode:
            if options.encryption_mode != 'SAMPLE-AES':
                sys.stderr.write('ERROR: --fairplay option incompatible with '+options.encryption_mode+' encryption mode\n')
                sys.exit(1)
        else:
            options.encryption_mode = 'SAMPLE-AES'
        options.fairplay = SplitArgs(options.fairplay)
        if 'uri' not in options.fairplay:
            sys.stderr.write('ERROR: --fairplay option requires a "uri" parameter (ex: skd://xxx)\n')
            sys.exit(1)

        options.signal_session_key = True

    # Widevine option
    if options.widevine:
        if not options.encryption_key:
            sys.stderr.write("ERROR: --widevine requires --encryption-key to be specified\n")
            sys.exit(1)
        if options.encryption_mode:
            if options.encryption_mode != 'SAMPLE-AES':
                sys.stderr.write('ERROR: --widevine option incompatible with '+options.encryption_mode+' encryption mode\n')
                sys.exit(1)
        else:
            options.encryption_mode = 'SAMPLE-AES'
        options.widevine = SplitArgs(options.widevine)
        if 'kid' not in options.widevine:
            sys.stderr.write('ERROR: --widevine option requires a "kid" parameter\n')
            sys.exit(1)
        if len(options.widevine['kid']) != 32:
            sys.stderr.write('ERROR: --widevine option "kid" must be 32 hex characters\n')
            sys.exit(1)
        if 'provider' not in options.widevine:
            sys.stderr.write('ERROR: --widevine option requires a "provider" parameter\n')
            sys.exit(1)
        if 'content_id' in options.widevine:
            options.widevine['content_id'] = options.widevine['content_id'].decode('hex')
        else:
            options.widevine['content_id'] = '*'

    # defaults
    if options.encryption_key and not options.encryption_mode:
        options.encryption_mode = 'AES-128'

    if options.encryption_mode == 'SAMPLE-AES':
        options.hls_version = 5

    # parse media sources syntax
    media_sources = [MediaSource(source) for source in args]
    for media_source in media_sources:
        media_source.has_audio  = False
        media_source.has_video  = False

    # create the output directory
    severity = 'ERROR'
    if options.force_output: severity = None
    MakeNewDir(dir=options.output_dir, exit_if_exists = not options.force_output, severity=severity)

    # output the media playlists
    OutputHls(options, media_sources)

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
