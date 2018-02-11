#!/usr/bin/env python

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2016 Axiomatic Systems, LLC.'

###
# NOTE: this script needs Bento4 command line binaries to run
# You must place the 'mp4info' 'mp4dump', 'mp4encrypt', 'mp4fragment', and 'mp4split' binaries
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
import math
from mp4utils import *
from subtitles import *

# setup main options
VERSION = "1.8.0"
SDK_REVISION = '622'
SCRIPT_PATH = path.abspath(path.dirname(__file__))
sys.path += [SCRIPT_PATH]

VIDEO_MIMETYPE              = 'video/mp4'
AUDIO_MIMETYPE              = 'audio/mp4'
SUBTITLES_MIMETYPE          = 'application/mp4'
VIDEO_DIR                   = 'video'
AUDIO_DIR                   = 'audio'
MPD_NS_COMPAT               = 'urn:mpeg:DASH:schema:MPD:2011'
MPD_NS                      = 'urn:mpeg:dash:schema:mpd:2011'
SPLIT_INIT_SEGMENT_NAME     = 'init.mp4'
NOSPLIT_INIT_FILE_PATTERN   = 'init-%s.mp4'
ONDEMAND_MEDIA_FILE_PATTERN = '%s-%s.mp4'

PADDED_SEGMENT_PATTERN      = 'seg-%04llu.m4s'
PADDED_SEGMENT_URL_PATTERN  = 'seg-%04d.m4s'
PADDED_SEGMENT_URL_TEMPLATE = '$RepresentationID$/seg-$Number%04d$.m4s'
NOPAD_SEGMENT_PATTERN       = 'seg-%llu.m4s'
NOPAD_SEGMENT_URL_PATTERN   = 'seg-%d.m4s'
NOPAD_SEGMENT_URL_TEMPLATE  = '$RepresentationID$/seg-$Number$.m4s'
SEGMENT_PATTERN             = NOPAD_SEGMENT_PATTERN
SEGMENT_URL_PATTERN         = NOPAD_SEGMENT_URL_PATTERN
SEGMENT_URL_TEMPLATE        = NOPAD_SEGMENT_URL_TEMPLATE

MEDIA_FILE_PATTERN          = '%s-%02d.mp4'

MARLIN_SCHEME_ID_URI        = 'urn:uuid:5E629AF5-38DA-4063-8977-97FFBD9902D4'
MARLIN_MAS_NAMESPACE        = 'urn:marlin:mas:1-0:services:schemas:mpd'
MARLIN_PSSH_SYSTEM_ID       = '69f908af481646ea910ccd5dcccb0a3a'

PLAYREADY_PSSH_SYSTEM_ID    = '9a04f07998404286ab92e65be0885f95'
PLAYREADY_SCHEME_ID_URI     = 'urn:uuid:9a04f079-9840-4286-ab92-e65be0885f95'
PLAYREADY_SCHEME_ID_URI_V10 = 'urn:uuid:79f0049a-4098-8642-ab92-e65be0885f95'
PLAYREADY_MSPR_NAMESPACE    = 'urn:microsoft:playready'

WIDEVINE_PSSH_SYSTEM_ID     = 'edef8ba979d64acea3c827dcd51d21ed'
WIDEVINE_SCHEME_ID_URI      = 'urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed'

PRIMETIME_PSSH_SYSTEM_ID    = 'f239e769efa348509c16a903c6932efb'
PRIMETIME_SCHEME_ID_URI     = 'urn:uuid:F239E769-EFA3-4850-9C16-A903C6932EFB'

MPEG_COMMON_ENCRYPTION_SCHEME_ID_URI = 'urn:mpeg:dash:mp4protection:2011'

EME_COMMON_ENCRYPTION_PSSH_SYSTEM_ID = '1077efecc0b24d02ace33c1e52e2fb4b'
EME_COMMON_ENCRYPTION_SCHEME_ID_URI  = 'urn:uuid:1077efec-c0b2-4d02-ace3-3c1e52e2fb4b'

SMOOTH_DEFAULT_TIMESCALE    = 10000000

SMIL_NAMESPACE              = 'http://www.w3.org/2001/SMIL20/Language'

CENC_2013_NAMESPACE         = 'urn:mpeg:cenc:2013'

DASH_DEFAULT_ROLE_NAMESPACE = 'urn:mpeg:dash:role:2011'

DASH_MEDIA_SEGMENT_URL_PATTERN_SMOOTH = "/QualityLevels($Bandwidth$)/Fragments(%s=$Time$)"
DASH_MEDIA_SEGMENT_URL_PATTERN_HIPPO  = '%s/Bitrate($Bandwidth$)/Fragment($Time$)'

HIPPO_MEDIA_SEGMENT_REGEXP_DEFAULT = '%s/Bitrate\\\\(%d\\\\)/Fragment\\\\((\\\\d+)\\\\)'
HIPPO_MEDIA_SEGMENT_GROUPS_DEFAULT = '["time"]'
HIPPO_MEDIA_SEGMENT_REGEXP_SMOOTH  = 'QualityLevels\\\\(%d\\\\)/Fragments\\\\(%s=(\\\\d+)\\\\)'
HIPPO_MEDIA_SEGMENT_GROUPS_SMOOTH  = '["time"]'

MPEG_DASH_AUDIO_CHANNEL_CONFIGURATION_SCHEME_ID_URI     = 'urn:mpeg:dash:23003:3:audio_channel_configuration:2011'
DOLBY_DIGITAL_AUDIO_CHANNEL_CONFIGURATION_SCHEME_ID_URI = 'tag:dolby.com,2014:dash:audio_channel_configuration:2011'

ISOFF_MAIN_PROFILE          = 'urn:mpeg:dash:profile:isoff-main:2011'
ISOFF_LIVE_PROFILE          = 'urn:mpeg:dash:profile:isoff-live:2011'
ISOFF_ON_DEMAND_PROFILE     = 'urn:mpeg:dash:profile:isoff-on-demand:2011'
HBBTV_15_ISOFF_LIVE_PROFILE = 'urn:hbbtv:dash:profile:isoff-live:2012'
ProfileAliases = {
  'main':      ISOFF_MAIN_PROFILE,
  'live':      ISOFF_LIVE_PROFILE,
  'on-demand': ISOFF_ON_DEMAND_PROFILE,
  'hbbtv-1.5': HBBTV_15_ISOFF_LIVE_PROFILE
}

TempFiles = []

MpegCencSchemeMap = {
    'cenc': 'MPEG-CENC',
    'cbc1': 'MPEG-CBC1',
    'cens': 'MPEG-CENS',
    'cbcs': 'MPEG-CBCS'
}

#############################################
def AddSegmentList(options, container, subdir, track, use_byte_range=False):
    if subdir:
        prefix = subdir+'/'
    else:
        prefix = ''
    segment_list = xml.SubElement(container,
                                  'SegmentList',
                                  timescale='1000',
                                  duration=str(int(round(track.average_segment_duration*1000))))
    if use_byte_range:
        byte_range = str(track.parent.init_segment.position)+'-'+str(track.parent.init_segment.position+track.parent.init_segment.size-1)
        xml.SubElement(segment_list,
                       'Initialization',
                       sourceURL=prefix + track.parent.media_name,
                       range=byte_range)
    else:
        xml.SubElement(segment_list,
                       'Initialization',
                       sourceURL=prefix + track.init_segment_name)
    i = 1
    for segment_index in track.moofs:
        segment = track.parent.segments[segment_index]
        segment_offset = segment[0].position
        segment_length = reduce(operator.add, [atom.size for atom in segment], 0)
        if use_byte_range:
            byte_range = str(segment_offset) + '-' + str(segment_offset + segment_length - 1)
            xml.SubElement(segment_list,
                           'SegmentURL',
                           media=prefix + track.parent.media_name,
                           mediaRange=byte_range)
        else:
            xml.SubElement(segment_list,
                           'SegmentURL',
                           media=prefix + (SEGMENT_URL_PATTERN % i))
        i += 1


#############################################
def AddSegmentTemplate(options, container, init_segment_url, media_url_template_prefix, track, stream_name):
    if options.use_segment_list:
        return

    if options.use_segment_timeline or track.type == 'subtitles':
        url_template = SEGMENT_URL_TEMPLATE
        use_template_numbers = True
        if options.smooth:
            url_base = path.basename(options.smooth_server_manifest_filename)
            url_template = url_base + DASH_MEDIA_SEGMENT_URL_PATTERN_SMOOTH % stream_name
            use_template_numbers = False
        elif options.hippo:
            url_template = DASH_MEDIA_SEGMENT_URL_PATTERN_HIPPO % stream_name
            use_template_numbers = False

        args = [container, 'SegmentTemplate']
        kwargs = {'timescale': str(track.timescale),
                  'initialization': init_segment_url,
                  'media': url_template,
                  'startNumber': '1'} # (keep the @startNumber, even if not needed, because some clients like Silverlight want it)

        segment_template = xml.SubElement(*args, **kwargs)
        segment_timeline = xml.SubElement(segment_template, 'SegmentTimeline')
        repeat_count = 0
        for i in range(len(track.segment_scaled_durations)):
            duration = track.segment_scaled_durations[i]
            if i + 1 < len(track.segment_scaled_durations) and duration == track.segment_scaled_durations[i + 1]:
                repeat_count += 1
            else:
                args = [segment_timeline, 'S']
                kwargs = {'d': str(duration)}
                if repeat_count:
                    kwargs['r'] = str(repeat_count)

                xml.SubElement(*args, **kwargs)
                repeat_count = 0
    else:
        xml.SubElement(container,
                       'SegmentTemplate',
                       timescale='1000',
                       duration=str(int(round(track.average_segment_duration*1000))),
                       initialization=init_segment_url,
                       media=SEGMENT_URL_TEMPLATE,
                       startNumber='1') # (keep the @startNumber, even if not needed, because some clients like Silverlight want it)

#############################################
def AddSegments(options, container, track):
    if options.use_segment_list:
        if options.split:
            subdir = track.representation_id
            use_byte_range = False
        else:
            subdir = None
            use_byte_range = True
        AddSegmentList(options, container, subdir, track, use_byte_range)

