# mp4dump
```
MP4 File Dumper - Version 1.2
(Bento4 Version 1.6.0.0)
(c) 2002-2011 Axiomatic Systems, LLC

usage: mp4dump [options] <input>
options are:
  --verbosity <n>
      sets the verbosity (details) level to <n> (between 0 and 3)
  --track <track_id>[:<key>]
      writes the track data into a file
      (<mp4filename>.<track_id>) and optionally
      tries to decrypt it with the key (128-bit in hex)
      (several --track options can be used, one for each track)
      Each sample is written preceded by its size encoded as a 32-bit
      value in big-endian byte order
  --format <format>
      format to use for the output, where <format> is either 
      'text' (default) or 'json'
```
