from unittest.mock import patch
import sys
import os
import importlib
mp4dash = importlib.import_module("mp4-dash")

BENTO4_HOME = os.environ['BENTO4_HOME']
TEST_OUTPUT_ROOT = os.path.join(BENTO4_HOME, "Test/Output/mp4dash")
VIDEO_H264_001_MP4 = os.path.join(BENTO4_HOME, "Test/Data/video-h264-001.mp4")
VIDEO_H264_002_MP4 = os.path.join(BENTO4_HOME, "Test/Data/video-h264-002.mp4")
AUDIO_AAC_001_MP4 = os.path.join(BENTO4_HOME, "Test/Data/audio-aac-001.mp4")
AUDIO_AAC_002_MP4 = os.path.join(BENTO4_HOME, "Test/Data/audio-aac-002.mp4")
AUDIO_AAC_003_MP4 = os.path.join(BENTO4_HOME, "Test/Data/audio-aac-003.mp4")

def run_mp4dash(extra_args, subdir, input_files):
    args = ["mp4dash"] + extra_args + ["-f", "-o", os.path.join(TEST_OUTPUT_ROOT, subdir)] + input_files
    with patch.object(sys, 'argv', args):
        mp4dash.main()

def test_mp4dash_001():
    run_mp4dash([], "001", [VIDEO_H264_002_MP4])

def test_mp4dash_002():
    run_mp4dash(["-v"], "002", [VIDEO_H264_002_MP4])

def test_mp4dash_003():
    run_mp4dash(["-d"], "003", [VIDEO_H264_002_MP4])

def test_mp4dash_004():
    run_mp4dash(["-d", "--hls"], "004", [VIDEO_H264_002_MP4])

def test_mp4dash_005():
    run_mp4dash(["-d", "--profiles", "on-demand"], "005", [VIDEO_H264_002_MP4])

def test_mp4dash_006():
    run_mp4dash(["-d", "--use-segment-list"], "006", [VIDEO_H264_002_MP4])

def test_mp4dash_007():
    run_mp4dash(["-d", "--use-segment-list", "--no-split"], "007", [VIDEO_H264_002_MP4])

def test_mp4dash_008():
    run_mp4dash(["-d", "--use-segment-timeline"], "008", [VIDEO_H264_002_MP4])

def test_mp4dash_009():
    run_mp4dash(["-v", "--smooth"], "009", [VIDEO_H264_002_MP4])

def test_mp4dash_010():
    run_mp4dash(["-v", "--hippo"], "010", [VIDEO_H264_002_MP4])

def test_mp4dash_011():
    run_mp4dash([
        "--hls",
        "--encryption-key=000102030405060708090a0b0c0d0e0f:00112233445566778899aabbccddeeff:a0a1a2a3a4a5a6a7a8a9aaabacadaeaf",
        "--encryption-cenc-scheme=cbcs",
        "--clearkey",
        "--clearkey-license-uri=http://foo.com/bar.json",
        "--eme-signaling=pssh-v1",
        "--marlin",
        "--marlin-add-pssh",
        "--widevine",
        "--widevine-header=provider:c1c2c3",
        "--playready",
        "--playready-version=4.3",
        "--playready-header=LA_URL:http://foo.com/123",
        "--primetime",
        "--fairplay-key-uri=skd://foobar"],
        "011",
        [VIDEO_H264_002_MP4])

def test_mp4dash_012():
    run_mp4dash([
        "--encryption-key=000102030405060708090a0b0c0d0e0f:00112233445566778899aabbccddeeff:a0a1a2a3a4a5a6a7a8a9aaabacadaeaf",
        "--playready",
        "--playready-version=4.0"],
        "012",
        [VIDEO_H264_002_MP4])

def test_mp4dash_013():
    run_mp4dash([
        "--encryption-key=000102030405060708090a0b0c0d0e0f:00112233445566778899aabbccddeeff:a0a1a2a3a4a5a6a7a8a9aaabacadaeaf",
        "--playready",
        "--playready-version=4.1"],
        "013",
        [VIDEO_H264_002_MP4])

def test_mp4dash_014():
    run_mp4dash([
        "--encryption-key=000102030405060708090a0b0c0d0e0f:00112233445566778899aabbccddeeff:a0a1a2a3a4a5a6a7a8a9aaabacadaeaf",
        "--playready",
        "--playready-version=4.2"],
        "014",
        [VIDEO_H264_002_MP4])

def test_mp4dash_015():
    run_mp4dash(["-d", "--profiles", "on-demand", "--hls"], "015", [VIDEO_H264_002_MP4])

def test_mp4dash_016():
    run_mp4dash(["-d", "--subtitles"], "016", [VIDEO_H264_002_MP4])

def test_mp4dash_017():
    run_mp4dash(["-d", "--hls"], "017", [AUDIO_AAC_002_MP4])

def test_mp4dash_018():
    run_mp4dash(["-d", "--hls"], "018", [VIDEO_H264_002_MP4, AUDIO_AAC_002_MP4, AUDIO_AAC_003_MP4])

def test_mp4dash_019():
    run_mp4dash([
        "--encryption-key=000102030405060708090a0b0c0d0e0f:00112233445566778899aabbccddeeff:random",
        "--encryption-cenc-scheme=cbcs",
        "--hls"],
        "019",
        [VIDEO_H264_002_MP4])

def test_mp4dash_020():
    run_mp4dash([
        "-v",
        "--encryption-key=000102030405060708090a0b0c0d0e0f:#abcdabcdabcdabcdabcdabcdabcdabcdabcdabcd:a0a1a2a3a4a5a6a7a8a9aaabacadaeaf",
        "--playready"],
        "020",
        [VIDEO_H264_002_MP4])
