# mp42ts
```
MP4 To MPEG2-TS File Converter - Version 1.3
(Bento4 Version 1.6.0.0)
(c) 2002-2018 Axiomatic Systems, LLC

usage: mp42ts [options] <input> <output>
Options:
  --pmt-pid <pid>   (default: 0x100)
  --audio-pid <pid> (default: 0x101)
  --video-pid <pid> (default: 0x102)
  --segment <segment-duration-in-seconds>
    [with this option, the <output> name must be a 'printf' template,
     like "seg-%d.ts"]
  --segment-duration-threshold in ms (default = 50)
    [only used with the --segment option]
  --pcr-offset <offset> in units of 90kHz (default 10000)
  --verbose
  --playlist <filename>
  --playlist-hls-version <n> (default=3)
```
