# mp4tag
```
MP4 File Tagger - Version 1.2 (Bento4 Version 1.6.0.0)
(c) 2002-2008 Axiomatic Systems, LLC

usage: mp4tag [options] [commands...] <input> [<output>]
commands:
  --help            print this usage information
  --show-tags       show tags found in the input file
  --list-symbols    list all the builtin symbols
  --list-keys       list all the builtin symbolic key names
  --set <key>:<type>:<value>
      set a tag (if the tag does not already exist, set behaves like add)
  --add <key>:<type>:<value>
      set/add a tag
      where <type> is:
        S    if <value> is a UTF-8 string
        LS   if <value> is a UTF-8 string with a language code (see notes)
        I8   if <value> is an 8-bit integer
        I16  if <value> is a 16-bit integer
        I32  if <value> is a 32-bit integer
        JPEG if <value> is the name of a JPEG file
        GIF  if <value> is the name of a GIF file
        B    if <value> is a binary string (see notes)
        Z    if <value> is the name of a builtin symbol
  --remove <key>
       remove a tag
  --extract <key>:<file>
       extract the value of a tag and save it to a file

NOTES:
  In all commands with a <key> argument, except for '--add', <key> can be 
  <key-name> or <key-name>#n where n is the zero-based index of the key when
  there is more than one key with the same name (ex: multiple images for cover
  art).
  A <key-name> has the form <namespace>/<name> or simply <name> (in which case
  the namespace defaults to 'meta').
  The <name> of a key is either one of a symbolic keys
  (see --list-keys) or a 4-character atom name.
  The namespace can be 'meta' for itunes-style metadata or 'dcf' for
  OMA-DCF-style metadata, or a user-defined long-form namespace
  (ex: com.mycompany.foo).
  Binary strings can be expressed as a normal string of ASCII characters
  prefixed by a + character if all the bytes fall in the ASCII range,
  or hex-encoded prefixed by a # character (ex: +hello, or #0FC4)
  Strings with a language code are expressed as: <lang>:<string>,
  where <lang> is a 3 character language code (ex: eng:hello)
```
