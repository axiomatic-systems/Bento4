# mp4edit
```
MP4 File Editor - Version 1.2
(Bento4 Version 1.6.0.0)
(c) 2002-2017 Axiomatic Systems, LLC

usage: mp4edit [commands] <input> <output>
    where commands include one or more of:
    --insert <atom_path>:<atom_source>[:<position>]
    --remove <atom_path>
    --replace <atom_path>:<atom_source>

    and <atom_source> may be either a filename for a file
    that contains the atom data (header and payload), or
    <uuid>#<file> with <uuid> specifying an atom uuid type,
    as a 32-character hex value, and <file> a file with the atom
    payload only.
```
