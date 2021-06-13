Welcome to Bento4
======================

Bento4 MP4, DASH, HLS, CMAF SDK and Tools
-----------------------------------------

A fast, modern, open source C++ toolkit for all your MP4 and DASH/HLS/CMAF media format needs.  

![](images/tokyometro.jpg)

Bento4 is a C++ class library and tools designed to read and write ISO-MP4 files. This format is defined in international specifications ISO/IEC 14496-12, 14496-14 and 14496-15. The format is a derivative of the Apple Quicktime file format, so Bento4 can be used to read and write most Quicktime files as well. In addition to supporting ISO-MP4, Bento4 includes support for parsing and multiplexing H.264 and H.265 elementary streams, converting ISO-MP4 to MPEG2-TS, packaging HLS and MPEG-DASH, CMAF, content encryption, decryption, and much more.

Features
--------

A number of formats and features based on the ISO-MP4 format and related technologies are also supported, including:

  * [MPEG DASH](developers/dash/index.md) with fragmented MP4 files, as defined in the international specification [ISO/IEC 23009-1](https://www.iso.org/standard/75485.html)
  * [HLS](developers/hls/index.md) with TS or MP4 segments (dual DASH/HLS output), as defined in [RFC 8216](https://datatracker.ietf.org/doc/html/rfc8216)
  * [CMAF](developers/cmaf/index.md) (Common Media Application Format) as defined in ISO/IEC 23000-19
  * MPEG Common Encryption (CENC) as specified in the international specification [ISO/IEC 23001-7](https://www.iso.org/standard/68042.html)
  * [PIFF](http://go.microsoft.com/?linkid=9682897) (Protected Interoperable File Format), and encrypted, fragmented MP4 format specified by Microsoft and used for encrypted HTTP Smooth Streaming.
  * Reading and writing 3GPP and iTunes-compatible metadata.
  * ISMA Encrytion and Decryption as defined in the [ISMA E&A specification](http://www.isma.tv/)
  * OMA 2.0 and 2.1 DCF/PDCF Encryption and Decryption as defined in the [OMA specifications](http://www.openmobilealliance.org/).
  * ISO-MP4 files profiled as part of the [3GPP family of standards](http://www.3gpp.org/).
  * The [UltraViolet](http://www.uvvu.com/) (DECE) CFF (Common File Format).
  * Parsing and multiplexing of H.264 (AVC) video and AAC audio elementary streams
  * Support for multiple DRM systems that are compatible with MP4-formatted content (usually leveraging CENC Common Encryption), such as Marlin, PlayReady, Widevine, FairPlay and Adobe Access.
  * Support for a wide range of codecs, including H.264 (AVC), H.265 (HEVC), AAC, HE-AAC, xHE-AAC, AC3 and eAC3 (Dolby Digital), AC4, Dolby Atmos, DTS, ALAC, and many more.
  * Support for Dolby Vision and HDR.

The SDK is designed to be cross-platform. The code is very portable; it can be compiled with any sufficiently modern C++ compiler. The implementation does not rely on any external library. All the code necessary to compile the SDK and tools is included in the standard distribution. The standard distribution contains makefiles for unix-like operating systems, including Linux and Android, project files for Microsoft Visual Studio, and an XCode project for MacOS X and iOS. There is also support for building the library with the [SCons](http://www.scons.org/) build system as well as [CMake](https://cmake.org/).

The library is Open Source, with a dual-license model. You can find out more about the license on the [About Page](about.md).
The [Developers Page](developers/index.md) contains specific information on where to obtain the source code and documentation.  
The [Downloads Page](downloads.md) contains the links to pre-built SDKs and tools that you can use to get started quickly.

Included Applications
---------------------

The Bento4 SDK includes several command-line applications/tools that are built using the SDK API. These include:

| TOOL NAME | DESCRIPTION |
|-----------|-------------|
| mp4info	| displays high level info about an MP4 file, including all tracks and codec details |
| mp4dump	|displays the entire atom/box structure of an MP4 file |
| mp4edit	|add/insert/remove/replace atom/box items of an MP4 file |
| mp4extract |extracts an atom/box from an MP4 file |
| mp4encrypt |encrypts an MP4 file (multiple encryption schemes are supported) |
| mp4decryp | decrypts an MP4 file (multiple encryption schemes are supported) |
| mp4dcfpackager | encrypts a media file into an OMA DCF file |
| mp4compact | converts ‘stsz’ tables into ‘stz2’ tables to create more compact MP4 files |
| mp4fragment | creates a fragmented MP4 file from a non-fragmented one |
| mp4split | splits a fragmented MP4 file into discrete files |
| mp4tag | show/edit MP4 metadata (iTunes-style and others) |
| mp4mux | multiplexes one or more elementary streams (H264/AVC, H265/HEVC, AAC) into an MP4 file |
| mp42aac | extract a raw AAC elementary stream from an MP4 file |
| mp42avc | extract a raw AVC/H.264 elementary stream from an MP4 file |
| mp42hevc | extract a raw HEVC/H.265 elementary stream from an MP4 file |
| mp42hls | converts an MP4 file to an HLS (HTTP Live Streaming) presentation, including the generation of the segments and .m3u8 playlist as well as AES-128 and SAMPLE-AES (for Fairplay) encryption. This can be used as a replacement for Apple’s mediafilesegmenter tool |
| mp42ts | converts an MP4 file to an MPEG2-TS file |
| mp4dash | creates an MPEG DASH output from one or more MP4 files, including encryption. As an option, an HLS playlist with MP4 segments can also be generated at the same time, allowing a single stream to be served as DASH and HLS. This is a full-featured MPEG DASH / HLS packager |
| mp4dashclone | creates a local clone of a remote or local MPEG DASH presentation, optionally encrypting the segments as they are cloned |
| mp4hls | creates a multi-bitrate HLS master playlist from one or more MP4 files, including support for encryption and I-frame-only playlists. This tool uses the ‘mp42hls’ low level tool internally, so all the options supported by that low level tool are also available. This can be used as a replacement for Apple’s variantplaylistcreator tool |