#############################################
def AddContentProtection(options, container, tracks):
    kids = []
    for track in tracks:
        kid = track.kid
        if kid is None and not options.no_media:
            PrintErrorAndExit('ERROR: no encryption info found in track '+str(track))
        if kid not in kids:
            kids.append(kid)

    # resolve the default KID and KEY
    key_info = None
    if len(tracks):
        key_info = tracks[0].key_info

    if key_info:
        default_kid = key_info['kid']
        default_key = key_info['key']
    else:
        default_kid = kids[0]
        default_key = None

    # EME Common Encryption
    if options.eme_signaling in ['pssh-v0', 'pssh-v1']:
        container.append(xml.Comment(' EME Common Encryption '))
        xml.register_namespace('cenc', CENC_2013_NAMESPACE)
        cp = xml.SubElement(container, 'ContentProtection', schemeIdUri=EME_COMMON_ENCRYPTION_SCHEME_ID_URI, value=options.encryption_cenc_scheme)
        if options.eme_signaling == 'pssh-v1':
            pssh_box = MakePsshBoxV1(EME_COMMON_ENCRYPTION_PSSH_SYSTEM_ID.decode('hex'), [default_kid], '')
        else:
            pssh_box = MakePsshBox(EME_COMMON_ENCRYPTION_PSSH_SYSTEM_ID.decode('hex'), '')
        pssh_b64 = pssh_box.encode('base64').replace('\n', '')
        pssh = xml.SubElement(cp, '{' + CENC_2013_NAMESPACE + '}pssh')
        pssh.text = pssh_b64

    # MPEG Common Encryption
    container.append(xml.Comment(' MPEG Common Encryption '))
    xml.register_namespace('cenc', CENC_2013_NAMESPACE)
    cp = xml.SubElement(container, 'ContentProtection', schemeIdUri=MPEG_COMMON_ENCRYPTION_SCHEME_ID_URI, value=options.encryption_cenc_scheme)
    default_kid_with_dashes = (default_kid[0:8]+'-'+default_kid[8:12]+'-'+default_kid[12:16]+'-'+default_kid[16:20]+'-'+default_kid[20:32]).lower()
    cp.set('{'+CENC_2013_NAMESPACE+'}default_KID', default_kid_with_dashes)

    # Marlin
    if options.marlin:
        container.append(xml.Comment(' Marlin '))
        xml.register_namespace('mas', MARLIN_MAS_NAMESPACE)
        cp = xml.SubElement(container, 'ContentProtection', schemeIdUri=MARLIN_SCHEME_ID_URI)
        cids = xml.SubElement(cp, '{' + MARLIN_MAS_NAMESPACE + '}MarlinContentIds')
        for kid in kids:
            cid = xml.SubElement(cids, '{' + MARLIN_MAS_NAMESPACE + '}MarlinContentId')
            cid.text = 'urn:marlin:kid:' + kid

    # PlayReady
    if options.playready:
        container.append(xml.Comment(' PlayReady '))
        xml.register_namespace('mspr', PLAYREADY_MSPR_NAMESPACE)
        if HBBTV_15_ISOFF_LIVE_PROFILE in options.profiles:
            playread_scheme_id_uri = PLAYREADY_SCHEME_ID_URI_V10
        else:
            playread_scheme_id_uri = PLAYREADY_SCHEME_ID_URI
        cp = xml.SubElement(container, 'ContentProtection', schemeIdUri=playread_scheme_id_uri)
        if options.playready_header:
            header_bin = ComputePlayReadyHeader(options.playready_header, default_kid, default_key)
            header_b64 = header_bin.encode('base64').replace('\n', '')
            pro = xml.SubElement(cp, '{' + PLAYREADY_MSPR_NAMESPACE + '}pro')
            pro.text = header_b64

    # Widevine
    if options.widevine:
        container.append(xml.Comment(' Widevine '))
        cp = xml.SubElement(container, 'ContentProtection', schemeIdUri=WIDEVINE_SCHEME_ID_URI)
        if options.widevine_header:
            pssh_payload = ComputeWidevineHeader(options.widevine_header, default_kid)
            pssh_box = MakePsshBox(WIDEVINE_PSSH_SYSTEM_ID.decode('hex'), pssh_payload)
            pssh_b64 = pssh_box.encode('base64').replace('\n', '')
            pssh = xml.SubElement(cp, '{' + CENC_2013_NAMESPACE + '}pssh')
            pssh.text = pssh_b64

    # Primetime
    if options.primetime:
        container.append(xml.Comment(' Primetime '))
        cp = xml.SubElement(container, 'ContentProtection', schemeIdUri=PRIMETIME_SCHEME_ID_URI)
        if options.primetime_metadata:
            pssh_payload = ComputePrimetimeMetaData(options.primetime_metadata, default_kid)
            pssh_box = MakePsshBox(PRIMETIME_PSSH_SYSTEM_ID.decode('hex'), pssh_payload)
            pssh_b64 = pssh_box.encode('base64').replace('\n', '')
            pssh = xml.SubElement(cp, '{' + CENC_2013_NAMESPACE + '}pssh')
            pssh.text = pssh_b64

#############################################
def AddDescriptor(adaptation_set, set_attributes, set_name, category_name):
    attributes = set_attributes.get(set_name)
    if not attributes and category_name:
        # try a catch-all category set name
        attributes = set_attributes.get(category_name)
        if attributes:
            set_name = category_name
    if not attributes: return

    for descriptor_name in attributes:
        descriptor_values = attributes[descriptor_name]
        for descriptor_value in descriptor_values.split(','):
            descriptor_namespace = None

            if descriptor_name.lower() == 'role':
                descriptor_namespace = DASH_DEFAULT_ROLE_NAMESPACE
            elif descriptor_name.startswith('{'):
                closeparen = descriptor_name.find('}')
                if closeparen >= 0:
                    descriptor_namespace = descriptor_name[1:closeparen]
                    descriptor_name = descriptor_name[closeparen+1:]

            # support using lowercase names instead of capitalized names
            if descriptor_name == 'accessibility': descriptor_name = 'Accessibility'
            if descriptor_name == 'role':          descriptor_name = 'Role'
            if descriptor_name == 'rating':        descriptor_name = 'Rating'
            if descriptor_name == 'viewpoint':     descriptor_name = 'Viewpoint'

            if descriptor_name not in ['Accessibility', 'Role', 'Rating', 'Viewpoint']:
                continue
            if descriptor_namespace:
                xml.SubElement(adaptation_set,
                               descriptor_name,
                               schemeIdUri=descriptor_namespace,
                               value=descriptor_value)
            else:
                sys.stderr.write('WARNING: ignoring ' + descriptor_name + ' descriptor for set "' + set_name + '", the schemeIdUri must be specified\n')

