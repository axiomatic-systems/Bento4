# mp4decrypt
```
MP4 Decrypter - Version 1.4
(Bento4 Version 1.6.0.0)
(c) 2002-2015 Axiomatic Systems, LLC

usage: mp4decrypt [options] <input> <output>
Options are:
  --show-progress : show progress details
  --key <id>:<k>
      <id> is either a track ID in decimal or a 128-bit KID in hex,
      <k> is a 128-bit key in hex
      (several --key options can be used, one for each track or KID)
      note: for dcf files, use 1 as the track index
      note: for Marlin IPMP/ACGK, use 0 as the track ID
      note: KIDs are only applicable to some encryption methods like MPEG-CENC
  --fragments-info <filename>
      Decrypt the fragments read from <input>, with track info read
      from <filename>.
```
