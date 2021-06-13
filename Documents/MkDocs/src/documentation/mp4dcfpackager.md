# mp4dcfpackager
```
MP4 DCF Packager - Version 1.0.1
(Bento4 Version 1.6.0.0)
(c) 2002-2009 Axiomatic Systems, LLC

usage: mp4dcfpackager --method <method> [options] <input> <output>
  --method: <method> is NULL, CBC or CTR
  Options:
  --show-progress: show progress details
  --content-type:  content MIME type
  --content-id:    content ID
  --rights-issuer: rights issuer URL
  --key <k>:<iv>
      Specifies the key to use for encryption.
      <k> a 128-bit key in hex (32 characters)
      and <iv> a 128-bit IV or salting key in hex (32 characters)
  --textual-header <name>:<value>
      Specifies a textual header where <name> is the header name,
      and <value> is the header value
      (several --textual-header options can be used)

```
