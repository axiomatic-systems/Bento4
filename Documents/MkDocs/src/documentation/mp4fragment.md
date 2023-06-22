# mp4fragment
```
MP4 Fragmenter - Version 1.7.0
(Bento4 Version 1.6.0.0)
(c) 2002-2020 Axiomatic Systems, LLC

usage: mp4fragment [options] <input> <output>
options are:
  --verbosity <n> sets the verbosity (details) level to <n> (between 0 and 3)
  --debug enable debugging information output
  --quiet don't print out notice messages
  --fragment-duration <milliseconds> (default = automatic)
  --timescale <n> (use 10000000 for Smooth Streaming compatibility)
  --track <track-id or type> only include media from one track (pass a track ID, 'audio', 'video' or 'subtitles')
  --index (re)create the segment index
  --trim trim excess media in longer tracks
  --no-tfdt don't add 'tfdt' boxes in the fragments (may be needed for legacy Smooth Streaming clients)
  --tfdt-start <start> value of the first tfdt timestamp, expressed as a floating point number in seconds
  --sequence-number-start <start> value of the first segment sequence number (default: 1)
  --force-i-frame-sync <auto|all> treat all I-frames as sync samples (for open-gop sequences)
    'auto' only forces the flag if an open-gop source is detected, 'all' forces the flag in all cases
  --copy-udta copy the moov/udta atom from input to output
  --no-zero-elst don't set the last edit list entry to 0 duration
  --trun-version-zero set the 'trun' box version to zero (default version: 1)
```
