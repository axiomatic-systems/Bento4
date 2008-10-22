from bento4 import *
from ctypes import c_int, c_char_p

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
        return (lb4.AP4_Movie.GetDuration(self.bt4movie).value,
                lb4.AP4_Movie.GetTimeScale(self.bt4movie).value)
    
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
    
    def __init__(self, type=TYPE_UNKNOWN, sample_table=None, id=0,
                 movie_duration=(), media_duration=(), language='',
                 dimensions=(), bt4track=None):
        """durations are tuple: (duration, timescale)"""
        self.bt4owner = False
        if bt4track is None:
            self.bt4owner = True
            self.bt4track = lb4.AP4_Track_Create(c_int(type),
                                                 sample_table.bt4table,
                                                 Ap4UI32(id),
                                                 Ap4UI32(movie_duration[1]),
                                                 Ap4UI32(movie_duration[0]),
                                                 Ap4UI32(media_duration[1]),
                                                 Ap4UI32(media_duration[0]),
                                                 c_char_p(language),
                                                 Ap4UI32(width),
                                                 Ap4UI32(height))
            sample_table.bt4owner = False # change the ownership
        else:
            self.bt4track = bt4track
        
    def __del__(self):
        if self.bt4owner:
            lb4.AP4_Track_Destroy(self.bt4track)
    
    @property
    def type(self):
        return lb4.AP4_Track_GetType(self.bt4track).value
    
    @property
    def handler_type(self):
        return lb4.AP4_Track_GetHandlerType(self.bt4track).value
    
    @property
    def id(self):
        return lb4.AP4_Track_GetId(self.bt4track).value
    
    @property
    def media_duration(self):
        return (lb4.AP4_Track_GetMediaDuration(self.bt4track).value,
                lb4.AP4_Track_GetMediaTimeScale(self.bt4track).value)
    
    @property
    def duration(self):
        """returns just the duration in the timescale of the movie"""
        return lb4.AP4_Track_GetDuration(self.bt4track)
    
    @property
    def language(self):
        return lb4.AP4_Track_GetLanguage(self.bt4track).value
    
    def set_movie_timescale(self, timescale):
        
    
    
    

        
        
            
        