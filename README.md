Bento4 
======
           
Bento4 is a C++ class library and tools designed to read and write ISO-MP4 files. 
This format is defined in international specifications ISO/IEC 14496-12, 14496-14 and 14496-15. 
The format is a derivative of the Apple Quicktime file format, so Bento4 can be used to read and write most Quicktime files as well.

Visit http://www.bento4.com for details

Features
--------

A number of formats and features based on the ISO-MP4 format and related technologies are also supported, including:

 * MPEG DASH with fragmented MP4 files, as defined in ISO/IEC 23009-1
 * MPEG Common Encryption (CENC) as specified in ISO/IEC 23001-7
 * PIFF (Protected Interoperable File Format): encrypted, fragmented MP4 format specified by Microsoft and used for encrypted HTTP Smooth Streaming.
 * Reading and writing 3GPP and iTunes-compatible metadata.
 * ISMA Encrytion and Decryption as defined in the ISMA E&A specification
 * OMA 2.0 and 2.1 DCF/PDCF Encryption and Decryption as defined in the OMA specifications.
 * ISO-MP4 files profiled as part of the 3GPP family of standards.
 * The UltraViolet (DECE) CFF (Common File Format).
 * Parsing and multiplexing of H.264 (AVC) video and AAC audio elementary streams
 * Support for multiple DRM systems that are compatible with MP4-formatted content (usually leveraging CENC Common Encryption), such as Marlin and PlayReady and Widevine.
 * Support for a wide range of codecs, including H.264 (AVC), H.265 (HEVC), AAC, AC3 and eAC3 (Dolby Digital), DTS, ALAC, and many more.

Design
------

The SDK is designed to be cross-platform. The code is very portable; it can be compiled with any sufficiently modern C++ compiler. The implementation does not rely on any external library. All the code necessary to compile the SDK and tools is included in the standard distribution. The standard distribution contains makefiles for unix-like operating systems, including Linux and Android, project files for Microsoft Visual Studio, and an XCode project for MacOS X and iOS. There is also support for building the library with the SCons build system.


License
-------

The library is Open Source, with a dual-license model. You can find out more about the license on the About Page.
The Developers Page contains specific information on where to obtain the source code and documentation. The Downloads Page contains the links to pre-built SDKs and tools that you can use to get started quickly.

Included Applications
---------------------

The Bento4 SDK includes several command-line applications/tools that are built using the SDK API. These include:

|app name       | description
|---------------|------------------
|mp4info	    | displays high level info about an MP4 file, including all tracks and codec details                                                              
|mp4dump	    | displays the entire atom/box structure of an MP4 file                                                                                           
|mp4edit	    | add/insert/remove/replace atom/box items of an MP4 file                                                                                         
|mp4extract	    | extracts an atom/box from an MP4 file                                                                                                           
|mp4encrypt	    | encrypts an MP4 file (multiple encryption schemes are supported)                                                                                
|mp4decrypt	    | decrypts an MP4 file (multiple encryption schemes are supported)                                                                                
|mp4dcfpackager | encrypts a media file into an OMA DCF file                                                                                                      
|mp4compact	    | converts ‘stsz’ tables into ‘stz2′ tables to create more compact MP4 files                                                                      
|mp4fragment    | creates a fragmented MP4 file from a non-fragmented one or re-fragments an already-fragmented file                                              
|mp4split	    | splits a fragmented MP4 file into discrete files                                                                                                
|mp4tag	        | show/edit MP4 metadata (iTunes-style and others)                                                                                                
|mp4mux	        | multiplexes one or more elementary streams (H264, AAC) into an MP4 file                                                                         
|mp42aac	    | extract a raw AAC elementary stream from an MP4 file                                                                                            
|mp42avc	    | extract a raw AVC/H.264 elementary stream from an MP4 file                                                                                      
|mp42ts	        | converts an MP4 file to an MPEG2-TS file, including support for fragmentation and generation of HTTP Live Streaming (HLS) playlists/manifests.
|mp4-dash	    | creates an MPEG DASH output from one or more MP4 files, including encryption.                                                                   
|mp4-dash-clone	| creates a local clone of a remote or local MPEG DASH presentation, optionally encrypting the segments as they are cloned.

