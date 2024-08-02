# mp4hls
```
Usage: mp4-hls.py [options] <media-file> [<media-file> ...]

Each <media-file> is the path to an MP4 file, optionally prefixed with a
stream selector delimited by [ and ]. The same input MP4 file may be repeated,
provided that the stream selector prefixes select different streams. Version
1.2.0 r637

Options:
  -h, --help            show this help message and exit
  -v, --verbose         Be verbose
  -d, --debug           Print out debugging information
  -o <output-dir>, --output-dir=<output-dir>
                        Output directory
  -f, --force           Allow output to an existing directory
  --hls-version=<version>
                        HLS Version (default: 4)
  --master-playlist-name=<filename>
                        Master Playlist name
  --media-playlist-name=<name>
                        Media Playlist name
  --iframe-playlist-name=<name>
                        I-frame Playlist name
  --output-single-file  Store segment data in a single output file per input
                        file
  --audio-format=AUDIO_FORMAT
                        Format for audio segments (packed or ts) (default:
                        packed)
  --segment-duration=SEGMENT_DURATION
                        Segment duration (default: 6)
  --encryption-mode=<mode>
                        Encryption mode (only used when --encryption-key is
                        specified). AES-128 or SAMPLE-AES (default: AES-128)
  --encryption-key=<key>
                        Encryption key in hexadecimal (default: no encryption)
  --encryption-iv-mode=<mode>
                        Encryption IV mode: 'sequence', 'random' or 'fps'
                        (Fairplay Streaming) (default: sequence). When the
                        mode is 'fps', the encryption key must be 32 bytes: 16
                        bytes for the key followed by 16 bytes for the IV.
  --encryption-key-uri=<uri>
                        Encryption key URI (may be a relative or absolute
                        URI). (default: key.bin)
  --encryption-key-format=<format>
                        Encryption key format. (default: 'identity')
  --encryption-key-format-versions=<versions>
                        Encryption key format versions.
  --signal-session-key  Signal an #EXT-X-SESSION-KEY tag in the master
                        playlist
  --fairplay=<fairplay-parameters>
                        Enable Fairplay Key Delivery. The <fairplay-
                        parameters> argument is one or more <name>:<value>
                        pair(s) (separated by '#' if more than one). Names
                        include 'uri' [string] (required)
  --widevine=<widevine-parameters>
                        Enable Widevine Key Delivery. The <widevine-header>
                        argument can be either: (1) the character '#' followed
                        by a Widevine header encoded in Base64, or (2) one or
                        more <name>:<value> pair(s) (separated by '#' if more
                        than one) specifying fields of a Widevine header
                        (field names include 'provider' [string] (required),
                        'content_id' [byte array in hex] (optional), 'kid'
                        [16-byte array in hex] (required))
  --output-encryption-key
                        Output the encryption key to a file (default: don't
                        output the key). This option is only valid when the
                        encryption key format is 'identity'
  --exec-dir=<exec_dir>
                        Directory where the Bento4 executables are located
  --base-url=<base_url>
                        The base URL for the Media Playlists and TS files
                        listed in the playlists. This is the prefix for the
                        files.
```
