#!/usr/bin/env python
from __future__ import print_function

__author__    = 'Jonas Birme (jonas.birme@eyevinn.se)'
__copyright__ = 'Copyright 2016 Eyevinn Technology (http://eyevinn.github.io)'

###
# NOTE: this script needs BEnto4 command line binaries to run
# You must place the 'mp4fragment' binaries 
# in a directory named 'bin/<platform>' at the same level as where this script is
# <platform> depends on the platform you're running on:
# Mac OSX   --> platform = macosx
# Linux x86 --> platform = linux-x86
# Windows   --> platform = win32
#
# Depends on
# - m3u8: (pip install m3u8)
# - pycurl: (pip install pycurl)
# - ffprobe: (pip install ffprobe)

import argparse
import sys
import tempfile
import platform
import re
import time
import datetime
from mp4utils import *
import m3u8
import pycurl
from ffprobe import FFProbe
import subprocess
import shlex

VERSION = "1.0.0"
SDK_REVISION = '613'
SCRIPT_PATH = path.abspath(path.dirname(__file__))
sys.path += [SCRIPT_PATH]

MPD_NS                      = 'urn:mpeg:dash:schema:mpd:2011'
ISOFF_LIVE_PROFILE          = 'urn:mpeg:dash:profile:isoff-live:2011'
DASH_DEFAULT_VIDEO_TIMESCALE= 90000
DASH_DEFAULT_AUDIO_TIMESCALE= 48000

TempFiles = []
Options = None

def debug_log(*args, **kwargs):
    if Options.debug:
        print(*args, file=sys.stderr, **kwargs)

def FFMpegCommand(infile, outfile, opts):
    exec_dir = Options.ffmpeg_exec_dir
    executable = path.join(exec_dir, 'ffmpeg')
    cmd = [executable]
    cmd.append('-i')
    cmd.append(infile)
    args = shlex.split(opts)
    cmd += args
    cmd.append(outfile)
    debug_log('COMMAND: %s' % cmd)
    try:
        FNULL = open(os.devnull, 'w')
        try:
            return subprocess.call(cmd, stdout=FNULL, stderr=subprocess.STDOUT)
        except OSError as e:
            debug_log('executable ' + executable + ' not found in exec_dir, trying with PATH')
            cmd[0] = path.basename(cmd[0])
            return subprocess.call(cmd, stdout=FNULL, stderr=subprocess.STDOUT)
    except CalledProcessError as e:
        message = "binary tool failed with error %d" % e.returncode
        raise Exception(message)
    except OSError as e:
        raise Exception('executable "'+name+'" not found, ensure that it is in your path or in the directory '+exec_dir)


class PT:
    def __init__(self, seconds):
        self.seconds = seconds
    def __str__(self):
        return "PT%dS" % self.seconds

class Period:
    def __init__(self, periodid):
        self.id = periodid
        self.periodStart = 0
        self.periodDuration = 30
        self.as_video = None
        self.as_audio = None
    def setPeriodStart(self, start):
        self.periodStart = start
    def increaseDuration(self, duration):
        self.periodDuration += duration
    def addAdaptationSetVideo(self, as_video):
        self.as_video = as_video
    def haveAdaptationSetVideo(self):
        return self.as_video != None
    def getAdaptationSetVideo(self):
        return self.as_video
    def addAdaptationSetAudio(self, as_audio):
        self.as_audio = as_audio
    def haveAdaptationSetAudio(self):
        return self.as_audio != None
    def getAdaptationSetAudio(self):
        return self.as_audio
    def asXML(self):
        xml = '  <Period id="%s" start="%s">\n' % (self.id, PT(self.periodStart))
        xml += self.as_video.asXML()
        xml += self.as_audio.asXML()
        xml += '  </Period>\n'
        return xml

class Segment:
    def __init__(self, duration, isFirst):
        self.duration = duration
        self.timescale = 1
        self.startTime = 0
        self.isFirst = isFirst
    def setTimescale(self, timescale):
        self.timescale = timescale 
    def setStartTime(self, startTime):
        self.startTime = startTime
    def asXML(self):
        if self.isFirst:
            xml = '          <S t="%d" d="%d" />\n' % (int(self.startTime * self.timescale), self.duration * self.timescale)
        else:
            xml = '          <S d="%d" />\n' % (int(self.duration * self.timescale))
        return xml
    def __str__(self):
        return '(duration=%s)' % (self.duration)

