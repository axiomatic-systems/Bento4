import bento4.core as bt4
import os.path as path
import unittest
from struct import pack

BENTO4_TEST_DATA_DIR = path.join(path.dirname(__file__), '..', 'Data')

class CoreTester(unittest.TestCase):
    
    def setUp(self):
        self.file = bt4.File(path.join(BENTO4_TEST_DATA_DIR, 'test-001.mp4'))

    def test_moov_position(self):
        self.assertTrue(self.file.moov_is_before_mdat)
        
    def test_atom_type_name(self):
        self.assertEquals('caca', bt4.atom_name(bt4.atom_type('caca')))
        self.assertNotEquals('zobi', bt4.atom_name(bt4.atom_type('fouf')))
        
    def test_filetype(self):
        major_brand, minor_version, compat_brands = self.file.type
        self.assertEquals(major_brand, bt4.File.FILE_BRAND_MP42)
        self.assertEquals(minor_version, 1)
        self.assertEquals(len(compat_brands), 2)
        self.assertEquals(compat_brands[0], bt4.File.FILE_BRAND_MP42)
        self.assertEquals(bt4.atom_name(compat_brands[1]), 'avc1')
        
    def test_movie(self):
        self.failIfEqual(self.file.movie, None, "no movie in file")
        
    def test_tracks(self):
        tracks = self.file.movie.tracks
        known_values = {
            1: {'type': bt4.Track.TYPE_AUDIO,
                'handler_type': bt4.Track.HANDLER_TYPE_SOUN,
                'sample_count': 78,
                'durationms': 3600,
                'media_timescale': 22050,
                'sample_descriptions': [
                    {'type': bt4.SampleDescription.TYPE_MPEG},
                ]},
            2: {'type': bt4.Track.TYPE_VIDEO,
                'handler_type': bt4.Track.HANDLER_TYPE_VIDE,
                'sample_count': 54,
                'durationms': 3600,
                'media_timescale': 30000,
                'sample_descriptions': [
                    {'type': bt4.SampleDescription.TYPE_AVC}
                ]},
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
        for id in known_values:
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
            if 'sample_descriptions' in known_values:
                known_sample_descs = known_values['sample_descriptions']
                for i in xrange(len(sample_descs)):
                    self.assertEquals(sample_descs['type'],
                                      tracks[id].sample_description(i).type)

    def test_avc_track(self):
        tracks = self.file.movie.tracks
        avc_track = tracks[2]
        test_filename = path.join(BENTO4_TEST_DATA_DIR, 'test-001.mp4.2')
        try:
            test_file = open(test_filename, 'rb')
            avc_data = ''
            for s in avc_track.sample_iterator():
                self.assertEquals(len(s.data), s.size)
                self.assertEquals(s.description_index, 0)
                avc_data += pack('>I', s.size)
                avc_data += s.data
            
            # test the sample data
            test_data = test_file.read()
            self.assertEquals(len(avc_data), len(test_data))
            self.assertEquals(avc_data, test_data,
                              'avc data mismatch')
            
            # test the sample description
            avc_desc = avc_track.sample_description(0)
            self.assertEquals(avc_desc.type, bt4.SampleDescription.TYPE_AVC)
            self.assertEquals(bt4.avc_profile_name(avc_desc.profile), "Main")
            self.assertEquals(avc_desc.profile_compatibility, 0x40)
            self.assertEquals(avc_desc.nalu_length_size, 4)
            self.assertEquals(avc_desc.width, 160)
            self.assertEquals(avc_desc.height, 120)
            self.assertEquals(avc_desc.depth, 24)
            
        finally:
            test_file.close()
                                       
        
if __name__ == '__main__':
    unittest.main()
        
    