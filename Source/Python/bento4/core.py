from bento4 import *
from bento4.errors import check_result
from ctypes import c_int, c_char_p, string_at
from struct import pack, unpack

def atom_type(name):
    return unpack('>I', pack('>4s', name))[0]

def atom_name(type):
    return unpack('>4s', pack('>I', type))[0]
    
class File(object):
    FILE_BRAND_QT__ = atom_type('qt  ')
    FILE_BRAND_ISOM = atom_type('isom')
    FILE_BRAND_MP41 = atom_type('mp41')
    FILE_BRAND_MP42 = atom_type('mp42')
    FILE_BRAND_3GP1 = atom_type('3gp1')
    FILE_BRAND_3GP2 = atom_type('3gp2')
    FILE_BRAND_3GP3 = atom_type('3gp3')
    FILE_BRAND_3GP4 = atom_type('3gp4')
    FILE_BRAND_3GP5 = atom_type('3gp5')
    FILE_BRAND_3G2A = atom_type('3g2a')
    FILE_BRAND_MMP4 = atom_type('mmp4')
    FILE_BRAND_M4A_ = atom_type('M4A ')
    FILE_BRAND_M4P_ = atom_type('M4P ')
    FILE_BRAND_MJP2 = atom_type('mjp2')
    
    def __init__(self, name='', movie=None):
        self.moov = movie # can't have self.movie because movie is a property
        if movie is None:
            if len(name) == 0:
                raise ValueError("name param cannot be empty")
            result = Ap4Result()
            self.bt4stream = lb4.AP4_FileByteStream_Create(c_char_p(name),
                                                           c_int(0), # read
                                                           byref(result))
            check_result(result.value)
            self.bt4file = lb4.AP4_File_FromStream(self.bt4stream, 0)
        else:
            self.bt4file = lb4.AP4_File_Create(movie.bt4movie)
            movie.bt4owner = False
            movie.file = self
        
    def __del__(self):
        lb4.AP4_File_Destroy(self.bt4file)
        try:
            lb4.AP4_Stream_Release(self.bt4stream)
        except AttributeError:
            pass # depending on how the object was created,
                 # self.bt4stream may or may not exist
                 
    def inspect(self, inspector):
        f = lb4.AP4_File_Inspect
        f.restype = check_result
        f(self.bt4file, inspector.bt4inspector)

    @property
    def moov_is_before_mdat(self):
        c = lb4.AP4_File_IsMoovBeforeMdat(self.bt4file)
        return True if c !=0 else False
    
    @property
    def movie(self):
        if self.moov:
            return self.moov
        
        bt4movie = lb4.AP4_File_GetMovie(self.bt4file)
        if bt4movie is None:
            return None
        else:
            self.moov = Movie(bt4movie=bt4movie)
            self.moov.file = file # add a reference here for ref counting
            return self.moov
    
    def get_type(self):
        """returns a tuple (major_brand, minor_version, [compatible_brands])"""
        major_brand = Ap4UI32()
        minor_version = Ap4UI32()
        compat = Ap4UI32()
        compat_count = Ap4UI32()
        
        # get the file type
        f = lb4.AP4_File_GetFileType
        f.restype = check_result
        f(self.bt4file, byref(major_brand),
          byref(minor_version), byref(compat_count))
        
        # get the compatible brands
        f = lb4.AP4_File_GetCompatibleBrand
        f.restype = check_result
        compat_brands = []
        for i in xrange(compat_count.value):
            f(self.bt4file, i, byref(compat))
            compat_brands += [compat.value]
        return (major_brand.value, minor_version.value, compat_brands)
    
    def set_type(self, value):
        """value: a tuple (major_brand, minor_version, [compatible_brands])"""
        major_brand, minor_version, compat_brands = value
        compat_count = len(compat_brands)
        compat_brand_array = Ap4UI32*compat_count
        f = lb4.AP4_File_SetFileType
        f.restype = check_result
        f(self.bt4file, major_brand, minor_version,
          compat_brand_array(*compat_brands), compat_count)
    
    type = property(get_type, set_type)
        
    
