DOCUMENTATION
=============

The documentation for Bento4 consists of pages hosted on this site, documents stored as files in the source distribution and SDKs, as well as embedded documentation in the source code header files.

MPEG DASH Packager
------------------
The [MPEG DASH](../developers/dash/index.md) page is the starting point for information on MPEG DASH packaging.
For a more advanced, detailed set of options, you can also directly consult the documentation for the [mp4dash](mp4dash.md) tool.  
The MPEG DASH packager can also be used to package HLS with MP4 fragments, allowing a dual DASH/HLS output from a single set of media files.

HLS Packager
------------
The [HLS](../developers/hls/index.md) page is the starting point for information HLS packaging.
For a more advanced and detailed set of options, you can also directly consult the documentation for the mp4hls tool.

Tools Documentation
-------------------

| TOOL NAME | DESCRIPTION |
|-----------|-------------|
| [mp4info](mp4info.md) | displays high level info about an MP4 file, including all tracks and codec details
| [mp4dump](mp4info.md) | displays the entire atom/box structure of an MP4 file
| [mp4edit](mp4edit.md) | add/insert/remove/replace atom/box items of an MP4 file
| [mp4extract](mp4extract.md) | extracts an atom/box from an MP4 file
| [mp4encrypt](mp4encrypt.md) | encrypts an MP4 file (multiple encryption schemes are supported)
| [mp4decrypt](mp4decrypt.md) | decrypts an MP4 file (multiple encryption schemes are supported)
| [mp4dcfpackager](mp4dcfpackager.md) | encrypts a media file into an OMA DCF file
| [mp4compact](mp4compact.md) | converts stsz tables into stz2 tables to create more compact MP4 files
| [mp4fragment](mp4fragment.md) | creates a fragmented MP4 file from a non-fragmented one.
| [mp4split](mp4split.md) | splits a fragmented MP4 file into discrete files
| [mp4tag](mp4tag.md) | show/edit MP4 metadata (iTunes-style and others)
| [mp4mux](mp4mux.md) | multiplexes one or more elementary streams (H264, AAC) into an MP4 file
| [mp42aac](mp42aac.md) | extract a raw AAC elementary stream from an MP4 file
| [mp42avc](mp42avc.md) | extract a raw AVC/H.264 elementary stream from an MP4 file
| [mp42hevc](mp42hevc.md) | extract a raw AVC/H.264 elementary stream from an MP4 file
| [mp42hls](mp42hls.md) | converts an MP4 file to an HLS (HTTP Live Streaming) presentation, including the generation of the segments and .m3u8 playlist as well as AES-128 and SAMPLE-AES (for FairPlay) encryption. This can be used as a replacement for Apple’s mediafilesegmenter tool.
| [mp42ts](mp42ts.md) | converts an MP4 file to an MPEG2-TS file.
| [mp4dash](mp4dash.md) | creates an MPEG DASH output from one or more MP4 files, including encryption.
| [mp4dashclone](mp4dashclone.md) | creates a local clone of a remote or local MPEG DASH presentation, optionally encrypting the segments as they are cloned.
| [mp4hls](mp4hls.md) | creates a multi-bitrate HLS master playlist from one or more MP4 files, including support for encryption and I-frame-only playlists. This can be used as a replacement for Apple’s variantplaylistcreator tool.

Library API Documentation
-------------------------

Some of the API documentation is produced from the source code comments using Doxygen. The doxygen output is available as a windows CHM file in Bento4.chm, and a set of HTML pages zipped together in Bento4-HTML.zip (to start, open the file named index.html with an HTML browser)

Licensing
---------

Consult the [About](../about.md) page for GPL and non-GPL licensing information.