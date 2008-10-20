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
        else:
            self.bt4file = lb4.AP4_File_Create(movie.bt4movie)
        
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
    
    def get_movie(self):
        bt4movie = lb4.AP4_File_GetMovie(self.bt4file)
        if bt4movie is None:
            return None
        else:
            return Movie(bt4movie)
        
    
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
        result = []
        count = lb4.AP4_Movie_GetTrackCount(self.bt4movie)
        for i in xrange(0, count.value):
            bt4track = lb4.AP4_Movie_GetTrackByIndex(self.bt4movie, Ap4Ordinal(i))
            result += [Track(bt4track)]
        return result
    
    @property
    def duration(self):
        return (lb4.AP4_Movie.GetDuration(self.bt4movie).value,
                lb4.AP4_Movie.GetTimeScale(self.bt4movie).value)
            
        
        
            
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
        
    

        
        
            
        