class Movie(object):
    
    def __init__(self, timescale=0, bt4movie=None):
        self.bt4owner = False
        if bt4movie is None:
            self.bt4movie = lb4.AP4_Movie_Create(Ap4UI32(timescale))
        else:
            self.bt4movie = bt4movie
            
    def __del__(self):
        if self.bt4owner:
            lb4.AP4_Movie_Destroy(self.bt4movie)
        
    @property    
    def tracks(self):
        result = {}
        count = lb4.AP4_Movie_GetTrackCount(self.bt4movie)
        for i in xrange(count):
            bt4track = lb4.AP4_Movie_GetTrackByIndex(self.bt4movie, Ap4Ordinal(i))
            track = Track(bt4track=bt4track)
            track.movie = self # add a reference here for ref counting
            result[track.id] = track
        return result
    
    @property
    def duration(self):
        return (lb4.AP4_Movie_GetDuration(self.bt4movie),
                lb4.AP4_Movie_GetTimeScale(self.bt4movie))
    
    def add_track(self, track):
        result = lb4.AP4_Movie_AddTrack(self.bt4movie. track.bt4track)
        if result.value != 0:
            raise RuntimeError("Track insertion failed with error %d" %
                               result.value)
        track.bt4owner = False # change ownership
        track.movie = self 
                      
            
class Track(object):
    TYPE_UNKNOWN = 0
    TYPE_AUDIO   = 1
    TYPE_VIDEO   = 2
    TYPE_SYSTEM  = 3
    TYPE_HINT    = 4
    TYPE_TEXT    = 5
    TYPE_JPEG    = 6
    TYPE_RTP     = 7
    
    HANDLER_TYPE_SOUN = atom_type('soun')
    HANDLER_TYPE_VIDE = atom_type('vide')
    HANDLER_TYPE_HINT = atom_type('hint')
    HANDLER_TYPE_MDIR = atom_type('mdir')
    HANDLER_TYPE_TEXT = atom_type('text')
    HANDLER_TYPE_TX3G = atom_type('tx3g')
    HANDLER_TYPE_JPEG = atom_type('jpeg')
    HANDLER_TYPE_ODSM = atom_type('odsm')
    HANDLER_TYPE_SDSM = atom_type('sdsm')
    
    def __init__(self, type=TYPE_UNKNOWN, sample_table=None, id=0,
                 track_duration=(), media_duration=(), language='',
                 dimensions=(), bt4track=None):
        """durations are tuple: (duration, timescale)
           track_duration is in the timescale of the movie"""
        self.bt4owner = False
        self.movie_timescale = -1 # invalid on purpose
        if bt4track is None:
            self.bt4owner = True
            self.bt4track = lb4.AP4_Track_Create(c_int(type),
                                                 sample_table.bt4table,
                                                 Ap4UI32(id),
                                                 Ap4UI32(track_duration[1]),
                                                 Ap4UI32(track_duration[0]),
                                                 Ap4UI32(media_duration[1]),
                                                 Ap4UI32(media_duration[0]),
                                                 c_char_p(language),
                                                 Ap4UI32(width),
                                                 Ap4UI32(height))
            self.movie_timescale = track_duration[1]
            sample_table.bt4owner = False # change the ownership
        else:
            self.bt4track = bt4track
        
    def __del__(self):
        if self.bt4owner:
            lb4.AP4_Track_Destroy(self.bt4track)
    
    @property
    def type(self):
        return lb4.AP4_Track_GetType(self.bt4track)
    
    @property
    def handler_type(self):
        return lb4.AP4_Track_GetHandlerType(self.bt4track)
    
    @property
    def id(self):
        return lb4.AP4_Track_GetId(self.bt4track)
    
    @property
    def media_duration(self):
        return (lb4.AP4_Track_GetMediaDuration(self.bt4track),
                lb4.AP4_Track_GetMediaTimeScale(self.bt4track))
    
    @property
    def duration(self):
        if self.movie_timescale == -1:
            # get it from the movie
            self.movie_timescale = self.movie.duration[1]
        return (lb4.AP4_Track_GetDuration(self.bt4track), self.movie_timescale)
    
    @property
    def language(self):
        f = lb4.AP4_Track_GetLanguage
        f.restype = c_char_p
        return f(self.bt4track)
    
    @property
    def sample_count(self):
        return lb4.AP4_Track_GetSampleCount(self.bt4track)
    
    def sample(self, index):
        if index<0 or index>=self.sample_count:
            raise IndexError()
        bt4sample = lb4.AP4_Sample_CreateEmpty()
        f = lb4.AP4_Track_GetSample
        f.restype = check_result
        try:
            f(self.bt4track, index, bt4sample)
        except Exception, e:
            lb4.AP4_Sample_Destroy(bt4sample) # prevents a leak
            raise e
        return Sample(bt4sample=bt4sample)
        
    
    def sample_iterator(self, start=0, end=-1):
        current = start
        end = end if end != -1 else self.sample_count
        while current<end:
            yield self.sample(current)
            current += 1
    
    def set_movie_timescale(self, timescale):
        f = lb4.AP4_Track_SetMovieTimeScale
        f.restype = check_result
        f(self.bt4track, Ap4UI32(timescale))
        
    def sample_index_for_timestamp_ms(self, ts):
        result = Ap4Ordinal()
        f = lb4.AP4_Track_GetSampleIndexForTimeStampMs
        f.restype = check_result
        f(self.bt4track, Ap4TimeStamp(ts), byref(result))
        return result.value

    def sample_description(self, index):
        bt4desc = lb4.AP4_Track_GetSampleDescription(self.bt4track,
                                                           Ap4Ordinal(index))
        if bt4desc == 0:
            raise IndexError()
        sampledesc_type = lb4.AP4_SampleDescription_GetType(bt4desc)
        track_type = self.type
        if sampledesc_type == SampleDescription.TYPE_AVC:
            result = AvcSampleDescription(bt4desc=bt4desc)
        elif sampledesc_type == SampleDescription.TYPE_MPEG:
            if track_type == Track.TYPE_AUDIO:
                result = MpegAudioSampleDescription(bt4desc=bt4desc)
            elif track_type == Track.TYPE_VIDEO:
                result = MpegVideoSampleDescription(bt4desc=bt4desc)
            elif track_type == Track.TYPE_MPEG:
                result = MpegSystemSampleDescription(bt4desc=bt4desc)
            else:
                result = MpegSampleDescription(bt4desc=bt4desc)
        else:
            if track_type == Track.TYPE_AUDIO:
                result = GenericAudioSampleDescription(bt4desc=bt4desc)
            elif track_type == Track.TYPE_VIDEO:
                result = GenericVideoSampleDescription(bt4desc=bt4desc)
            else:
                result = SampleDescription(bt4desc)
        result.track = self # add a reference
        return result
            
    
    