#############################################
def OutputDash(options, set_attributes, audio_sets, video_sets, subtitles_sets, subtitles_files):
    all_audio_tracks     = sum(audio_sets.values(),     [])
    all_video_tracks     = sum(video_sets.values(),     [])
    all_subtitles_tracks = sum(subtitles_sets.values(), [])

    # compute the total duration (we take the duration of the video)
    if len(all_video_tracks):
        presentation_duration = all_video_tracks[0].total_duration
    elif len(all_audio_tracks):
        presentation_duration = all_audio_tracks[0].total_duration
    elif len(all_subtitles_tracks):
        presentation_duration = all_subtitles_tracks[0].total_duration
    else:
        return

    # create the MPD
    if options.use_compat_namespace:
        mpd_ns = MPD_NS_COMPAT
    else:
        mpd_ns = MPD_NS
    mpd = xml.Element('MPD',
                      xmlns=mpd_ns,
                      profiles=','.join(options.profiles),
                      minBufferTime="PT%.02fS" % options.min_buffer_time,
                      mediaPresentationDuration=XmlDuration(presentation_duration),
                      type='static')
    mpd.append(xml.Comment(' Created with Bento4 mp4-dash.py, VERSION=' + VERSION + '-' + SDK_REVISION + ' '))
    period = xml.SubElement(mpd, 'Period')

    # process the video tracks
    if len(video_sets):
        period.append(xml.Comment(' Video '))

        for video_tracks in video_sets.values():
            # compute the min and max values
            minWidth  = 0
            minHeight = 0
            maxWidth  = 0
            maxHeight = 0
            for video_track in video_tracks:
                if minWidth  == 0 or video_track.width < minWidth:  minWidth  = video_track.width
                if minHeight == 0 or video_track.width < minHeight: minHeight = video_track.height
                if video_track.width  > maxWidth:  maxWidth  = video_track.width
                if video_track.height > maxHeight: maxHeight = video_track.height

            adaptation_set = xml.SubElement(period,
                                            'AdaptationSet',
                                            mimeType=VIDEO_MIMETYPE,
                                            segmentAlignment='true',
                                            startWithSAP='1',
                                            minWidth=str(minWidth),
                                            maxWidth=str(maxWidth),
                                            minHeight=str(minHeight),
                                            maxHeight=str(maxHeight))

            # see if we have descriptors
            AddDescriptor(adaptation_set, set_attributes, 'video', None)

            # setup content protection
            if options.encryption_key or options.marlin or options.playready or options.widevine:
                AddContentProtection(options, adaptation_set, video_tracks)

            if not options.on_demand:
                if options.split:
                    init_segment_url                  = '$RepresentationID$/' + SPLIT_INIT_SEGMENT_NAME
                    media_segment_url_template_prefix = '$RepresentationID$/'
                else:
                    init_segment_url                  = NOSPLIT_INIT_FILE_PATTERN % ('$RepresentationID$')
                    media_segment_url_template_prefix = ''
                AddSegmentTemplate(options, adaptation_set, init_segment_url, media_segment_url_template_prefix, video_tracks[0], 'video')

            for video_track in video_tracks:
                representation = xml.SubElement(adaptation_set,
                                                'Representation',
                                                id=video_track.representation_id,
                                                codecs=video_track.codec,
                                                width=str(video_track.width),
                                                height=str(video_track.height),
                                                scanType=video_track.scan_type,
                                                frameRate=video_track.frame_rate_ratio,
                                                bandwidth=str(video_track.bandwidth))
                if hasattr(video_track, 'max_playout_rate'):
                    representation.set('maxPlayoutRate', video_track.max_playout_rate)

                if options.on_demand:
                    base_url = xml.SubElement(representation, 'BaseURL')
                    base_url.text = ONDEMAND_MEDIA_FILE_PATTERN % (options.media_prefix, video_track.representation_id)
                    sidx_range = (video_track.sidx_atom.position, video_track.sidx_atom.position+video_track.sidx_atom.size-1)
                    init_range = (0, video_track.moov_atom.position+video_track.moov_atom.size-1)
                    segment_base = xml.SubElement(representation, 'SegmentBase', indexRange=str(sidx_range[0])+'-'+str(sidx_range[1]))
                    xml.SubElement(segment_base, 'Initialization', range=str(init_range[0])+'-'+str(init_range[1]))
                else:
                    AddSegments(options, representation, video_track)

    # process the audio tracks
    if len(audio_sets):
        period.append(xml.Comment(' Audio '))
        for adaptation_set_name, audio_tracks in audio_sets.items():
            args = [period, 'AdaptationSet']
            kwargs = {'mimeType': AUDIO_MIMETYPE, 'startWithSAP': '1', 'segmentAlignment': 'true'}
            language = audio_tracks[0].language
            if (language != 'und') or options.always_output_lang:
                kwargs['lang'] = language
            adaptation_set = xml.SubElement(*args, **kwargs)

            # see if we have descriptors
            AddDescriptor(adaptation_set, set_attributes, 'audio/' + language, 'audio')

            # setup content protection
            if options.encryption_key or options.marlin or options.playready or options.widevine:
                AddContentProtection(options, adaptation_set, audio_tracks)

            if not options.on_demand:
                if options.split:
                    init_segment_url                  = '$RepresentationID$/' + SPLIT_INIT_SEGMENT_NAME
                    media_segment_url_template_prefix = '$RepresentationID$/'
                else:
                    init_segment_url                  = NOSPLIT_INIT_FILE_PATTERN % ('$RepresentationID$')
                    media_segment_url_template_prefix = ''

                stream_name = 'audio_' + language
                AddSegmentTemplate(options, adaptation_set, init_segment_url, media_segment_url_template_prefix, audio_tracks[0], stream_name)

            for audio_track in audio_tracks:
                representation = xml.SubElement(adaptation_set,
                                                'Representation',
                                                id=audio_track.representation_id,
                                                codecs=audio_track.codec,
                                                bandwidth=str(audio_track.bandwidth),
                                                audioSamplingRate=str(audio_track.sample_rate))
                if audio_track.codec == 'ec-3':
                    audio_channel_config_value = ComputeDolbyDigitalAudioChannelConfig(audio_track)
                    scheme_id_uri = DOLBY_DIGITAL_AUDIO_CHANNEL_CONFIGURATION_SCHEME_ID_URI
                else:
                    audio_channel_config_value = str(audio_track.channels)
                    scheme_id_uri = MPEG_DASH_AUDIO_CHANNEL_CONFIGURATION_SCHEME_ID_URI
                audio_channel_config = xml.SubElement(representation,
                                                      'AudioChannelConfiguration',
                                                      schemeIdUri=scheme_id_uri,
                                                      value=audio_channel_config_value)

                if options.on_demand:
                    base_url = xml.SubElement(representation, 'BaseURL')
                    base_url.text = ONDEMAND_MEDIA_FILE_PATTERN % (options.media_prefix, audio_track.representation_id)
                    sidx_range = (audio_track.sidx_atom.position, audio_track.sidx_atom.position+audio_track.sidx_atom.size-1)
                    init_range = (0, audio_track.moov_atom.position+audio_track.moov_atom.size-1)
                    segment_base = xml.SubElement(representation, 'SegmentBase', indexRange=str(sidx_range[0])+'-'+str(sidx_range[1]))
                    xml.SubElement(segment_base, 'Initialization', range=str(init_range[0])+'-'+str(init_range[1]))
                else:
                    AddSegments(options, representation, audio_track)

    # process all the subtitles tracks
    if len(subtitles_sets):
        period.append(xml.Comment(' Subtitles (Encapsulated) '))
        for adaptation_set_name, subtitles_tracks in subtitles_sets.items():
            for subtitles_track in subtitles_tracks:
                args = [period, 'AdaptationSet']
                kwargs = {'mimeType': SUBTITLES_MIMETYPE, 'startWithSAP': '1'}
                if (subtitles_track.language != 'und') or options.always_output_lang:
                    kwargs['lang'] = subtitles_track.language
                adaptation_set = xml.SubElement(*args, **kwargs)

                # add a 'subtitles' role
                xml.SubElement(adaptation_set, 'Role', schemeIdUri='urn:mpeg:dash:role:2011', value='subtitle')

                # see if we have other descriptors
                AddDescriptor(adaptation_set, set_attributes, 'subtitles/' + subtitles_track.language, 'subtitles')

                representation = xml.SubElement(adaptation_set,
                                                'Representation',
                                                id=subtitles_track.representation_id,
                                                codecs=subtitles_track.codec,
                                                bandwidth=str(subtitles_track.bandwidth))

                if options.on_demand:
                    base_url = xml.SubElement(representation, 'BaseURL')
                    base_url.text = ONDEMAND_MEDIA_FILE_PATTERN % (options.media_prefix, subtitles_track.representation_id)
                    sidx_range = (subtitles_track.sidx_atom.position, subtitles_track.sidx_atom.position+subtitles_track.sidx_atom.size-1)
                    init_range = (0, subtitles_track.moov_atom.position+subtitles_track.moov_atom.size-1)
                    segment_base = xml.SubElement(representation, 'SegmentBase', indexRange=str(sidx_range[0])+'-'+str(sidx_range[1]))
                    xml.SubElement(segment_base, 'Initialization', range=str(init_range[0])+'-'+str(init_range[1]))
                else:
                    if options.split:
                        init_segment_url                  = '$RepresentationID$/' + SPLIT_INIT_SEGMENT_NAME
                        media_segment_url_template_prefix = '$RepresentationID$/'
                    else:
                        init_segment_url                  = NOSPLIT_INIT_FILE_PATTERN % ('$RepresentationID$')
                        media_segment_url_template_prefix = ''

                    stream_name = 'subtitles_' + subtitles_track.language
                    AddSegmentTemplate(options, adaptation_set, init_segment_url, media_segment_url_template_prefix, subtitles_track, stream_name)
                    AddSegments(options, representation, subtitles_track)

    # process all the subtitles files
    if len(subtitles_files):
        period.append(xml.Comment(' Subtitles (Sidecar) '))
        for subtitles_file in subtitles_files:
            args = [period, 'AdaptationSet']
            kwargs = {
                'mimeType':    subtitles_file.mime_type,
                'contentType': 'text'
            }
            if (subtitles_file.language != None):
                kwargs['lang'] = subtitles_file.language
            adaptation_set = xml.SubElement(*args, **kwargs)

            # add a 'subtitles' role
            xml.SubElement(adaptation_set, 'Role', schemeIdUri='urn:mpeg:dash:role:2011', value='subtitle')

            # see if we have other descriptors
            AddDescriptor(adaptation_set, set_attributes, 'subtitles/' + subtitles_file.language, 'subtitles')

            # estimate the bandwidth
            bandwidth = 1024 # default
            if presentation_duration and subtitles_file.size:
                bandwidth = int((8*subtitles_file.size)/presentation_duration)

            representation = xml.SubElement(adaptation_set,
                                            'Representation',
                                            id='subtitles/'+subtitles_file.language,
                                            bandwidth=str(bandwidth))
            base_url = xml.SubElement(representation, 'BaseURL')
            base_url.text = 'subtitles/'+subtitles_file.language+'/'+subtitles_file.media_name

    # save the MPD
    if options.mpd_filename:
        mpd_xml = parseString(xml.tostring(mpd)).toprettyxml("  ")
        # use a regex to fix a bug in toprettyxml() that inserts newlines in text content
        mpd_xml = re.sub(r'((?<=>)(\n[\s]*)(?=[^<\s]))|(?<=[^>\s])(\n[\s]*)(?=<)', '', mpd_xml)
        open(path.join(options.output_dir, options.mpd_filename), "wb").write(mpd_xml)


#############################################
def ComputeHlsWidevineKeyLine(options, track):
    try:
        pairs = options.widevine_header.split('#')
        fields = {}
        for pair in pairs:
            name, value = pair.split(':', 1)
            fields[name] = value
    except:
        raise Exception('invalid syntax for --widevine-header option')
    
    if 'content_id' not in fields:
        fields['content_id'] = '*'
    if 'kid' not in fields:
        fields['kid'] = track.key_info['kid']

    json_param = '{ "provider": "%(provider)s", "content_id": "%(content_id)s", "key_ids": ["%(kid)s"] }' % fields
    key_line   = 'URI="data:text/plain;base64,'+json_param.encode('base64').replace('\n','')+'",KEYFORMAT="com.widevine",KEYFORMATVERSIONS="1",IV=0x'+track.key_info['iv']

    return key_line

#############################################
def ComputeHlsFairplayKeyLine(options):
    return 'URI="'+options.fairplay_key_uri+'",KEYFORMAT="com.apple.streamingkeydelivery",KEYFORMATVERSIONS="1"'

#############################################
def OutputHlsCommon(options, track, media_subdir, playlist_name, media_file_name):
    hls_target_duration = math.ceil(max(track.segment_durations))

    playlist_file = open(path.join(options.output_dir, media_subdir, playlist_name), 'wb+')
    playlist_file.write('#EXTM3U\r\n')
    playlist_file.write('# Created with Bento4 mp4-dash.py, VERSION=' + VERSION + '-' + SDK_REVISION+'\r\n')
    playlist_file.write('#\r\n')
    playlist_file.write('#EXT-X-VERSION:6\r\n')
    playlist_file.write('#EXT-X-PLAYLIST-TYPE:VOD\r\n')
    playlist_file.write('#EXT-X-INDEPENDENT-SEGMENTS\r\n')
    playlist_file.write('#EXT-X-TARGETDURATION:%d\r\n' % (hls_target_duration))
    playlist_file.write('#EXT-X-MEDIA-SEQUENCE:0\r\n')
    if options.split:
        playlist_file.write('#EXT-X-MAP:URI="%s"\r\n' % (SPLIT_INIT_SEGMENT_NAME))
    else:
        init_segment_size = track.parent.init_segment.position + track.parent.init_segment.size
        playlist_file.write('#EXT-X-MAP:URI="%s",BYTERANGE="%d@0"\r\n' % (media_file_name, init_segment_size))

    if options.encryption_key:
        key_lines = []
        if options.fairplay_key_uri:
            key_lines.append(ComputeHlsFairplayKeyLine(options))
        if options.widevine_header:
            key_lines.append(ComputeHlsWidevineKeyLine(options, track))

        if len(key_lines) == 0:
            key_lines.append('URI="'+options.hls_key_url+'",IV=0x'+track.key_info['iv'])
        
        for key_line in key_lines:
            playlist_file.write('#EXT-X-KEY:METHOD=SAMPLE-AES,'+key_line+'\r\n')

    return playlist_file

#############################################
def OutputHlsTrack(options, track, media_subdir, media_playlist_name, media_file_name):
    media_playlist_file = OutputHlsCommon(options, track, media_subdir, media_playlist_name, media_file_name)

    if options.split:
        segment_pattern = SEGMENT_PATTERN.replace('ll','')

    for i in range(len(track.segment_durations)):
        media_playlist_file.write('#EXTINF:%f,\r\n' % (track.segment_durations[i]))
        if options.on_demand or not options.split:
            segment          = track.parent.segments[track.moofs[i]]
            segment_position = segment[0].position
            segment_size     = reduce(operator.add, [atom.size for atom in segment], 0)
            media_playlist_file.write('#EXT-X-BYTERANGE:%d@%d\r\n' % (segment_size, segment_position))
            media_playlist_file.write(media_file_name)
        else:
            media_playlist_file.write(segment_pattern % (i+1))
        media_playlist_file.write('\r\n')

    media_playlist_file.write('#EXT-X-ENDLIST\r\n')

