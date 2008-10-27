from bento4 import *
from bento4.errors import check_result
from ctypes import c_int, c_char_p, string_at
from struct import pack, unpack

class File(object):
    
    def __init__(self, name='', movie=None):
        if movie is None:
            if len(name) == 0:
                raise ValueError("name param cannot be empty")
            self.bt4stream = lb4.AP4_FileByteStream_Create(c_char_p(name),
                                                           c_int(0)) # read
            if bt4stream is None:
                raise IOError("Unable to open file %s" % name)
            self.bt4file = lb4.AP4_File_FromStream(bt4stream)
            self.movie = None
        else:
            self.bt4file = lb4.AP4_File_Create(movie.bt4movie)
            movie.bt4owner = False
            movie.file = self
            self.movie = movie
        
    def __del__(self):
        lb4.AP4_File_Destroy(self.bt4file)
        try:
            lb4.AP4_Stream_Release(self.bt4stream)
        except AttributeError:
            pass # depending on how the object was created,
                 # self.bt4stream may or may not exist

    
    def is_moov_before_mdat(self):
        c = lb4.AP4_File_IsMoovBeforeMdat(self.bt4file)
        return True if c.value !=0 else False
    
    @property
    def movie(self):
        if self.movie:
            return self.movie
        
        bt4movie = lb4.AP4_File_GetMovie(self.bt4file)
        if bt4movie is None:
            return None
        else:
            self.movie = Movie(bt4movie)
            self.movie.file = file # add a reference here for ref counting
            return self.movie
        
    
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
        for i in xrange(0, count.value):
            bt4track = lb4.AP4_Movie_GetTrackByIndex(self.bt4movie, Ap4Ordinal(i))
            track = Track(bt4track)
            track.movie = self # add a reference here for ref counting
            result[track.id] = track
        return result
    
    @property
    def duration(self):
        return (lb4.AP4_Movie.GetDuration(self.bt4movie),
                lb4.AP4_Movie.GetTimeScale(self.bt4movie))
    
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
    
    HANDLER_TYPE_SOUN = unpack('>I', pack('>4s', 'soun'))
    HANDLER_TYPE_VIDE = unpack('>I', pack('>4s', 'vide'))
    HANDLER_TYPE_HINT = unpack('>I', pack('>4s', 'hint'))
    HANDLER_TYPE_MDIR = unpack('>I', pack('>4s', 'mdir'))
    HANDLER_TYPE_TEXT = unpack('>I', pack('>4s', 'text'))
    HANDLER_TYPE_TX3G = unpack('>I', pack('>4s', 'tx3g'))
    HANDLER_TYPE_JPEG = unpack('>I', pack('>4s', 'jpeg'))
    HANDLER_TYPE_ODSM = unpack('>I', pack('>4s', 'odsm'))
    HANDLER_TYPE_SDSM = unpack('>I', pack('>4s', 'sdsm'))
    
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
    
    def get_sample(self, index):
        if index<0 or index>=self.sample_count:
            raise IndexError()
        bt4sample = lb4.AP4_Sample_CreateEmpty()
        f = AP4_Track_GetSample
        f.restype = check_result
        try:
            f(self.bt4track, bt4sample)
        except Exception, e:
            lb4.AP4_Sample_Destroy(bt4sample) # prevents a leak
            raise e
        return Sample(bt4sample=bt4sample)
        
    
    def sample_iterator(self, start=0, end=-1):
        current = start
        end = end if end != -1 else self.sample_count
        while current<end:
            yield self.get_sample(current)
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
        raise NotImplementedError()
    
    
class Sample(object):
    def __init__(self, data_stream=None, offset=0, size=0, desc_index=0,
                 dts=0, cts_offset=0, is_sync=False, bt4sample=None):
        if bt4sample is None:
            raise NotImplementedError()
            pass
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
            f(self.bt4sample, self.bt4buffer)            
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
    def sample_description_index(self):
        return lb4.AP4_Sample_GetSampleDescriptionIndex(self,bt4sample)
    
    @property
    def dts(self):
        return lb4.AP4_Sample_GetDts(self.bt4sample)
    
    @property
    def cts(self):
        return lb4.AP4_Sample_GetCts(self.bt4sample)
    
    @property
    def is_sync(self):
        v = lb4.AP4_Sample_GetIsSync(self.bt4sample)
        return False if v==0 else True
    
    
            

class SampleDescription(object):
    pass
    

        
        
            
        