class Sample(object):
    def __init__(self, data_stream=None, offset=0, size=0, desc_index=0,
                 dts=0, cts_offset=0, is_sync=False, bt4sample=None):
        if bt4sample is None:
            raise NotImplementedError()
        else:
            self.bt4sample = bt4sample
            
    def __del__(self):
        # always the owner of the sample
        lb4.AP4_Sample_Destroy(self.bt4sample)
        
    @property
    def data(self):
        bt4buffer = lb4.AP4_DataBuffer_Create(self.size)
        f = lb4.AP4_Sample_ReadData
        f.restype = check_result
        try:
            f(self.bt4sample, bt4buffer)            
            return string_at(lb4.AP4_DataBuffer_GetData(bt4buffer),
                             lb4.AP4_DataBuffer_GetDataSize(bt4buffer))
        except Exception, e:
            raise e
        finally:
            lb4.AP4_DataBuffer_Destroy(bt4buffer) # prevents a leak        
    
    @property
    def offset(self):
        return lb4.AP4_Sample_GetOffset(self,bt4sample)
    
    @property
    def size(self):
        return lb4.AP4_Sample_GetSize(self.bt4sample)
    
    @property
    def description_index(self):
        return lb4.AP4_Sample_GetDescriptionIndex(self.bt4sample)
    
    @property
    def dts(self):
        return lb4.AP4_Sample_GetDts(self.bt4sample)
    
    @property
    def cts(self):
        return lb4.AP4_Sample_GetCts(self.bt4sample)
    
    @property
    def is_sync(self):
        v = lb4.AP4_Sample_IsSync(self.bt4sample)
        return False if v==0 else True
    
    
            