class TSBase:
    def __init__(self):
        self.startTime = 0
        self.duration = 0
        self.streams = []
    def parsedata(self, probedata):
        if len(probedata.streams) > 0:
            self.startTime = float(probedata.streams[0].start_time)
            debug_log("Probed TS: startTime=%s" % self.startTime)
    def isRemuxed(self, outdir, profile, number):
        dashaudiopath = '%s/audio-%s_%s.dash' % (outdir, profile, number)
        return os.path.exists(dashaudiopath)
    def getStartTime(self):
        return self.startTime
    def cleanup(self):
        return


class TSRemote(TSBase):
    def __init__(self, uri):
        TSBase.__init__(self)
        self.uri = uri
        self.downloadedFile = None
        res = re.match('.*/(.*\.ts)$', self.uri)
        self.fname = res.group(1)
        self.fpath = Options.tmpdir + "/" + self.fname
    def probe(self):
        self.download()
        self.parsedata(FFProbe(self.downloadedFile.name))
    def download(self):
        if self.downloadedFile == None:
            debug_log("Downloading %s" % self.fname)
            self.downloadedFile = open(self.fpath, 'wb')
            c = pycurl.Curl()
            c.setopt(c.URL, self.uri)
            c.setopt(c.WRITEDATA, self.downloadedFile)
            c.perform()
            c.close
            self.downloadedFile.close()
    def remuxMP4(self, outdir, profile, number):
        self.download()
        dashaudiopath = '%s/audio-%s_%s.dash' % (outdir, profile, number)
        dashvideopath = '%s/video-%s_%s.dash' % (outdir, profile, number)
        tmpaudio = tempfile.NamedTemporaryFile(dir=Options.tmpdir, suffix='.mp4')
        tmpvideo = tempfile.NamedTemporaryFile(dir=Options.tmpdir, suffix='.mp4')
        FFMpegCommand(self.downloadedFile.name, tmpaudio.name, '-y -bsf:a aac_adtstoasc -acodec copy -vn')
        FFMpegCommand(self.downloadedFile.name, tmpvideo.name, '-y -vcodec copy -an')
        self.probe()
        Mp4Fragment(Options, tmpaudio.name, dashaudiopath, tfdt_start=str(self.getStartTime()), quiet=True) 
        Mp4Fragment(Options, tmpvideo.name, dashvideopath, tfdt_start=str(self.getStartTime()), quiet=True) 
    def getFilename(self):
        return self.downloadedFile.name
    def cleanup(self):
        return

class TSLocal(TSBase):
    def __init__(self, path):
        TSBase.__init__(self)
        self.path = path
    def probe(self):
        self.parsedata(FFProbe(self.path))
    def remuxMP4(self, outdir, profile, number):
        dashaudiopath = '%s/audio-%s_%s.dash' % (outdir, profile, number)
        dashvideopath = '%s/video-%s_%s.dash' % (outdir, profile, number)
        tmpaudio = tempfile.NamedTemporaryFile(dir=Options.tmpdir, suffix='.mp4')
        tmpvideo = tempfile.NamedTemporaryFile(dir=Options.tmpdir, suffix='.mp4')
        FFMpegCommand(self.path, tmpaudio.name, '-y -bsf:a aac_adtstoasc -acodec copy -vn')
        FFMpegCommand(self.path, tmpvideo.name, '-y -vcodec copy -an')
        Mp4Fragment(Options, tmpaudio.name, dashaudiopath, tfdt_start=str(self.getStartTime()), quiet=True) 
        Mp4Fragment(Options, tmpvideo.name, dashvideopath, tfdt_start=str(self.getStartTime()), quiet=True) 
    def getFilename(self):
        return self.path

class TSStream:
    def __init__(self, streamid):
        self.id = streamid



class RepresentationBase:
    def __init__(self, representationid, bandwidth):
        self.id = representationid
        self.bandwidth = bandwidth
    def getBandwidth(self):
        return self.bandwidth
    def asXML(self):
        xml = '      <Representation id="%s" bandwidth="%s" />\n' % (self.id, self.bandwidth)
        return xml
    def __str__(self):
        return "(id=%s, bandwidth=%s)" % (self.id, self.bandwidth)