#############################################
def OutputHlsIframeIndex(options, track, media_subdir, iframes_playlist_name, media_file_name):
    index_playlist_file = OutputHlsCommon(options, track, media_subdir, iframes_playlist_name, media_file_name)

    index_playlist_file.write('#EXT-X-I-FRAMES-ONLY\r\n')

    if not options.split:
        # get the I-frame index for a single file
        json_index = Mp4IframIndex(options, path.join(options.output_dir, media_file_name))
        index = json.loads(json_index)
        for i in range(len(track.segment_durations)):
            if i < len(index):
                index_entry = index[i]
                index_playlist_file.write('#EXTINF:%f,\r\n' % (track.segment_durations[i]))
                fragment_start    = int(index_entry['fragmentStart'])
                iframe_offset     = int(index_entry['offset'])
                iframe_size       = int(index_entry['size'])
                iframe_range_size = iframe_size + (iframe_offset-fragment_start)
                index_playlist_file.write('#EXT-X-BYTERANGE:%d@%d\r\n' % (iframe_range_size, fragment_start))
                index_playlist_file.write(media_file_name+'\r\n')
    else:
        segment_pattern = SEGMENT_PATTERN.replace('ll','')
        for i in range(len(track.segment_durations)):
            fragment_basename = segment_pattern % (i+1)
            fragment_file = path.join(options.output_dir, media_subdir, fragment_basename)
            init_file = path.join(options.output_dir, media_subdir, options.init_segment)
            if not path.exists(fragment_file):
                break
            json_index = Mp4IframIndex(options, fragment_file, fragments_info=init_file)
            index = json.loads(json_index)
            if len(index) < 1:
                break
            iframe_size       = int(index[0]['size'])
            iframe_offset     = int(index[0]['offset'])
            iframe_range_size = iframe_size + iframe_offset
            index_playlist_file.write('#EXTINF:%f,\r\n' % (track.segment_durations[i]))
            index_playlist_file.write('#EXT-X-BYTERANGE:%d@0\r\n' % (iframe_range_size))
            index_playlist_file.write(fragment_basename+'\r\n')
        
    index_playlist_file.write('#EXT-X-ENDLIST\r\n')

#############################################
def OutputHls(options, set_attributes, audio_sets, video_sets, subtitles_sets, subtitles_files):
    all_audio_tracks     = sum(audio_sets.values(),     [])
    all_video_tracks     = sum(video_sets.values(),     [])
    all_subtitles_tracks = sum(subtitles_sets.values(), [])

    master_playlist_file = open(path.join(options.output_dir, options.hls_master_playlist_name), 'wb+')
    master_playlist_file.write('#EXTM3U\r\n')
    master_playlist_file.write('# Created with Bento4 mp4-dash.py, VERSION=' + VERSION + '-' + SDK_REVISION+'\r\n')
    master_playlist_file.write('#\r\n')
    master_playlist_file.write('#EXT-X-VERSION:6\r\n')
    master_playlist_file.write('\r\n')
    master_playlist_file.write('# Media Playlists\r\n')

    master_playlist_file.write('\r\n')
    master_playlist_file.write('# Audio\r\n')
    audio_groups = {}
    for adaptation_set_name, audio_tracks in audio_sets.items():
        language = audio_tracks[0].language.decode('utf-8')
        language_name = LanguageNames.get(language, language).decode('utf-8')

        audio_group_name = adaptation_set_name[0]+'/'+adaptation_set_name[2]
        audio_groups[audio_group_name] = {
            'codec': '',
            'average_segment_bitrate': 0,
            'max_segment_bitrate': 0
        }
        for audio_track in audio_tracks:
            # update the avergage and max bitrates
            if audio_track.average_segment_bitrate > audio_groups[audio_group_name]['average_segment_bitrate']:
                audio_groups[audio_group_name]['average_segment_bitrate'] = audio_track.average_segment_bitrate
            if audio_track.max_segment_bitrate > audio_groups[audio_group_name]['max_segment_bitrate']:
                audio_groups[audio_group_name]['max_segment_bitrate'] = audio_track.max_segment_bitrate

            # update/check the codec
            if audio_groups[audio_group_name]['codec'] == '':
                audio_groups[audio_group_name]['codec'] = audio_track.codec
            else:
                if audio_groups[audio_group_name]['codec'] != audio_track.codec:
                    print 'WARNING: audio codecs not all the same:', audio_groups[audio_group_name]['codec'], audio_track.codec

            if options.on_demand or not options.split:
                media_subdir        = ''
                media_file_name     = audio_track.parent.media_name
                media_playlist_name = audio_track.representation_id+".m3u8"
                media_playlist_path = media_playlist_name
            else:
                media_subdir        = audio_track.representation_id
                media_file_name     = ''
                media_playlist_name = options.hls_media_playlist_name
                media_playlist_path = media_subdir+'/'+media_playlist_name

            master_playlist_file.write(('#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID="%s",LANGUAGE="%s",NAME="%s",AUTOSELECT=YES,DEFAULT=YES,URI="%s"\r\n' % (
                                        audio_group_name,
                                        language,
                                        language_name,
                                        media_playlist_path)).encode('utf-8'))
            OutputHlsTrack(options, audio_track, media_subdir, media_playlist_name, media_file_name)

    master_playlist_file.write('\r\n')
    master_playlist_file.write('# Video\r\n')
    iframe_playlist_lines = []
    for video_track in all_video_tracks:
        if options.on_demand or not options.split:
            media_subdir          = ''
            media_file_name       = video_track.parent.media_name
            media_playlist_name   = video_track.representation_id+".m3u8"
            media_playlist_path   = media_playlist_name
            iframes_playlist_name = video_track.representation_id+"_iframes.m3u8"
        else:
            media_subdir          = video_track.representation_id
            media_file_name       = ''
            media_playlist_name   = options.hls_media_playlist_name
            media_playlist_path   = media_subdir+'/'+media_playlist_name
            iframes_playlist_name = options.hls_iframes_playlist_name

        if len(audio_groups):
            # one entry per audio group
            for audio_group_name in audio_groups:
                audio_codec = audio_groups[audio_group_name]['codec']
                master_playlist_file.write('#EXT-X-STREAM-INF:AUDIO="%s",AVERAGE-BANDWIDTH=%d,BANDWIDTH=%d,CODECS="%s",RESOLUTION=%dx%d\r\n' % (
                                           audio_group_name,
                                           video_track.average_segment_bitrate + audio_groups[audio_group_name]['average_segment_bitrate'],
                                           video_track.max_segment_bitrate + audio_groups[audio_group_name]['max_segment_bitrate'],
                                           video_track.codec+','+audio_codec,
                                           video_track.width,
                                           video_track.height))
                master_playlist_file.write(media_playlist_path+'\r\n')
        else:
            # no audio
            master_playlist_file.write('#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=%d,BANDWIDTH=%d,CODECS="%s",RESOLUTION=%dx%d\r\n' % (
                                       video_track.average_segment_bitrate,
                                       video_track.max_segment_bitrate,
                                       video_track.codec,
                                       video_track.width,
                                       video_track.height))
            master_playlist_file.write(media_playlist_path+'\r\n')

        OutputHlsTrack(options, video_track, media_subdir, media_playlist_name, media_file_name)
        OutputHlsIframeIndex(options, video_track, media_subdir, iframes_playlist_name, media_file_name)

        # this will be written later
        iframe_playlist_lines.append('#EXT-X-I-FRAME-STREAM-INF:AVERAGE-BANDWIDTH=%d,BANDWIDTH=%d,CODECS="%s",RESOLUTION=%dx%d,URI="%s"\r\n' % (
                                     video_track.average_segment_bitrate,
                                     video_track.max_segment_bitrate,
                                     video_track.codec,
                                     video_track.width,
                                     video_track.height,
                                     media_playlist_path))

    master_playlist_file.write('\r\n# I-Frame Playlists\r\n')
    master_playlist_file.write(''.join(iframe_playlist_lines))

    # IMSC1 subtitles
    if len(all_subtitles_tracks):
        master_playlist_file.write('\r\n# Subtitles (IMSC1)\r\n')
        for subtitles_track in all_subtitles_tracks:
            if subtitles_track.codec != 'stpp':
                # only accept IMSC1 tracks
                continue
            language = subtitles_track.language.decode('utf-8')
            language_name = LanguageNames.get(language, language).decode('utf-8')
            
            if options.on_demand or not options.split:
                media_subdir        = ''
                media_file_name     = subtitles_track.parent.media_name
                media_playlist_name = subtitles_track.representation_id+".m3u8"
                media_playlist_path = media_playlist_name
            else:
                media_subdir        = subtitles_track.representation_id
                media_file_name     = ''
                media_playlist_name = options.hls_media_playlist_name
                media_playlist_path = media_subdir+'/'+media_playlist_name

            master_playlist_file.write('#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="imsc1",NAME="{0:s}",DEFAULT=NO,AUTOSELECT=YES,LANGUAGE="{1:s}",URI="{2:s}"\r\n'
                                       .format(language_name, language, media_playlist_path))
            
    # WebVTT subtitles
    if len(subtitles_files):
        master_playlist_file.write('\r\n# Subtitles (WebVTT)\r\n')
        for subtitles_file in subtitles_files:
            master_playlist_file.write('#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID="subs",NAME="{0:s}",DEFAULT=NO,AUTOSELECT=YES,FORCED=YES,LANGUAGE="{0:s}",URI="subtitles/{0:s}/{1:s}"\r\n'
                                       .format(subtitles_file.language,subtitles_file.media_name))