class SampleDescription(object):
    TYPE_UNKNOWN    = 0
    TYPE_MPEG       = 1
    TYPE_PROTECTED  = 2
    TYPE_AVC        = 3
    
    def __init__(self, bt4desc=None, bt4owner=False, **kwargs):
        self.bt4desc = bt4desc
        self.bt4owner = bt4owner
        super(SampleDescription, self).__init__(**kwargs)

    def __del__(self):
        if self.bt4owner:
            lb4.AP4_SampleDescription_Destroy(self.bt4desc)
        
    @property
    def type(self):
        return lb4.AP4_SampleDescription_GetType(self.bt4desc)
    
    @property
    def format(self):
        return lb4.AP4_SampleDescription_GetFormat(self.bt4desc)
    
            
class AudioSampleDescription(object):
    """mixin class"""
    
    def __init__(self, bt4audiodesc, **kwargs):
        self.bt4audiodesc = bt4audiodesc
        super(AudioSampleDescription, self).__init__(**kwargs)
    
    @property
    def sample_rate(self):
        return lb4.AP4_AudioSampleDescription_GetSampleRate(self.bt4audiodesc)
    
    @property
    def sample_size(self):
        return lb4.AP4_AudioSampleDescription_GetSampleSize(self.bt4audiodesc)
    
    @property
    def channel_count(self):
        return lb4.AP4_AudioSampleDescription_GetChannelCount(self.bt4audiodesc)
    

class VideoSampleDescription(object):
    """mixin class"""
    
    def __init__(self, bt4videodesc, **kwargs):
        self.bt4videodesc = bt4videodesc
        super(VideoSampleDescription, self).__init__(**kwargs)
    
    @property
    def width(self):
        return lb4.AP4_VideoSampleDescription_GetWidth(self.bt4videodesc)
     
    @property
    def height(self):
        return lb4.AP4_VideoSampleDescription_GetHeight(self.bt4videodesc)
    
    @property
    def depth(self):
        return lb4.AP4_VideoSampleDescription_GetDepth(self.bt4videodesc)
     
    @property
    def compressor_name(self):
        f = lb4.AP4_VideoSampleDescription_GetCompressorName
        f.restype = c_char_p
        return f(self.bt4videodesc)
 

class MpegSampleDescription(SampleDescription):
    STREAM_TYPE_FORBIDDEN = 0x00
    STREAM_TYPE_OD        = 0x01
    STREAM_TYPE_CR        = 0x02	
    STREAM_TYPE_BIFS      = 0x03
    STREAM_TYPE_VISUAL    = 0x04
    STREAM_TYPE_AUDIO     = 0x05
    STREAM_TYPE_MPEG7     = 0x06
    STREAM_TYPE_IPMP      = 0x07
    STREAM_TYPE_OCI       = 0x08
    STREAM_TYPE_MPEGJ     = 0x09
    STREAM_TYPE_TEXT      = 0x0D

    OTI_MPEG4_SYSTEM         = 0x01
    OTI_MPEG4_SYSTEM_COR     = 0x02
    OTI_MPEG4_TEXT           = 0x08
    OTI_MPEG4_VISUAL         = 0x20
    OTI_MPEG4_AUDIO          = 0x40
    OTI_MPEG2_VISUAL_SIMPLE  = 0x60
    OTI_MPEG2_VISUAL_MAIN    = 0x61
    OTI_MPEG2_VISUAL_SNR     = 0x62
    OTI_MPEG2_VISUAL_SPATIAL = 0x63
    OTI_MPEG2_VISUAL_HIGH    = 0x64
    OTI_MPEG2_VISUAL_422     = 0x65
    OTI_MPEG2_AAC_AUDIO_MAIN = 0x66
    OTI_MPEG2_AAC_AUDIO_LC   = 0x67
    OTI_MPEG2_AAC_AUDIO_SSRP = 0x68
    OTI_MPEG2_PART3_AUDIO    = 0x69
    OTI_MPEG1_VISUAL         = 0x6A
    OTI_MPEG1_AUDIO          = 0x6B
    OTI_JPEG                 = 0x6C
    
    def __init__(self, bt4mpegdesc, **kwargs):
        self.bt4mpegdesc = bt4mpegdesc
        super(MpegSampleDescription, self).__init__(**kwargs)
   
    @property
    def stream_type(self):
        return lb4.AP4_MpegSampleDescription_GetStreamType(self.bt4mpegdesc)
    
    @property
    def object_type_id(self):
        return lb4.AP4_MpegSampleDescription_GetObjectTypeId(self.bt4mpegdesc)
    
    @property
    def buffer_size(self):
        return lb4.AP4_MpegSampleDescription_GetBufferSize(self.bt4mpegdesc)
    
    @property
    def max_bitrate(self):
        return lb4.AP4_MpegSampleDescription_GetMaxBitrate(self.bt4mpegdesc)
    
    @property
    def avg_bitrate(self):
        return lb4.AP4_MpegSampleDescription_GetAvgBitrate(self.bt4mpegdesc)
    
    @property
    def decoder_info(self):
        bt4buf = lb4.AP4_MpegSampleDescription_GetDecoderInfo(self.bt4mpegdesc)
        return string_at(lb4.AP4_DataBuffer_GetData(bt4buf),
                         lb4.AP4_DataBuffer_GetDataSize(bt4buf))


