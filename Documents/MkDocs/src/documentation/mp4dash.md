# mp4dash
```
Usage: mp4-dash.py [options] <media-file> [<media-file> ...]

Each <media-file> is the path to a fragmented MP4 file, optionally prefixed
with a stream selector delimited by [ and ]. The same input MP4 file may be
repeated, provided that the stream selector prefixes select different streams.
Version 2.0.0 r637

Options:
  -h, --help            show this help message and exit
  -v, --verbose         Be verbose
  -d, --debug           Print out debugging information
  -o <output-dir>, --output-dir=<output-dir>
                        Output directory
  -f, --force           Allow output to an existing directory
  --mpd-name=<filename>
                        MPD file name
  --profiles=<profiles>
                        Comma-separated list of one or more profile(s).
                        Complete profile names can be used, or profile aliases
                        ('live'='urn:mpeg:dash:profile:isoff-live:2011', 'on-
                        demand'='urn:mpeg:dash:profile:isoff-on-demand:2011',
                        'hbbtv-1.5='urn:hbbtv:dash:profile:isoff-live:2012')
  --no-media            Do not output media files (MPD/Manifests only)
  --rename-media        Use a file name pattern instead of the base name of
                        input files for output media files.
  --media-prefix=<prefix>
                        Use this prefix for prefixed media file names (instead
                        of the default prefix "media")
  --init-segment=<filename>
                        Initialization segment name
  --no-split            Do not split the file into individual segment files
  --use-segment-list    Use segment lists instead of segment templates
  --use-segment-template-number-padding
                        Use padded numbers in segment URL/filename templates
  --use-segment-timeline
                        Use segment timelines (necessary if segment durations
                        vary)
  --min-buffer-time=<duration>
                        Minimum buffer time (in seconds)
  --max-playout-rate=<strategy>
                        Max Playout Rate setting strategy for trick-play
                        support. Supported strategies: lowest:X
  --language-map=<lang_from>:<lang_to>[,...]
                        Remap language code <lang_from> to <lang_to>. Multiple
                        mappings can be specified, separated by ','
  --always-output-lang  Always output an @lang attribute for audio tracks even
                        when the language is undefined
  --subtitles           Enable Subtitles
  --attributes=<attributes-definition>
                        Specify the attributes of a set of tracks. This option
                        may be used multiple times, once per attribute set.
  --smooth              Produce an output compatible with Smooth Streaming
  --smooth-client-manifest-name=<filename>
                        Smooth Streaming Client Manifest file name
  --smooth-server-manifest-name=<filename>
                        Smooth Streaming Server Manifest file name
  --smooth-h264-fourcc=<fourcc>
                        Smooth Streaming FourCC value for H.264 video
                        (default=H264) [some older players use AVC1]
  --hls                 Output HLS playlists in addition to MPEG DASH
  --hls-key-url=<url>   HLS key URL (default: key.bin)
  --hls-master-playlist-name=<filename>
                        HLS master playlist name (default: master.m3u8)
  --hls-media-playlist-name=<filename>
                        HLS media playlist name (default: media.m3u8)
  --hls-iframes-playlist-name=<filename>
                        HLS I-Frames playlist name (default: iframes.m3u8)
  --hippo               Produce an output compatible with the Hippo Media
                        Server
  --hippo-server-manifest-name=<filename>
                        Hippo Media Server Manifest file name
  --use-compat-namespace
                        Use the original DASH MPD namespace as it was
                        specified in the first published specification
  --use-legacy-audio-channel-config-uri
                        Use the legacy DASH namespace URI for the
                        AudioChannelConfiguration descriptor
  --encryption-key=<key-spec>
                        Encrypt some or all tracks with MPEG CENC (AES-128),
                        where <key-spec> specifies the KID(s) and Key(s) to
                        use, using one of the following forms: (1) <KID>:<key>
                        or <KID>:<key>:<IV> with <KID> (and <IV> if
                        specififed) as a 32-character hex string and <key>
                        either a 32-character hex string or the character '#'
                        followed by a base64-encoded key seed; or (2) @<key-
                        locator> where <key-locator> is an expression of one
                        of the supported key locator schemes. Each entry may
                        be prefixed with an optional track filter, and
                        multiple <key-spec> entries can be used, separated by
                        ','. (see online docs for details)
  --encryption-cenc-scheme=<cenc-scheme>
                        MPEG Common Encryption scheme (cenc, cbc1, cens or
                        cbcs). (default: cenc)
  --encryption-args=<cmdline-arguments>
                        Pass additional command line arguments to mp4encrypt
                        (separated by spaces)
  --eme-signaling=<eme-signaling-type>
                        Add EME-compliant signaling in the MPD and PSSH boxes
                        (valid options are 'pssh-v0' and 'pssh-v1')
  --merge-keys          Merge all keys in a single set used for all
                        <ContentProtection> elements
  --marlin              Add Marlin signaling to the MPD (requires an encrypted
                        input, or the --encryption-key option)
  --marlin-add-pssh     Add an (optional) Marlin 'pssh' box in the init
                        segment(s)
  --playready           Add PlayReady signaling to the MPD (requires an
                        encrypted input, or the --encryption-key option)
  --playready-version=PLAYREADY_VERSION
                        PlayReady version to use (4.0, 4.1, 4.2, 4.3),
                        defaults to 4.0
  --playready-header=<playready-header>
                        Add a PlayReady PRO element in the MPD and a PlayReady
                        PSSH box in the init segments. The use of this option
                        implies the --playready option. The <playready-header>
                        argument can be either: (1) the character '@' followed
                        by the name of a file containing a PlayReady XML
                        Rights Management Header (<WRMHEADER>) or a PlayReady
                        Header Object (PRO) in binary form,  or (2) the
                        character '#' followed by a PlayReady Header Object
                        encoded in Base64, or (3) one or more <name>:<value>
                        pair(s) (separated by '#' if more than one) specifying
                        fields of a PlayReady Header Object (field names
                        include LA_URL, LUI_URL and DS_ID)
  --playready-add-pssh  Store the PlayReady header in a 'pssh' box in the init
                        segment(s) [deprecated: this is now implicitly on by
                        default when the --playready or --playready-header
                        option is used]
  --playready-no-pssh   Do not store the PlayReady header in a 'pssh' box in
                        the init segment(s)
  --widevine            Add Widevine signaling to the MPD (requires an
                        encrypted input, or the --encryption-key option)
  --widevine-header=<widevine-header>
                        Add a Widevine entry in the MPD, and a Widevine PSSH
                        box in the init segments. The use of this option
                        implies the --widevine option. The <widevine-header>
                        argument can be either: (1) the character '#' followed
                        by a Widevine header encoded in Base64 (either a
                        complete PSSH box or just the PSSH box payload), or
                        (2) one or more <name>:<value> pair(s) (separated by
                        '#' if more than one) specifying fields of a Widevine
                        header (field names include 'provider' [string],
                        'content_id' [byte array in hex], 'policy' [string])
  --primetime           Add Primetime signaling to the MPD (requires an
                        encrypted input, or the --encryption-key option)
  --primetime-metadata=<primetime-metadata>
                        Add Primetime metadata in a PSSH box in the init
                        segments. The use of this option implies the
                        --primetime option. The <primetime-data> argument can
                        be either: (1) the character '@' followed by the name
                        of a file containing the Primetime Metadata to use, or
                        (2) the character '#' followed by the Primetime
                        Metadata encoded in Base64
  --fairplay-key-uri=FAIRPLAY_KEY_URI
                        Specify the key URI to use for FairPlay Streaming key
                        delivery (only valid with --hls option)
  --clearkey            Add Clear Key signaling to the MPD (requires an
                        encrypted input, or the --encryption-key option))
  --clearkey-license-uri=CLEARKEY_LICENSE_URI
                        Specify the license/key URI to use for Clear Key (only
                        valid with --clearkey option)
  --exec-dir=<exec_dir>
                        Directory where the Bento4 executables are located
                        (use '-' to look for executable in the current PATH)
```
