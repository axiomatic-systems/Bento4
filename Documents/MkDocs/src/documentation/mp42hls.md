# mp42hls
```
MP4 To HLS File Converter - Version 1.2
(Bento4 Version 1.6.0.0)
(c) 2002-2018 Axiomatic Systems, LLC

usage: mp42hls [options] <input>
Options:
  --verbose
  --show-info
  --hls-version <n> (default: 3)
  --pmt-pid <pid>
    PID to use for the PMT (default: 0x100)
  --audio-pid <pid>
    PID to use for audio packets (default: 0x101)
  --video-pid <pid>
    PID to use for video packets (default: 0x102)
  --audio-track-id <n>
    Read audio from track ID <n> (0 means no audio)
  --video-track-id <n>
    Read video from track ID <n> (0 means no video)
  --audio-format <format>
    Format to use for audio-only segments: 'ts' or 'packed' (default: 'ts')
  --segment-duration <n>
    Target segment duration in seconds (default: 6)
  --segment-duration-threshold <t>
    Segment duration threshold in milliseconds (default: 15)
  --pcr-offset <offset> in units of 90kHz (default 10000)
  --index-filename <filename>
    Filename to use for the playlist/index (default: stream.m3u8)
  --segment-filename-template <pattern>
    Filename pattern to use for the segments. Use a printf-style pattern with
    one number field for the segment number, unless using single file mode
    (default: segment-%d.<ext> for separate segment files, or stream.<ext> for single file)
  --segment-url-template <pattern>
    URL pattern to use for the segments. Use a printf-style pattern with
    one number field for the segment number unless unsing single file mode.
    (may be a relative or absolute URI).
    (default: segment-%d.<ext> for separate segment files, or stream.<ext> for single file)
  --iframe-index-filename <filename>
    Filename to use for the I-Frame playlist (default: iframes.m3u8 when HLS version >= 4)
  --output-single-file
    Output all the media in a single file instead of separate segment files.
    The segment filename template and segment URL template must be simple strings
    without '%d' or other printf-style patterns
  --encryption-mode <mode>
    Encryption mode (only used when --encryption-key is specified). AES-128 or SAMPLE-AES (default: AES-128)
  --encryption-key <key>
    Encryption key in hexadecimal (default: no encryption)
  --encryption-iv-mode <mode>
    Encryption IV mode: 'sequence', 'random' or 'fps' (FairPlay Streaming) (default: sequence)
    (when the mode is 'fps', the encryption key must be 32 bytes: 16 bytes for the key
    followed by 16 bytes for the IV).
  --encryption-key-uri <uri>
    Encryption key URI (may be a realtive or absolute URI). (default: key.bin)
  --encryption-key-format <format>
    Encryption key format. (default: 'identity')
  --encryption-key-format-versions <versions>
    Encryption key format versions.
  --encryption-key-line <ext-x-key-line>
    Preformatted encryption key line (only the portion after the #EXT-X-KEY: tag).
    This option can be used multiple times, once for each preformatted key line to be included in the playlist.
    (this option is mutually exclusive with the --encryption-key-uri, --encryption-key-format and --encryption-key-format-versions options)
    (the IV and METHOD parameters will automatically be added, so they must not appear in the <ext-x-key-line> argument)
```
