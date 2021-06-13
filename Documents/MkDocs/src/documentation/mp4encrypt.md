# mp4encrypt
```
MP4 Encrypter - Version 1.6
(Bento4 Version 1.6.0.0)
(c) 2002-2016 Axiomatic Systems, LLC

usage: mp4encrypt --method <method> [options] <input> <output>
     <method> is OMA-PDCF-CBC, OMA-PDCF-CTR, MARLIN-IPMP-ACBC,
     MARLIN-IPMP-ACGK, ISMA-IAEC, PIFF-CBC, PIFF-CTR, MPEG-CENC,
     MPEG-CBC1, MPEG-CENS, MPEG-CBCS
  Options:
  --show-progress
      Show progress details
  --fragments-info <filename>
      Encrypt the fragments read from <input>, with track info read
      from <filename>
  --key <n>:<k>:<iv>
      Specifies the key to use for a track (or group key).
      <n> is a track ID, <k> a 128-bit key in hex (32 characters)
      and <iv> a 64-bit or 128-bit IV or salting key in hex
      (16 or 32 characters) depending on the cipher mode
      The key and IV values can also be specified with the keyword 'random'
      instead of a hex-encoded value, in which case a randomly generated value
      will be used.
      (several --key options can be used, one for each track)
  --strict
      Fail if there is a warning (ex: one or more tracks would be left unencrypted)
  --property <n>:<name>:<value>
      Specifies a named string property for a track
      <n> is a track ID, <name> a property name, and <value> is the
      property value
      (several --property options can be used, one or more for each track)
  --global-option <name>:<value>
      Sets the global option <name> to be equal to <value>
  --pssh <system-id>:<filename>
      Add a 'pssh' atom for this system ID, with the payload
      loaded from <filename>.
      (several --pssh options can be used, with a different system ID for each)
      (the filename can be left empty after the ':' if no payload is needed)
  --pssh-v1 <system-id>:<filename>
      Same as --pssh but generates a version=1 'pssh' atom
      (this option must appear *after* the --property options on the command line)
  --kms-uri <uri>
      Specifies the KMS URI for the ISMA-IAEC method

  Method Specifics:
    OMA-PDCF-CBC, MARLIN-IPMP-ACBC, MARLIN-IPMP-ACGK, PIFF-CBC, MPEG-CBC1, MPEG-CBCS: 
    the <iv> can be 64-bit or 128-bit
    If the IV is specified as a 64-bit value, it will be padded with zeros.

    OMA-PDCF-CTR, ISMA-IAEC, PIFF-CTR, MPEG-CENC, MPEG-CENS:
    the <iv> should be a 64-bit hex string.
    If a 128-bit value is supplied, it will be truncated to 64-bit.

    OMA-PDCF-CBC, OMA-PDCF-CTR: The following properties are defined,
    and all other properties are stored in the textual headers:
      ContentId       -> the content ID for the track
      RightsIssuerUrl -> the Rights Issuer URL

    MARLIN-IPMP-ACBC, MARLIN-IPMP-ACGK: The following properties are defined:
      ContentId -> the content ID for the track

    MARLIN-IPMP-ACGK: The group key is specified with --key where <n>
    is 0. The <iv> part of the key must be present, but will be ignored;
    It should therefore be set to 0000000000000000

    MPEG-CENC, MPEG-CBC1, MPEG-CENS, MPEG-CBCS, PIFF-CTR, PIFF-CBC:
    The following properties are defined:
      KID -> the value of KID, 16 bytes, in hexadecimal (32 characters)
      ContentId -> Content ID mapping for KID (Marlin option)
      PsshPadding -> pad the 'pssh' container to this size
                    (only when using ContentId).
                    This property should be set for track ID 0 only
```
