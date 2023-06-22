# mp4split
```
MP4 Fragment Splitter - Version 1.2
(Bento4 Version 1.6.0.0)
(c) 2002-2016 Axiomatic Systems, LLC

usage: mp4split [options] <input>
Options:
  --verbose : print verbose information when running
  --init-segment <filename> : name of init segment (default: init.mp4)
  --init-only : only output the init segment (no media segments)
  --media-segment <filename-pattern> (default: segment-%llu.%04llu.m4s)
    NOTE: all parameters are 64-bit integers, use %llu in the pattern
  --start-number <n> : start numbering segments at <n> (default=1)
  --pattern-parameters <params> : one or more selector letter (default: IN)
     I: track ID
     N: segment number
  --track-id <track-id> : only output segments with this track ID
     More than one track IDs can be specified if <track-id> is a comma-separated
     list of track IDs
  --audio : only output audio segments
  --video : only output video segments
```