#############################################
def OutputSmooth(options, audio_tracks, video_tracks):
    # compute the total duration (we take the duration of the video)
    if len(video_tracks):
        presentation_duration = video_tracks[0].total_duration
    else:
        presentation_duration = audio_tracks[0].total_duration

    # create the Client Manifest
    client_manifest = xml.Element('SmoothStreamingMedia',
                                  MajorVersion="2",
                                  MinorVersion="0",
                                  TimeScale="10000000",
                                  Duration=str(int(round(presentation_duration*10000000.0))))
    client_manifest.append(xml.Comment(' Created with Bento4 mp4-dash.py, VERSION='+VERSION+'-'+SDK_REVISION+' '))

    # process the audio tracks
    for audio_track in audio_tracks:
        stream_name = "audio_"+audio_track.language
        audio_url_pattern="QualityLevels({bitrate})/Fragments(%s={start time})" % (stream_name)
        stream_index = xml.SubElement(client_manifest,
                                      'StreamIndex',
                                      Chunks=str(len(audio_track.moofs)),
                                      Url=audio_url_pattern,
                                      Type="audio",
                                      Name=stream_name,
                                      QualityLevels="1",
                                      TimeScale=str(audio_track.timescale))
        if audio_track.language != 'und' or options.always_output_lang:
            stream_index.set('Language', audio_track.language)

        if audio_track.codec == 'ec-3':
            # Dolby Digital Plus
            (channels, codec_private_data) = ComputeDolbyDigitalSmoothStreamingInfo(audio_track)
            audio_tag = '65534'
            fourcc = 'EC-3'
            channels = str(channels)
            data_rate = int(audio_track.info['sample_descriptions'][0]['dolby_digital_info']['data_rate'])
            packet_size = str(4*data_rate)
        else:
            # assume AAC
            audio_tag = '255'
            fourcc = 'AACL'
            codec_private_data=audio_track.info['sample_descriptions'][0]['decoder_info']
            channels = str(audio_track.channels)
            packet_size = str(2*audio_track.channels)

        xml.SubElement(stream_index,
                       'QualityLevel',
                       Bitrate=str(audio_track.bandwidth),
                       SamplingRate=str(audio_track.sample_rate),
                       Channels=channels,
                       BitsPerSample="16",
                       PacketSize=packet_size,
                       AudioTag=audio_tag,
                       FourCC=fourcc,
                       Index="0",
                       CodecPrivateData=codec_private_data)

        for duration in audio_track.segment_scaled_durations:
            xml.SubElement(stream_index, "c", d=str(duration))

    # process all the video tracks
    if len(video_tracks):
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
            xml.SubElement(stream_index,
                           'QualityLevel',
                           Bitrate=str(video_track.bandwidth),
                           MaxWidth=str(video_track.width),
                           MaxHeight=str(video_track.height),
                           FourCC=options.smooth_h264_fourcc,
                           CodecPrivateData=codec_private_data,
                           Index=str(qindex))
            qindex += 1

        for duration in video_tracks[0].segment_scaled_durations:
            xml.SubElement(stream_index, "c", d=str(duration))

    if options.playready_header:
        key_info = None
        if options.encryption_key:
            if len(video_tracks):
                key_info = video_tracks[0].key_info
                if not key_info and len(audio_tracks):
                    key_info = audio_tracks[0].key_info

        if key_info:
            kid = key_info['kid']
            key = key_info['key']
        else:
            if len(video_tracks):
                kid = video_tracks[0].kid
            else:
                kid = audio_tracks[0].kid
            key = None
        header_bin = ComputePlayReadyHeader(options.playready_header, kid, key)
        header_b64 = header_bin.encode('base64').replace('\n', '')
        protection = xml.SubElement(client_manifest, 'Protection')
        protection_header = xml.SubElement(protection,
                                           'ProtectionHeader',
                                           SystemID='9a04f079-9840-4286-ab92-e65be0885f95')
        protection_header.text = header_b64

    # save the Smooth Client Manifest
    if options.smooth_client_manifest_filename != '':
        open(path.join(options.output_dir, options.smooth_client_manifest_filename), "wb").write(parseString(xml.tostring(client_manifest)).toprettyxml("  "))

    # create the Server Manifest file
    server_manifest = xml.Element('smil', xmlns=SMIL_NAMESPACE)
    server_manifest_head = xml.SubElement(server_manifest, 'head')
    xml.SubElement(server_manifest_head,
                   'meta',
                   name='clientManifestRelativePath',
                   content=path.basename(options.smooth_client_manifest_filename))
    server_manifest_body = xml.SubElement(server_manifest, 'body')
    server_manifest_switch = xml.SubElement(server_manifest_body, 'switch')
    for audio_track in audio_tracks:
        audio_entry = xml.SubElement(server_manifest_switch,
                                     'audio',
                                     src=audio_track.parent.media_name,
                                     systemBitrate=str(audio_track.bandwidth))
        xml.SubElement(audio_entry,
                       'param',
                       name='trackID',
                       value=str(audio_track.id),
                       valueType='data')
        if audio_track.language:
            xml.SubElement(audio_entry,
                           'param',
                           name='trackName',
                           value="audio_" + audio_track.language,
                           valueType='data')
        if audio_track.timescale != SMOOTH_DEFAULT_TIMESCALE:
            xml.SubElement(audio_entry,
                           'param',
                           name='timeScale',
                           value=str(audio_track.timescale),
                           valueType='data')

    for video_track in video_tracks:
        video_entry = xml.SubElement(server_manifest_switch,
                                     'video',
                                     src=video_track.parent.media_name,
                                     systemBitrate=str(video_track.bandwidth))
        xml.SubElement(video_entry,
                       'param',
                       name='trackID',
                       value=str(video_track.id),
                       valueType='data')
        if video_track.timescale != SMOOTH_DEFAULT_TIMESCALE:
            xml.SubElement(video_entry,
                           'param',
                           name='timeScale',
                           value=str(video_track.timescale),
                           valueType='data')

    # save the Manifest
    if options.smooth_server_manifest_filename != '':
        open(path.join(options.output_dir, options.smooth_server_manifest_filename), "wb").write(parseString(xml.tostring(server_manifest)).toprettyxml("  "))

#############################################
def OutputHippo(options, audio_tracks, video_tracks):
    # create the Server Manifest file
    server_manifest = '{\n  "media": [\n'
    sep = ''
    for track in audio_tracks+video_tracks:
        server_manifest += sep+'    {\n'
        server_manifest += '      "trackId": '+ str(track.id) + ',\n'
        server_manifest += '      "mediaSegments": {\n'
        server_manifest += '        "urls": [\n'
        server_manifest += '          {\n'
        if options.smooth:
            server_manifest += '            "pattern": "'+(HIPPO_MEDIA_SEGMENT_REGEXP_SMOOTH % (track.bandwidth, track.stream_id))+'",\n'
            server_manifest += '            "fields": '+HIPPO_MEDIA_SEGMENT_GROUPS_SMOOTH+'\n'
        else:
            server_manifest += '            "pattern": "'+(HIPPO_MEDIA_SEGMENT_REGEXP_DEFAULT % (track.stream_id, track.bandwidth))+'",\n'
            server_manifest += '            "fields": '+HIPPO_MEDIA_SEGMENT_GROUPS_DEFAULT+'\n'
        server_manifest += '          }\n'
        server_manifest += '        ],\n'
        server_manifest += '        "file": "' + track.parent.media_name + '"\n'
        server_manifest += '      },\n'
        server_manifest += '      "initSegment": {\n'
        server_manifest += '        "file": "' + track.init_segment_name + '"\n'
        server_manifest += '      }\n'
        server_manifest += '    }'
        sep = ',\n'
    server_manifest += '\n  ]\n}'

    # save the Manifest
    if options.hippo_server_manifest_filename != '':
        open(path.join(options.output_dir, options.hippo_server_manifest_filename), "wb").write(server_manifest)

#############################################
def SelectTracks(options, media_sources):
    # parse the media files
    file_list_index = 1
    mp4_files = {}
    mp4_media_names = []
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
        print 'Parsing media file', str(file_list_index)+':', GetMappedFileName(media_file)
        mp4_file = Mp4File(Options, media_source)

        # set some metadata properties for this file
        mp4_file.file_list_index = file_list_index
        file_list_index += 1
        if options.rename_media:
            mp4_file.media_name = MEDIA_FILE_PATTERN % (options.media_prefix, mp4_file.file_list_index)
        elif '+media' in media_source.spec:
            mp4_file.media_name = media_source.spec['+media']
        elif 'media' in media_source.spec: # we still accept the form without the '+' sign, for legacy support
            mp4_file.media_name = media_source.spec['media']
        else:
            mp4_file.media_name = path.basename(media_source.original_filename)

        if not options.split:
            if mp4_file.media_name in mp4_media_names:
                PrintErrorAndExit('ERROR: output media name %s is not unique, consider using --rename-media'%mp4_file.media_name)

        # check the file
        if mp4_file.info['movie']['fragments'] != True:
            PrintErrorAndExit('ERROR: file '+str(mp4_file.file_list_index)+' is not fragmented (use mp4fragment to fragment it)')

        # set the source property
        media_source.mp4_file = mp4_file
        mp4_files[media_file] = mp4_file
        mp4_media_names.append(mp4_file.media_name)


    # select tracks
    audio_adaptation_sets     = {}
    video_adaptation_sets     = {}
    subtitles_adaptation_sets = {}
    for media_source in media_sources:
        track_id       = media_source.spec['track']
        track_type     = media_source.spec['type']
        track_language = media_source.spec['language']
        tracks         = []

        if media_source.format != 'mp4':
            if track_id or track_type:
                PrintErrorAndExit('ERROR: track ID and track type selections only apply to MP4 media sources')

            continue

        if track_type not in ['', 'audio', 'video', 'subtitles']:
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

        # pre-process the track metadata
        for track in tracks:
            # track language
            language = LanguageCodeMap.get(track.language, track.language)
            if track_language and track_language != language and track_language != track.language:
                continue
            remap_language = media_source.spec.get('+language')
            if remap_language:
                language = remap_language
            elif options.language_map and language in options.language_map:
                language = options.language_map[language]
            track.language = language

            # video scan type
            if track.type == 'video':
                track.scan_type = media_source.spec.get('+scan_type', track.scan_type)

        # process audio tracks
        for track in [t for t in tracks if t.type == 'audio']:
            adaptation_set_name = ('audio', track.language, track.codec_family)
            adaptation_set = audio_adaptation_sets.get(adaptation_set_name, [])
            audio_adaptation_sets[adaptation_set_name] = adaptation_set

            # only keep this track if there isn't already a track with the same codec at the same bitrate (within 10%)
            with_same_bandwidth = [t for t in adaptation_set if abs(float(t.bandwidth-track.bandwidth)/float(t.bandwidth)) < 0.1]
            if len(with_same_bandwidth):
                continue

            adaptation_set.append(track)
            track.order_index = len(adaptation_set)

        # process video tracks
        for track in [t for t in tracks if t.type == 'video']:
            adaptation_set_name = ('video', track.codec_family)
            adaptation_set = video_adaptation_sets.get(adaptation_set_name, [])
            video_adaptation_sets[adaptation_set_name] = adaptation_set
            adaptation_set.append(track)
            track.order_index = len(adaptation_set)

        # process subtitle tracks
        if options.subtitles:
            for track in [t for t in tracks if t.type == 'subtitles']:
                adaptation_set_name = ('subtitles', track.language, track.codec_family)
                adaptation_set = subtitles_adaptation_sets.get(adaptation_set_name, [])
                subtitles_adaptation_sets[adaptation_set_name] = adaptation_set

                if len([et for et in adaptation_set if et.language == track.language]):
                    continue # only accept one track for each language

                adaptation_set.append(track)
                track.order_index = len(adaptation_set)

    return (audio_adaptation_sets, video_adaptation_sets, subtitles_adaptation_sets, mp4_files)

#############################################
def SelectSubtitlesFiles(options, media_sources):
    return [SubtitlesFile(options, media_source) for media_source in media_sources if media_source.format in ['ttml', 'webvtt']]

#############################################
def ResolveEncryptionKeys(options):
    options.key_infos = []
    for key_spec in options.encryption_key.split(','):
        key_info = {'filter': ['audio', 'video']}
        if key_spec.startswith('audio:') or key_spec.startswith('video:'):
            separator = key_spec.find(':')
            key_info['filter'] = [key_spec[:separator]]
            key_spec = key_spec[separator+1:]
        iv_hex = None
        if key_spec.startswith('@'):
            kid_hex, key_hex = GetEncryptionKey(options, key_spec[1:])
        else:
            if ':' not in key_spec:
                raise Exception('Invalid argument syntax for --encryption-key option')
            parts = key_spec.split(':', 2)
            kid_hex = parts[0]
            key_hex = parts[1]
            if len(parts) > 2:
                iv_hex = parts[2]
            if len(kid_hex) != 32:
                raise Exception('Invalid argument format for --encryption-key option')

            if key_hex.startswith('#'):
                if len(key_hex) != 41:
                    raise Exception('Invalid argument format for --encryption-key option')
                key_seed_bin = key_hex[1:].decode('base64')
                kid_bin = kid_hex.decode('hex')
                key_hex = DerivePlayReadyKey(key_seed_bin, kid_bin).encode('hex')
                if options.verbose:
                    print 'PlayReady Derived Key =', key_hex
            else:
                if len(key_hex) != 32:
                    raise Exception('Invalid argument format for --encryption-key option')

        key_info['key'] = key_hex
        key_info['kid'] = kid_hex
        key_info['iv']  = iv_hex or 'random'
        if options.hls and not iv_hex:
            # for HLS, we need to know the IV
            import random
            sys_random = random.SystemRandom()
            random_iv = sys_random.getrandbits(128)
            key_info['iv'] = '%016x' % random_iv

        options.key_infos.append(key_info)