class GenericAudioSampleDescription(SampleDescription,
                                    AudioSampleDescription):
    
    def __init__(self, bt4desc):
        bt4audiodesc = lb4.AP4_SampleDescription_AsAudio(bt4desc)
        super(GenericAudioSampleDescription, self).__init__(bt4desc=bt4desc,
                                                            bt4audiodesc=bt4audiodesc)
    
    
class GenericVideoSampleDescription(SampleDescription,
                                    VideoSampleDescription):
    
    def __init__(self, bt4desc):
        bt4videodesc = lb4.AP4_SampleDescription_AsVideo(bt4desc)
        super(GenericVideoSampleDescription, self).__init__(bt4desc=bt4desc,
                                                            bt4videodesc=bt4videodesc)
        
        
def avc_profile_name(profile):
    f = lb4.AP4_AvcSampleDescription_GetProfileName
    f.restype = c_char_p
    return f(profile)
        
        
class AvcSampleDescription(SampleDescription, VideoSampleDescription):
    
    def __init__(self, bt4desc):
        bt4videodesc = lb4.AP4_SampleDescription_AsVideo(bt4desc)
        super(AvcSampleDescription, self).__init__(bt4desc=bt4desc,
                                                   bt4videodesc=bt4videodesc)
        self.bt4avcdesc = lb4.AP4_SampleDescription_AsAvc(bt4desc)
        
    @property
    def config_version(self):
        return lb4.AP4_AvcSampleDescription_GetConfigurationVersion(self.bt4avcdesc)
    
    @property
    def profile(self):
        return lb4.AP4_AvcSampleDescription_GetProfile(self.bt4avcdesc)
    
    @property
    def level(self):
        return lb4.AP4_AvcSampleDescription_GetLevel(self.bt4avcdesc)

    @property
    def profile_compatibility(self):
        return lb4.AP4_AvcSampleDescription_GetProfileCompatibility(self.bt4avcdesc)

    @property
    def nalu_length_size(self):
        return lb4.AP4_AvcSampleDescription_GetNaluLengthSize(self.bt4avcdesc)
    
    @property
    def sequence_params(self):
        result = []
        count = lb4.AP4_AvcSampleDescription_GetSequenceParameterCount(self.bt4avcdesc)
        for i in xrange(count):
            bt4buf = lb4.AP4_AvcSampleDescription_GetSequenceParameter(self.bt4avcdesc, i)
            result += [string_at(lb4.AP4_DataBuffer_GetData(bt4buf),
                                 lb4.AP4_DataBuffer_GetDataSize(bt4buf))]
        return result
    
    @property
    def picture_params(self):
        result = []
        count = lb4.AP4_AvcSampleDescription_GetPictureParametersCount(self.bt4avcdesc)
        for i in xrange(count):
            bt4buf = lb4.AP4_AvcSampleDescription_GetPictureParameter(self.bt4avcdesc, i)
            result += [string_at(AP4_DataBuffer_GetData(bt4buf),
                                 AP4_DataBuffer_GetDataSize(bt4buf))]
        return result
         

