# mp4mux
```
MP4 Elementary Stream Multiplexer - Version 3.0
(Bento4 Version 1.6.0.0)
(c) 2002-20020 Axiomatic Systems, LLC

usage: mp4mux [options] --track [<type>:]<input>[#<params>] [--track [<type>:]<input>[#<params>] ...] <output>

  <params>, when specified, are expressd as a comma-separated list of
  one or more <name>=<value> parameters

Supported types:
  h264: H264/AVC NAL units
  h265: H265/HEVC NAL units
    optional params:
      dv_profile: integer number for Dolby vision profile ID (valid value: 5,8,9)
      dv_bc: integer number for Dolby vision BL signal cross-compatibility ID (mandatory if dv_profile is set to 8/9)
      frame_rate: floating point number in frames per second (default=24.0)
      format: hev1 or hvc1 (default) for HEVC tracks and Dolby Vision backward-compatible tracks
              dvhe or dvh1 (default) for Dolby vision tracks
  aac:  AAC in ADTS format
  ac3:  Dolby Digital
  ec3:  Dolby Digital Plus
  ac4:  Dolby AC-4
  mp4:  MP4 track(s) from an MP4 file
    optional params:
      track: audio, video, or integer track ID (default=all tracks)

Common optional parameters for all types:
  language: language code (3-character ISO 639-2 Alpha-3 code)

If no type is specified for an input, the type will be inferred from the file extension

Options:
  --verbose: show more details
```
