
### to be removed
import sys
sys.path += ['/Users/julien/Development/Bento4/Source/Python']

import bento4.core as bt4
import os.path
import unittest

BENTO4_TEST_DATA_DIR = os.path.join(os.path.dirname(__file__), '..', 'Data')

class CoreTester(unittest.TestCase):
    
    def setUp(self):
        self.file = bt4.File(os.path.join(BENTO4_TEST_DATA_DIR, 'test-001.mp4'))
        
    def test_moov_position(self):
        self.assertTrue(self.file.is_moov_before_mdat)
        
    def test_movie(self):
        self.failIfEqual(self.file.movie, None, "no movie in file")
        
    def test_tracks(self):
        tracks = self.file.movie.tracks
        known_values = {
            1: {'type': bt4.Track.TYPE_AUDIO,
                'handler_type': bt4.Track.HANDLER_TYPE_SOUN,
                'sample_count': 78,
                'durationms': 3600,
                'media_timescale': 22050},
            2: {'type': bt4.Track.TYPE_VIDEO,
                'handler_type': bt4.Track.HANDLER_TYPE_VIDE,
                'sample_count': 54,
                'durationms': 3600,
                'media_timescale': 30000},
            3: {'type': bt4.Track.TYPE_HINT,
                'handler_type': bt4.Track.HANDLER_TYPE_HINT,
                'sample_count': 54,
                'durationms': 3600,
                'media_timescale': 90000},
            4: {'type': bt4.Track.TYPE_HINT,
                'handler_type': bt4.Track.HANDLER_TYPE_HINT,
                'sample_count': 39,
                'durationms': 3600,
                'media_timescale': 22050}
        }
        for id in tracks:
            self.assertEquals(tracks[id].id, id)
            self.assertEquals(tracks[id].type, known_values[id]['type'])
            self.assertEquals(tracks[id].handler_type,
                              known_values[id]['handler_type'])
            self.assertEquals(tracks[id].media_duration[1],
                              known_values[id]['media_timescale'])
            self.assertEquals(tracks[id].sample_count,
                              known_values[id]['sample_count'])
            duration, timescale = tracks[id].duration
            self.assertEquals(duration*1000/timescale,
                              known_values[id]['durationms'])
        
if __name__ == '__main__':
    unittest.main()
        
    