class RepresentationVideo(RepresentationBase):
    def __init__(self, representationid, bandwidth, width, height):
        RepresentationBase.__init__(self, representationid, bandwidth)
        self.width = width
        self.height = height
    def getHeight(self):
        return self.height
    def getWidth(self):
        return self.width
    def asXML(self):
        xml = '      <Representation id="%s" width="%s" height="%s" bandwidth="%s" scanType="progressive" />\n' % (self.id, self.width, self.height, self.bandwidth)
        return xml
    def __str__(self):
        return "(id=%s, bandwidth=%s, width=%s, height=%s)" % (self.id, self.bandwidth, self.width, self.height)

class RepresentationAudio(RepresentationBase):
    def __init__(self, representationid, bandwidth):
        RepresentationBase.__init__(self, representationid, bandwidth)
 
class MPD_Base:
    def __init__(self):
        self.maxSegmentDuration = 10
        self.firstSegmentStartTime = 0
        self.periods = []
        period = Period('1')
        period.setPeriodStart(0)
        self.periods.append(period)
    def havePeriods(self):
        return len(self.periods) > 0
    def getPeriod(self, idx):
        return self.periods[idx]
    def getAllPeriods(self):
        return self.periods;
    def asXML(self):
        xml = '<?xml version="1.0"?>';
        xml += '<MPD xmlns="%s" profiles="%s" type="dynamic" minimumUpdatePeriod="PT10S" minBufferTime="PT1.500S" maxSegmentDuration="%s" availabilityStartTime="%s" publishTime="%s">\n' % (MPD_NS, ISOFF_LIVE_PROFILE, PT(self.maxSegmentDuration), self._getAvailabilityStartTime(), self._getPublishTime())
        if self.havePeriods():
            for p in self.getAllPeriods():
                xml += p.asXML()
        xml += '</MPD>\n'
        return xml
    def _getAvailabilityStartTime(self):
        tsnow = time.time()
        availstart = tsnow - self.firstSegmentStartTime
        return datetime.datetime.fromtimestamp(availstart).isoformat() + "Z"
    def _getPublishTime(self):
        return datetime.datetime.utcnow().isoformat() + "Z"

class AdaptationSetBase:
    def __init__(self, mimeType, codec, timescale):
        self.representations = []
        self.segments = []
        self.mimeType = mimeType
        self.codec = codec
        self.timescale = timescale
        self.startNumber = '0'
        self.presentationTimeOffset = 0
    def addRepresentation(self, representation):
        self.representations.append(representation)
    def addSegment(self, segment):
        segment.setTimescale(self.timescale)
        self.segments.append(segment)
    def setStartNumber(self, startNumber):
        self.startNumber = startNumber.lstrip('0')
    def setStartTime(self, startTime):
        self.presentationTimeOffset = int(startTime * self.timescale)
    def __str__(self):
        s = "(mimeType=%s, codec=%s, representations=%d):\n" % (self.mimeType, self.codec, len(self.representations))
        for r in self.representations:
            s += "        + " + str(r) + "\n"
        for seg in self.segments:
            s += "           + " + str(seg) + "\n"
        return s

class AdaptationSetVideo(AdaptationSetBase):
    def __init__(self, mimeType, codec):
        AdaptationSetBase.__init__(self, mimeType, codec, DASH_DEFAULT_VIDEO_TIMESCALE)
    def asXML(self):
        idxlist = xrange(len(self.representations))
        maxWidth = self.representations[max(idxlist, key = lambda x: self.representations[x].getWidth())].getWidth()
        maxHeight = self.representations[max(idxlist, key = lambda x: self.representations[x].getHeight())].getHeight()
        maxBandwidth = self.representations[max(idxlist, key = lambda x: self.representations[x].getBandwidth())].getBandwidth()
        minWidth = self.representations[min(idxlist, key = lambda x: self.representations[x].getWidth())].getWidth()
        minHeight = self.representations[min(idxlist, key = lambda x: self.representations[x].getHeight())].getHeight()
        minBandwidth = self.representations[min(idxlist, key = lambda x: self.representations[x].getBandwidth())].getBandwidth()
        xml = ''
        xml += '    <AdaptationSet mimeType="%s" codecs="%s" minWidth="%d" maxWidth="%d" minHeight="%d" maxHeight="%d" startWithSAP="1" segmentAlignment="true" minBandwidth="%d" maxBandwidth="%d">\n' % (self.mimeType, self.codec, minWidth, maxWidth, minHeight, maxHeight, minBandwidth, maxBandwidth)
        xml += '      <SegmentTemplate timescale="%d" media="$RepresentationID$_$Number$.dash" startNumber="%s">\n' % (self.timescale, self.startNumber)
        xml += '        <SegmentTimeline>\n';
        for s in self.segments:
            xml += s.asXML()
        xml += '        </SegmentTimeline>\n';
        xml += '      </SegmentTemplate>\n'
        for r in self.representations:
            xml += r.asXML()
        xml += '    </AdaptationSet>\n'
        return xml