#############################################
FileNameMap = {}
def MapFileName(from_name, to_name):
    global FileNameMap
    FileNameMap[from_name] = to_name

def GetMappedFileName(filename):
    return FileNameMap.get(filename, filename)

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
                          description="Each <media-file> is the path to a fragmented MP4 file, optionally prefixed with a stream selector delimited by [ and ]. The same input MP4 file may be repeated, provided that the stream selector prefixes select different streams. Version " + VERSION + " r" + SDK_REVISION)
    parser.add_option('-v', '--verbose', dest="verbose", action='store_true', default=False,
                      help="Be verbose")
    parser.add_option('-d', '--debug', dest="debug", action='store_true', default=False,
                      help="Print out debugging information")
    parser.add_option('-o', '--output-dir', dest="output_dir",
                      help="Output directory", metavar="<output-dir>", default='output')
    parser.add_option('-f', '--force', dest="force_output", action="store_true",
                      help="Allow output to an existing directory", default=False)
    parser.add_option('', '--mpd-name', dest="mpd_filename",
                      help="MPD file name", metavar="<filename>", default='stream.mpd')
    parser.add_option('', '--profiles', dest='profiles',
                      help="Comma-separated list of one or more profile(s). Complete profile names can be used, or profile aliases ('live'='"+ISOFF_LIVE_PROFILE+"', 'on-demand'='"+ISOFF_ON_DEMAND_PROFILE+"', 'hbbtv-1.5='"+HBBTV_15_ISOFF_LIVE_PROFILE+"')", default='live',
                      metavar="<profiles>")
    parser.add_option('', '--no-media', dest="no_media", action='store_true', default=False,
                      help="Do not output media files (MPD/Manifests only)")
    parser.add_option('', '--rename-media', dest='rename_media', action='store_true', default=False,
                      help = 'Use a file name pattern instead of the base name of input files for output media files.')
    parser.add_option('', '--media-prefix', dest='media_prefix', metavar='<prefix>', default='media',
                      help='Use this prefix for prefixed media file names (instead of the default prefix "media")')
    parser.add_option('', '--init-segment', dest="init_segment",
                      help="Initialization segment name", metavar="<filename>", default='init.mp4')
    parser.add_option('', "--no-split", action="store_false", dest="split", default=True,
                      help="Do not split the file into individual segment files")
    parser.add_option('', "--use-segment-list", action="store_true", dest="use_segment_list", default=False,
                      help="Use segment lists instead of segment templates")
    parser.add_option('', '--use-segment-template-number-padding', action='store_true', dest='segment_template_padding', default=False,
                      help="Use padded numbers in segment URL/filename templates")
    parser.add_option('', "--use-segment-timeline", action="store_true", dest="use_segment_timeline", default=False,
                      help="Use segment timelines (necessary if segment durations vary)")
    parser.add_option('', "--min-buffer-time", metavar='<duration>', dest="min_buffer_time", type="float", default=0.0,
                      help="Minimum buffer time (in seconds)")
    parser.add_option('', "--max-playout-rate", metavar='<strategy>', dest='max_playout_rate_strategy',
                      help="Max Playout Rate setting strategy for trick-play support. Supported strategies: lowest:X"),
    parser.add_option('', "--language-map", dest="language_map", metavar="<lang_from>:<lang_to>[,...]",
                      help="Remap language code <lang_from> to <lang_to>. Multiple mappings can be specified, separated by ','")
    parser.add_option('', "--always-output-lang", dest="always_output_lang", action='store_true', default=False,
                      help="Always output an @lang attribute for audio tracks even when the language is undefined"),
    parser.add_option('', "--subtitles", dest="subtitles", action="store_true", default=False,
                      help="Enable Subtitles")
    parser.add_option('', "--attributes", dest="attributes", action="append", metavar='<attributes-definition>', default=[],
                      help="Specify the attributes of a set of tracks. This option may be used multiple times, once per attribute set.")
    parser.add_option('', "--smooth", dest="smooth", default=False, action="store_true",
                      help="Produce an output compatible with Smooth Streaming")
    parser.add_option('', '--smooth-client-manifest-name', dest="smooth_client_manifest_filename",
                      help="Smooth Streaming Client Manifest file name", metavar="<filename>", default='stream.ismc')
    parser.add_option('', '--smooth-server-manifest-name', dest="smooth_server_manifest_filename",
                      help="Smooth Streaming Server Manifest file name", metavar="<filename>", default='stream.ism')
    parser.add_option('', '--smooth-h264-fourcc', dest='smooth_h264_fourcc',
                      help="Smooth Streaming FourCC value for H.264 video (default=H264) [some older players use AVC1]", metavar="<fourcc>", default='H264')
    parser.add_option('', '--hls', dest="hls", default=False, action="store_true",
                      help="Output HLS playlists in addition to MPEG DASH")
    parser.add_option('', '--hls-key-url', dest="hls_key_url",
                      help="HLS key URL (default: key.bin)", metavar="<url>", default='key.bin')
    parser.add_option('', '--hls-master-playlist-name', dest="hls_master_playlist_name",
                      help="HLS master playlist name (default: master.m3u8)", metavar="<filename>", default='master.m3u8')
    parser.add_option('', '--hls-media-playlist-name', dest="hls_media_playlist_name",
                      help="HLS media playlist name (default: media.m3u8)", metavar="<filename>", default='media.m3u8')
    parser.add_option('', '--hls-iframes-playlist-name', dest="hls_iframes_playlist_name",
                      help="HLS I-Frames playlist name (default: iframes.m3u8)", metavar="<filename>", default='iframes.m3u8')
    parser.add_option('', "--hippo", dest="hippo", default=False, action="store_true",
                      help="Produce an output compatible with the Hippo Media Server")
    parser.add_option('', '--hippo-server-manifest-name', dest="hippo_server_manifest_filename",
                      help="Hippo Media Server Manifest file name", metavar="<filename>", default='stream.msm')
    parser.add_option('', "--use-compat-namespace", dest="use_compat_namespace", action="store_true", default=False,
                      help="Use the original DASH MPD namespace as it was specified in the first published specification")
    parser.add_option('', "--encryption-key", dest="encryption_key", metavar='<key-spec>', default=None,
                      help="Encrypt some or all tracks with MPEG CENC (AES-128), where <key-spec> specifies the KID(s) and Key(s) to use, using one of the following forms: " +
                           "(1) <KID>:<key> or <KID>:<key>:<IV> with <KID> (and <IV> if specififed) as a 32-character hex string and <key> either a 32-character hex string or the character '#' followed by a base64-encoded key seed; or " +
                           "(2) @<key-locator> where <key-locator> is an expression of one of the supported key locator schemes. Each entry may be prefixed with an optional track filter, and multiple <key-spec> entries can be used, separated by ','. (see online docs for details)")
    parser.add_option('', "--encryption-cenc-scheme", dest="encryption_cenc_scheme", metavar='<cenc-scheme>', default='cenc', choices=('cenc', 'cbc1', 'cens', 'cbcs'),
                      help="MPEG Common Encryption scheme (cenc, cbc1, cens or cbcs). (default: cenc)")
    parser.add_option('', "--encryption-args", dest="encryption_args", metavar='<cmdline-arguments>', default=None,
                      help="Pass additional command line arguments to mp4encrypt (separated by spaces)")
    parser.add_option('', "--eme-signaling", dest="eme_signaling", metavar='<eme-signaling-type>', choices=['pssh-v0', 'pssh-v1'],
                      help="Add EME-compliant signaling in the MPD and PSSH boxes (valid options are 'pssh-v0' and 'pssh-v1')")
    parser.add_option('', "--marlin", dest="marlin", action="store_true", default=False,
                      help="Add Marlin signaling to the MPD (requires an encrypted input, or the --encryption-key option)")
    parser.add_option('', "--marlin-add-pssh", dest="marlin_add_pssh", action="store_true", default=False,
                      help="Add an (optional) Marlin 'pssh' box in the init segment(s)")
    parser.add_option('', "--playready", dest="playready", action="store_true", default=False,
                      help="Add PlayReady signaling to the MPD (requires an encrypted input, or the --encryption-key option)")
    parser.add_option('', "--playready-header", dest="playready_header", metavar='<playready-header>', default=None,
                      help="Add a PlayReady PRO element in the MPD and a PlayReady PSSH box in the init segments. The use of this option implies the --playready option. " +
                           "The <playready-header> argument can be either: " +
                           "(1) the character '@' followed by the name of a file containing a PlayReady XML Rights Management Header (<WRMHEADER>) or a PlayReady Header Object (PRO) in binary form,  or "
                           "(2) the character '#' followed by a PlayReady Header Object encoded in Base64, or " +
                           "(3) one or more <name>:<value> pair(s) (separated by '#' if more than one) specifying fields of a PlayReady Header Object (field names include LA_URL, LUI_URL and DS_ID)")
    parser.add_option('', "--playready-add-pssh", dest="playready_add_pssh", action="store_true", default=False,
                      help="Store the PlayReady header in a 'pssh' box in the init segment(s) [deprecated: this is now implicitly on by default when the --playready or --playready-header option is used]")
    parser.add_option('', "--playready-no-pssh", dest="playready_no_pssh", action="store_true", default=False,
                      help="Do not store the PlayReady header in a 'pssh' box in the init segment(s)")
    parser.add_option('', "--widevine", dest="widevine", action="store_true", default=False,
                      help="Add Widevine signaling to the MPD (requires an encrypted input, or the --encryption-key option)")
    parser.add_option('', "--widevine-header", dest="widevine_header", metavar='<widevine-header>', default=None,
                      help="Add a Widevine entry in the MPD, and a Widevine PSSH box in the init segments. The use of this option implies the --widevine option. " +
                           "The <widevine-header> argument can be either: " +
                           "(1) the character '#' followed by a Widevine header encoded in Base64, or " +
                           "(2) one or more <name>:<value> pair(s) (separated by '#' if more than one) specifying fields of a Widevine header (field names include 'provider' [string], 'content_id' [byte array in hex], 'policy' [string])")
    parser.add_option('', "--primetime", dest="primetime", action="store_true", default=False,
                      help="Add Primetime signaling to the MPD (requires an encrypted input, or the --encryption-key option)")
    parser.add_option('', "--primetime-metadata", dest="primetime_metadata", metavar='<primetime-metadata>', default=None,
                      help="Add Primetime metadata in a PSSH box in the init segments. The use of this option implies the --primetime option. " +
                           "The <primetime-data> argument can be either: " +
                           "(1) the character '@' followed by the name of a file containing the Primetime Metadata to use, or "
                           "(2) the character '#' followed by the Primetime Metadata encoded in Base64")
    parser.add_option('', "--fairplay-key-uri", dest="fairplay_key_uri",
                      help="Specify the key URI to use for FairPlay Streaming key delivery (only valid with --hls option)")
    parser.add_option('', "--exec-dir", metavar="<exec_dir>", dest="exec_dir", default=default_exec_dir,
                      help="Directory where the Bento4 executables are located")
    (options, args) = parser.parse_args()
    if len(args) == 0:
        parser.print_help()
        sys.exit(1)
    global Options
    Options = options

    # set some synthetic (not from command line) options
    options.on_demand = False

    # check the consistency of the options
    if options.smooth:
        options.split = False
        options.use_segment_timeline = True
        if options.use_segment_list:
            raise Exception('ERROR: --smooth and --use-segment-list are mutually exclusive')

    if options.hippo:
        options.split = False
        options.use_segment_timeline = True
        if options.use_segment_list:
            raise Exception('ERROR: --hippo and --use-segment-list are mutually exclusive')

    if not path.exists(options.exec_dir):
        PrintErrorAndExit('Executable directory does not exist ('+Options.exec_dir+'), use --exec-dir')

    if options.max_playout_rate_strategy:
        if not options.max_playout_rate_strategy.startswith('lowest:'):
            PrintErrorAndExit('Max Playout Rate strategy '+options.max_playout_rate_strategy+' is not supported')

    # switch variables
    if options.segment_template_padding:
        global SEGMENT_PATTERN, SEGMENT_URL_PATTERN, SEGMENT_URL_TEMPLATE
        SEGMENT_PATTERN      = PADDED_SEGMENT_PATTERN
        SEGMENT_URL_PATTERN  = PADDED_SEGMENT_URL_PATTERN
        SEGMENT_URL_TEMPLATE = PADDED_SEGMENT_URL_TEMPLATE

    # post-process some of the options
    if not options.profiles:
        if options.use_segment_list:
            options.profiles = [ISOFF_MAIN_PROFILE]
        else:
            options.profiles = [ISOFF_LIVE_PROFILE]
    else:
        profiles = []
        for profile in options.profiles.split(','):
            profile = profile.strip()
            if profile in ProfileAliases:
                profile = ProfileAliases[profile]
            profiles.append(profile)
        options.profiles = profiles
    if ISOFF_ON_DEMAND_PROFILE in options.profiles:
        options.on_demand = True
        options.split = False
        if ISOFF_LIVE_PROFILE in options.profiles:
            raise Exception('on-demand and live profiles are mutually exclusive')
    if HBBTV_15_ISOFF_LIVE_PROFILE in options.profiles:
        options.playready_no_pssh = True
        if options.playready_add_pssh:
            sys.stderr.write('INFO: since hbbtv-1.5 profile is selected, no PlayReady PSSH box will be added to the init segments\n')
        options.always_output_lang = True

    if not options.split:
        if not options.smooth and not options.hippo and not options.on_demand and not options.use_segment_list:
            sys.stderr.write('WARNING: --no-split requires --use-segment-list, which will be enabled automatically\n')
            options.use_segment_list = True

    if options.on_demand and options.use_segment_list:
        raise Exception('segment lists cannot be used with the on-demand profile')

    if options.smooth:
        if ISOFF_LIVE_PROFILE not in options.profiles:
            raise Exception('--smooth requires the live profile')
    if options.hippo:
        if ISOFF_LIVE_PROFILE not in options.profiles:
            raise Exception('--hippo requires the live profile')

    if options.verbose:
        print 'Profiles:', ','.join(options.profiles)

    if options.playready_header or options.playready_add_pssh:
        options.playready = True

    if options.playready:
        options.playready_add_pssh = True

    if options.playready_no_pssh:
        options.playready_add_pssh = False

    if options.widevine_header:
        options.widevine = True
        if options.hls and options.widevine_header.startswith('#'):
            raise Exception('with --hls, only the <name>:<value> pair syntax is supported for the --widevine-header option')

    if options.primetime_metadata:
        options.primetime = True

    if options.fairplay_key_uri:
        if not options.hls:
            sys.stderr.write('WARNING: --fairplay-key-uri is only valid with --hls, ignoring\n')
            
    if options.hls:
        if options.encryption_key and options.encryption_cenc_scheme != 'cbcs':
            raise Exception('--hls requires --encryption-cenc-scheme=cbcs')

    # compute the KID(s) and encryption key(s) if needed
    if options.encryption_key:
        ResolveEncryptionKeys(options)
        if options.verbose:
            for key_info in options.key_infos:
                if key_info['key']:
                    message = 'KID='+key_info['kid']+', KEY='+key_info['key']+', IV='+key_info['iv']
                else:
                    message = '[NOT ENCRYPTED]'
                print 'Key Info for '+'/'.join(key_info['filter'])+': '+message

    # process language map options
    if options.language_map:
        mappings = options.language_map.split(',')
        options.language_map = {}
        for mapping in mappings:
            from_lang, to_lang = mapping.split(':')
            options.language_map[from_lang] = to_lang

    # parse the attributes definitions
    set_attributes = {}
    for set_attributes_spec in options.attributes:
        try:
            set_name, attributes = set_attributes_spec.split(':', 1)
            set_attributes[set_name] = {}
            for attribute in attributes.split(','):
                name, value = attribute.split('=', 1)
                set_attributes[set_name][name] = value
        except:
            raise Exception('Invalid syntax for --attributes option')

    # parse media sources syntax
    media_sources = [MediaSource(source) for source in args]

    # create the output directory
    severity = 'ERROR'
    if options.no_media: severity = 'WARNING'
    if options.force_output: severity = None
    MakeNewDir(dir=options.output_dir, exit_if_exists = not (options.no_media or options.force_output), severity=severity)

    # for on-demand, we need to first extract tracks into individual media files
    if options.on_demand:
        (audio_sets, video_sets, subtitles_sets, mp4_files) = SelectTracks(options, media_sources)
        media_sources = filter(lambda x: x.format == "webvtt", media_sources) #Keep subtitles
        for track in sum(audio_sets.values()+video_sets.values(), []):
            print 'Extracting track', track.id, 'from', GetMappedFileName(track.parent.media_source.filename)
            track_file = tempfile.NamedTemporaryFile(dir = options.output_dir, delete=False)
            TempFiles.append(track_file.name)
            track_file.close() # necessary on Windows
            MapFileName(track_file.name, path.basename(track_file.name) + ' = Extracted[track '+str(track.id) + ' from '+GetMappedFileName(track.parent.media_source.filename)+']')

            Mp4Fragment(options,
                        track.parent.media_source.filename,
                        track_file.name,
                        track = str(track.id),
                        index = True,
                        quiet = True)

            media_source = MediaSource(track_file.name)
            media_source.spec = track.parent.media_source.spec
            media_sources.append(media_source)

    # encrypt the input files if needed
    if options.encryption_key:
        encrypted_files = {}
        for media_source in media_sources:
            if media_source.format != 'mp4': continue

            media_file = media_source.filename

            # check if we have already encrypted this file
            if media_file in encrypted_files:
                media_source.filename = encrypted_files[media_file].name
                continue

            # get the mp4 file info
            json_info = Mp4Info(Options, media_file, format='json', fast=True)
            info = json.loads(json_info, strict=False)

            if not info['movie']['fragments']:
                PrintErrorAndExit('ERROR: file '+media_file+' is not fragmented (use mp4fragment to fragment it)')

            if 'tracks' not in info:
                raise Exception('No track found in input file(s)')

            # select which KID/KEY to use for each track
            default_kid = options.key_infos[0]['kid']
            default_key = options.key_infos[0]['key']
            media_source.track_key_infos = {}
            for track in info['tracks']:
                for key_info in options.key_infos:
                    if track['type'].lower() in key_info['filter']:
                        media_source.track_key_infos[track['id']] = key_info

            # skip now if we're only outputing the MPD
            if options.no_media:
                continue

            # don't process any further if we won't have key material for this media source
            if len(media_source.track_key_infos) == 0:
                continue

            print 'Encrypting track IDs '+str(sorted(media_source.track_key_infos.keys()))+' in '+ GetMappedFileName(media_file)
            encrypted_file = tempfile.NamedTemporaryFile(dir = options.output_dir, delete=False)
            encrypted_files[media_file] = encrypted_file
            TempFiles.append(encrypted_file.name)
            encrypted_file.close() # necessary on Windows
            MapFileName(encrypted_file.name, path.basename(encrypted_file.name) + ' = Encrypted[' + GetMappedFileName(media_file) + ']')
            args = ['--method', MpegCencSchemeMap[options.encryption_cenc_scheme]]

            if options.encryption_args:
                args += options.encryption_args.split()
            else:
                if options.smooth or options.playready:
                    args += ['--global-option', 'mpeg-cenc.piff-compatible:true']

            for track_id in sorted(media_source.track_key_infos.keys()):
                key_info = media_source.track_key_infos[track_id]
                args += ['--key', str(track_id)+':'+key_info['key']+':'+key_info['iv'], '--property', str(track_id)+':KID:'+key_info['kid']]

            # EME Common Encryption / Clearkey
            if options.eme_signaling == 'pssh-v0':
                args += ['--pssh', EME_COMMON_ENCRYPTION_PSSH_SYSTEM_ID+':']
            elif options.eme_signaling == 'pssh-v1':
                args += ['--pssh-v1', EME_COMMON_ENCRYPTION_PSSH_SYSTEM_ID+':']

            # Marlin
            if options.marlin_add_pssh:
                marlin_pssh = ComputeMarlinPssh(options)
                pssh_file = tempfile.NamedTemporaryFile(dir = options.output_dir, delete=False)
                pssh_file.write(marlin_pssh)
                TempFiles.append(pssh_file.name)
                pssh_file.close() # necessary on Windows
                args += ['--pssh', MARLIN_PSSH_SYSTEM_ID+':'+pssh_file.name]

            # PlayReady
            if options.playready_add_pssh:
                playready_header = ComputePlayReadyHeader(options.playready_header, default_kid, default_key)
                pssh_file = tempfile.NamedTemporaryFile(dir = options.output_dir, delete=False)
                pssh_file.write(playready_header)
                TempFiles.append(pssh_file.name)
                pssh_file.close() # necessary on Windows
                args += ['--pssh', PLAYREADY_PSSH_SYSTEM_ID+':'+pssh_file.name]

            # Widevine
            if options.widevine_header:
                widevine_header = ComputeWidevineHeader(options.widevine_header, default_kid)
                pssh_file = tempfile.NamedTemporaryFile(dir = options.output_dir, delete=False)
                pssh_file.write(widevine_header)
                TempFiles.append(pssh_file.name)
                pssh_file.close() # necessary on Windows
                args += ['--pssh', WIDEVINE_PSSH_SYSTEM_ID+':'+pssh_file.name]

            # Primetime
            if options.primetime_metadata:
                primetime_metadata = ComputePrimetimeMetaData(options.primetime_metadata, default_kid)
                pssh_file = tempfile.NamedTemporaryFile(dir = options.output_dir, delete=False)
                pssh_file.write(primetime_metadata)
                TempFiles.append(pssh_file.name)
                pssh_file.close() # necessary on Windows
                args += ['--pssh', PRIMETIME_PSSH_SYSTEM_ID+':'+pssh_file.name]

            Mp4Encrypt(options, media_file, encrypted_file.name, *args)
            media_source.filename = encrypted_file.name

    # parse the media sources and select the audio and video tracks
    (audio_sets, video_sets, subtitles_sets, mp4_files) = SelectTracks(options, media_sources)
    subtitles_files = SelectSubtitlesFiles(options, media_sources)

    # store lists of all tracks by type
    audio_tracks     = sum(audio_sets.values(),     [])
    video_tracks     = sum(video_sets.values(),     [])
    subtitles_tracks = sum(subtitles_sets.values(), [])

    # check that we have at least one audio and one video
    if len(audio_tracks) == 0 and len(video_tracks) == 0 and len(subtitles_tracks) == 0:
        PrintErrorAndExit('ERROR: no track selected')

    if Options.verbose:
        print 'Audio:',     audio_sets
        print 'Video:',     video_sets
        print 'Subtitles:', subtitles_sets

    # assign key info to tracks
    if options.encryption_key:
        for track in audio_tracks+video_tracks+subtitles_tracks:
            track.key_info = track.parent.media_source.track_key_infos.get(track.id)

    # check that segments are consistent between tracks of the same adaptation set
    for tracks in video_sets.values():
        prev_track = None
        for track in tracks:
            if prev_track:
                if track.total_sample_count != prev_track.total_sample_count:
                    sys.stderr.write('WARNING: video sample count mismatch between "'+str(track)+'" and "'+str(prev_track)+'"\n')
            prev_track = track

    # check that the video segments match for all video tracks in the same adaptation set
    for tracks in video_sets.values():
        if len(tracks) > 1:
            anchor = tracks[0]
            for track in tracks[1:]:
                if track.sample_counts[:-1] != anchor.sample_counts[:-1]:
                    PrintErrorAndExit('ERROR: video tracks are not aligned ("'+str(track)+'" differs from '+str(anchor)+')')

    # check that the video segment durations are almost all equal
    if not options.use_segment_timeline:
        for video_track in video_tracks:
            for segment_duration in video_track.segment_durations[:-2]:
                ratio = segment_duration/video_track.average_segment_duration
                if ratio > 1.1 or ratio < 0.9:
                    sys.stderr.write('WARNING: video segment durations for "' + str(video_track) + '" vary by more than 10% (consider using --use-segment-timeline)\n')
                    break
        for audio_track in audio_tracks:
            for segment_duration in audio_track.segment_durations[:-2]:
                ratio = segment_duration/audio_track.average_segment_duration
                if ratio > 1.1 or ratio < 0.9:
                    sys.stderr.write('WARNING: audio segment durations for "' + str(audio_track) + '" vary by more than 10% (consider using --use-segment-timeline)\n')
                    break

    # round the audio segment durations to be equal to the video segment durations
    if len(video_tracks):
        for audio_track in audio_tracks:
            ratio = audio_track.average_segment_duration/video_tracks[0].average_segment_duration
            if abs(ratio-1.0) < 0.05:
                # within 5%, make it equal
                if options.verbose:
                    print 'INFO: adjusting segment duration for audio track '+str(audio_track)+' to '+str(video_tracks[0].average_segment_duration)+' to match the video'
                audio_track.average_segment_duration = video_tracks[0].average_segment_duration

    # compute the representation id and init segment name for each track
    for adaptation_sets in [audio_sets, video_sets, subtitles_sets]:
        for adaptation_set_name, tracks in adaptation_sets.items():
            for track in tracks:
                if options.split:
                    track.representation_id = '/'.join(adaptation_set_name)
                    if len(tracks) > 1:
                        track.representation_id += '/'+str(track.order_index)
                    track.init_segment_name = SPLIT_INIT_SEGMENT_NAME
                else:
                    track.representation_id = '-'.join(adaptation_set_name)
                    if len(tracks) > 1:
                        track.representation_id += '-'+str(track.order_index)
                    if options.on_demand:
                        track.parent.media_name = ONDEMAND_MEDIA_FILE_PATTERN % (options.media_prefix, track.representation_id)
                    else:
                        track.init_segment_name = NOSPLIT_INIT_FILE_PATTERN % (track.representation_id)
                track.stream_id = adaptation_set_name[0]
                if adaptation_set_name[0] == 'audio':
                    track.stream_id += '_'+track.language

    # compute index and init offsets for the on-demand profile
    if options.on_demand:
        for track in audio_tracks+video_tracks+subtitles_tracks:
            atoms = WalkAtoms(track.parent.media_source.filename, 'moof')
            for atom in atoms:
                if atom.type == 'sidx' and not hasattr(track, 'sidx_atom'):
                    track.sidx_atom = atom
                if atom.type == 'moov' and not hasattr(track, 'moov_atom'):
                    track.moov_atom = atom

    # compute some values if not set
    if options.min_buffer_time == 0.0:
        if len(video_tracks):
            options.min_buffer_time = video_tracks[0].average_segment_duration
        else:
            options.min_buffer_time = audio_tracks[0].average_segment_duration

    # print info about the tracks
    if options.verbose:
        for track in audio_tracks+video_tracks+subtitles_tracks:
            print ('%s track: ' + str(track) + ' - language=%s, max bitrate=%d, avg bitrate=%d, req bandwidth=%d, codec=%s') % (track.type, track.language, track.max_segment_bitrate, track.average_segment_bitrate, track.bandwidth, track.codec)

    # deal with the max playout strategy if set
    if options.max_playout_rate_strategy:
        max_playout_rate = options.max_playout_rate_strategy.split(':')[1]
        lowest_bandwidth_track = None
        lowest_bandwidth = -1
        for video_track in video_tracks:
            if lowest_bandwidth < 0 or video_track.bandwidth < lowest_bandwidth:
                lowest_bandwidth = video_track.bandwidth
                lowest_bandwidth_track = video_track
        if lowest_bandwidth_track:
            lowest_bandwidth_track.max_playout_rate = max_playout_rate

    # create the directories and split/copy/process the media if needed
    if not options.no_media:
        if options.split:
            for adaptation_sets in [audio_sets, video_sets, subtitles_sets]:
                for adaptation_set_name, tracks in adaptation_sets.items():
                    base_dir = options.output_dir
                    for subdir in adaptation_set_name:
                        base_dir = path.join(base_dir, subdir)
                        MakeNewDir(base_dir)
                    for track in tracks:
                        out_dir = base_dir
                        if len(tracks) > 1:
                            out_dir = path.join(out_dir, str(track.order_index))
                            MakeNewDir(out_dir)
                        print 'Splitting media file ('+adaptation_set_name[0]+')', GetMappedFileName(track.parent.media_source.filename)
                        Mp4Split(options,
                                 track.parent.media_source.filename,
                                 track_id               = str(track.id),
                                 pattern_parameters     = 'N',
                                 start_number           = '1',
                                 init_segment           = path.join(out_dir, track.init_segment_name),
                                 media_segment          = path.join(out_dir, SEGMENT_PATTERN))

            # if len(video_tracks):
            #     MakeNewDir(path.join(options.output_dir, 'video'))
            # for video_track in video_tracks:
            #     out_dir = path.join(options.output_dir, 'video', str(video_track.order_index))
            #     MakeNewDir(out_dir)
            #     print 'Splitting media file (video)', GetMappedFileName(video_track.parent.media_source.filename)
            #     Mp4Split(Options,
            #              video_track.parent.media_source.filename,
            #              track_id               = str(video_track.id),
            #              pattern_parameters     = 'N',
            #              start_number           = '1',
            #              init_segment           = path.join(out_dir, video_track.init_segment_name),
            #              media_segment          = path.join(out_dir, SEGMENT_PATTERN))
            #
            # for adaptation_set_name, tracks in subtitles_sets.items():
            #     out_dir = options.output_dir
            #     for subdir in adaptation_set_name:
            #         out_dir = path.join(out_dir, subdir)
            #         MakeNewDir(out_dir)
            #     for subtitles_track in tracks:
            #         print 'Splitting media file (subtitles)', GetMappedFileName(subtitles_track.parent.media_source.filename)
            #         Mp4Split(Options,
            #                  subtitles_track.parent.media_source.filename,
            #                  track_id               = str(subtitles_track.id),
            #                  pattern_parameters     = 'N',
            #                  start_number           = '1',
            #                  init_segment           = path.join(out_dir, subtitles_track.init_segment_name),
            #                  media_segment          = path.join(out_dir, SEGMENT_PATTERN))
        else:
            for mp4_file in mp4_files.values():
                print 'Processing and Copying media file', GetMappedFileName(mp4_file.media_source.filename)
                media_filename = path.join(options.output_dir, mp4_file.media_name)
                if not options.force_output and path.exists(media_filename):
                    PrintErrorAndExit('ERROR: file ' + media_filename + ' already exists')

                shutil.copyfile(mp4_file.media_source.filename, media_filename)
            if options.smooth or options.hippo:
                for track in audio_tracks+video_tracks+subtitles_tracks:
                    Mp4Split(options,
                             track.parent.media_source.filename,
                             track_id     = str(track.id),
                             init_only    = True,
                             init_segment = path.join(options.output_dir, track.init_segment_name))

        if len(subtitles_files):
            MakeNewDir(path.join(options.output_dir, 'subtitles'))
            for subtitles_file in subtitles_files:
                print 'Processing and Copying subtitles file', GetMappedFileName(subtitles_file.media_source.filename)
                out_dir = path.join(options.output_dir, 'subtitles', subtitles_file.language)
                MakeNewDir(out_dir)
                media_filename = path.join(out_dir, subtitles_file.media_name)
                shutil.copyfile(subtitles_file.media_source.filename, media_filename)

    # output the DASH MPD
    OutputDash(options, set_attributes, audio_sets, video_sets, subtitles_sets, subtitles_files)

    # output the HLS playlists
    if options.hls:
        OutputHls(options, set_attributes, audio_sets, video_sets, subtitles_sets, subtitles_files)

    # output the Smooth Manifests
    if options.smooth:
        OutputSmooth(options, audio_tracks, video_tracks)

    # output the Hippo Manifest
    if options.hippo:
        OutputHippo(options, audio_tracks, video_tracks)

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
    finally:
        for f in TempFiles:
            os.unlink(f)