class MpegSystemSampleDescription(MpegSampleDescription):
    
    def __init__(self, bt4desc):
        bt4mpegdesc = lb4.AP4_SampleDescription_AsMpeg(bt4desc)
        super(MpegSystemSampleDescription, self).__init__(bt4desc=bt4desc,
                                                          bt4mpegdesc=bt4mpegdesc)
        
        
def mpeg_audio_object_type_string(type):
    f = lb4.AP4_MpegAudioSampleDescription_GetMpegAudioObjectTypeString
    f.restype = c_char_p
    return f(type)

class MpegAudioSampleDescription(MpegSampleDescription,
                                 AudioSampleDescription) :
    
    MPEG4_AUDIO_OBJECT_TYPE_AAC_MAIN        = 1  #  AAC Main Profile              
    MPEG4_AUDIO_OBJECT_TYPE_AAC_LC          = 2  #  AAC Low Complexity            
    MPEG4_AUDIO_OBJECT_TYPE_AAC_SSR         = 3  #  AAC Scalable Sample Rate      
    MPEG4_AUDIO_OBJECT_TYPE_AAC_LTP         = 4  #  AAC Long Term Predictor       
    MPEG4_AUDIO_OBJECT_TYPE_SBR             = 5  #  Spectral Band Replication          
    MPEG4_AUDIO_OBJECT_TYPE_AAC_SCALABLE    = 6  #  AAC Scalable                       
    MPEG4_AUDIO_OBJECT_TYPE_TWINVQ          = 7  #  Twin VQ                            
    MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LC       = 17 #  Error Resilient AAC Low Complexity 
    MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LTP      = 19 #  Error Resilient AAC Long Term Prediction 
    MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_SCALABLE = 20 #  Error Resilient AAC Scalable 
    MPEG4_AUDIO_OBJECT_TYPE_ER_TWINVQ       = 21 #  Error Resilient Twin VQ 
    MPEG4_AUDIO_OBJECT_TYPE_ER_BSAC         = 22 #  Error Resilient Bit Sliced Arithmetic Coding 
    MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LD       = 23 #  Error Resilient AAC Low Delay 
    MPEG4_AUDIO_OBJECT_TYPE_LAYER_1         = 32 #  MPEG Layer 1 
    MPEG4_AUDIO_OBJECT_TYPE_LAYER_2         = 33 #  MPEG Layer 2 
    MPEG4_AUDIO_OBJECT_TYPE_LAYER_3         = 34 #  MPEG Layer 3 

    def __init__(self, bt4desc):
        bt4audiodesc = lb4.AP4_SampleDescription_AsAudio(bt4desc)
        bt4mpegdesc = lb4.AP4_SampleDescription_AsMpeg(bt4desc)
        super(MpegAudioSampleDescription, self).__init__(bt4desc=bt4desc,
                                                         bt4audiodesc=bt4audiodesc,
                                                         bt4mpegdesc=bt4mpegdesc)
        self.bt4mpegaudiodesc = lb4.AP4_SampleDescription_AsMpegAudio(bt4desc)
    
    @property    
    def mpeg4_audio_object_type(self):
        return lb4.AP4_MpegAudioSampleDescription_GetMpeg4AudioObjecType(self.bt4mpegaudiodesc)
        

class MpegVideoSampleDescription(MpegSampleDescription,
                                 VideoSampleDescription):
    
    def __init__(self, bt4desc):
        bt4videodesc = lb4.AP4_SampleDescription_AsVideo(bt4desc)
        bt4mpegdesc = lb4.AP4_SampleDescription_AsMpeg(bt4desc)
        super(MpegVideoSampleDescription, self).__init__(bt4desc=bt4desc,
                                                         bt4mpegdesc=bt4mpegdesc,
                                                         bt4videodesc=bt4videodesc)


        
            
        