class AdaptationSetAudio(AdaptationSetBase):
    def __init__(self, mimeType, codec):
        AdaptationSetBase.__init__(self, mimeType, codec, DASH_DEFAULT_AUDIO_TIMESCALE)
    def asXML(self):
        xml = ''
        xml += '    <AdaptationSet mimeType="%s" codecs="%s">\n' % (self.mimeType, self.codec)
        xml += '      <SegmentTemplate timescale="%d" media="$RepresentationID$_$Number$.dash" startNumber="%s">\n' % (self.timescale, self.startNumber)
        xml += '        <SegmentTimeline>\n';
        for s in self.segments:
            xml += s.asXML()
        xml += '        </SegmentTimeline>\n';
        xml += '      </SegmentTemplate>\n'
        for r in self.representations:
            xml += r.asXML()
        xml += '    </AdaptationSet>\n'
        return xml


 
class MPD_HLS(MPD_Base):
    def __init__(self, playlistlocator, remux, remuxoutput):
        MPD_Base.__init__(self)
        self.playlistlocator = playlistlocator
        self.profilepattern = 'master(\d+).m3u8' #TODO fix this hardcoded pattern
        self.numberpattern = 'master\d+_(\d+).ts' #TODO fix this hardcoded pattern
        self.isRemote = False
        self.doRemux = remux
        self.remuxOutput = remuxoutput
        self.baseurl = ''
        res = re.match('^(.*)/.*.m3u8$', playlistlocator)
        if res:
            self.baseurl = res.group(1) + '/'
	if re.match('^http', playlistlocator):
            self.isRemote = True
        self.currentPeriod = 0
    def setProfilePattern(self, profilepattern):
        self.profilepattern = profilepatten
    def load(self):
        debug_log("Loading playlist: ", self.playlistlocator)
        m3u8_obj = m3u8.load(self.playlistlocator)
        if m3u8_obj.is_variant:
            if m3u8_obj.playlist_type == "VOD":
                raise Exception("VOD playlists not yet supported")
            self._parseMaster(m3u8_obj)
        else:
            raise Exception("Can only create DASH manifest from an HLS master playlist")
        m3u8_pl = None
        if self.doRemux:
            for pl in m3u8_obj.playlists:
                m3u8_pl = m3u8.load(self.baseurl + pl.uri)
                self._remuxSegmentsInPlaylist(pl.uri, m3u8_pl)
        else:
            p = m3u8_obj.playlists[0]
            debug_log("Loading playlist: ", self.baseurl + p.uri)
            m3u8_pl = m3u8.load(self.baseurl + p.uri)
        self._parsePlaylist(m3u8_pl)
        for per in self.getAllPeriods():
            debug_log("Audio: ", per.as_audio)
            debug_log("Video: ", per.as_video)
    def _remuxSegmentsInPlaylist(self, playlisturi, playlist):
        for seg in playlist.segments:
            seqno = self._getStartNumberFromFilename(seg.uri).lstrip('0')
            profile = self._profileFromFilename(playlisturi)
            segts = None
            if self.isRemote:
                if not re.match('^http', seg.uri):
                    uri = self.baseurl + seg.uri
                segts = TSRemote(uri)
            else:
                segts = TSLocal(seg.uri)
            segts.download()
            number = self._getStartNumberFromFilename(seg.uri)
            if not segts.isRemuxed(self.remuxOutput, profile, number):
                segts.remuxMP4(self.remuxOutput, profile, number)
                segts.cleanup()
    def _profileFromFilename(self, filename):
        result = re.match(self.profilepattern, filename)
        if result:
            return result.group(1)
        else:
            return len(self.representations)
    def _getStartNumberFromFilename(self, filename):
        result = re.match(self.numberpattern, filename)
        if result:
            return result.group(1)
        return 0
    def _parsePlaylist(self, playlist):
        self.maxSegmentDuration = playlist.target_duration
        isFirst = True
        for seg in playlist.segments:
            duration = float(seg.duration)
            videoseg = Segment(duration, isFirst)
            audioseg = Segment(duration, isFirst)
            period = self.getPeriod(self.currentPeriod)
            period.getAdaptationSetVideo().addSegment(videoseg)
            period.getAdaptationSetAudio().addSegment(audioseg)
            period.increaseDuration(int(duration))
            if isFirst:
                self.firstSegmentStartTime = self._getStartTimeFromFile(seg.uri)
                videoseg.setStartTime(self.firstSegmentStartTime)
                audioseg.setStartTime(self.firstSegmentStartTime)
                as_audio = period.getAdaptationSetAudio()
                as_video = period.getAdaptationSetVideo()
                as_video.setStartNumber(self._getStartNumberFromFilename(seg.uri))
                as_video.setStartTime(self.firstSegmentStartTime)
                as_audio.setStartNumber(self._getStartNumberFromFilename(seg.uri))
                as_audio.setStartTime(self.firstSegmentStartTime)
            isFirst = False
    def _getStartTimeFromFile(self, uri):
        if self.isRemote:
            if not re.match('^http', uri):
                uri = self.baseurl + uri
            ts = TSRemote(uri)
        else:
            ts = TSLocal(uri)
        ts.probe()
        return ts.getStartTime()
    def _parseMaster(self, variant):
        debug_log("Parsing master playlist")
        for playlist in variant.playlists:
            stream = playlist.stream_info
            (video_codec, audio_codec) = stream.codecs.split(',')
            profile = self._profileFromFilename(playlist.uri) 
            period = self.getPeriod(self.currentPeriod)
            if not period.haveAdaptationSetVideo():
                as_video = AdaptationSetVideo('video/mp4', video_codec) 
                period.addAdaptationSetVideo(as_video)
            if not period.haveAdaptationSetAudio():
                as_audio = AdaptationSetAudio('audio/mp4', audio_codec) 
                audio_representation = RepresentationAudio('audio-%s' % profile, 96000)
                as_audio.addRepresentation(audio_representation)
                period.addAdaptationSetAudio(as_audio)
            video_representation = RepresentationVideo('video-%s' % profile, stream.bandwidth, stream.resolution[0], stream.resolution[1])
            period.getAdaptationSetVideo().addRepresentation(video_representation)


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


    parser = argparse.ArgumentParser(
        description="Generate an MPEG DASH manifest from a live HLS source including the option to download and rewrap TS segments to MP4 fragments. Writes MPEG DASH manifest to stdout",
        epilog="Example (no remux): hls2mp4dash http://example.com/live/event/master.m3u8\n\n Example (remux): hls2mp4dash --remux --remux-output=/media/event --tmpdir=/media/tscache http://example.com/live/event/master.m3u8"
    )

    parser.add_argument('playlist', metavar='PLAYLIST', help='Path to HLS playlist file. Can be a URI or local file.')
    parser.add_argument('--remux', dest='remux', action='store_true', default=False, help='download and remux TS segments to MP4 fragments')
    parser.add_argument('--remux-output', metavar='<ouput_dir>', dest='remux_output', default='.', help="Where remuxed fragments and MPD will be stored")
    parser.add_argument('--verbose', dest='verbose', action='store_true', default=False, help="Be verbose")
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help="Be extremely verbose")
    parser.add_argument('--tmpdir', dest='tmpdir', metavar='<tmp_dir>', default='/tmp/', help="Where to store all temporary files")
    parser.add_argument('--exec-dir', metavar='<exec_dir>', dest='exec_dir', default=default_exec_dir, help="Where Bento4 applications are found")
    parser.add_argument('--ffmpeg_exec-dir', metavar='<exec_dir>', dest='ffmpeg_exec_dir', default='', help="Where ffmpeg is found")
    args = parser.parse_args()
    global Options
    Options = args

    mpd = MPD_HLS(args.playlist, Options.remux, Options.remux_output)
    mpd.load()
    if Options.remux:
        outfile = open(Options.remux_output + "/manifest.mpd", "w")
        outfile.write(mpd.asXML())
        outfile.close()
    else:
        print(mpd.asXML())

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
