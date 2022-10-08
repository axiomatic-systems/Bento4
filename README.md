Bento4
=====
![CI](https://github.com/axiomatic-systems/Bento4/workflows/CI/badge.svg?branch=master)
[![Build Status](https://travis-ci.org/axiomatic-systems/Bento4.svg?branch=master)](https://travis-ci.org/axiomatic-systems/Bento4.svg?branch=master)
[![Code Quality: Cpp](https://img.shields.io/lgtm/grade/cpp/g/axiomatic-systems/Bento4.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/axiomatic-systems/Bento4/context:cpp)
[![Total Alerts](https://img.shields.io/lgtm/alerts/g/axiomatic-systems/Bento4.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/axiomatic-systems/Bento4/alerts)
           
Bento4 is a C++ class library and tools designed to read and write ISO-MP4 files.
This format is defined in international specifications ISO/IEC 14496-12, 14496-14 and 14496-15.
The format is a derivative of the Apple Quicktime file format, so Bento4 can be used to read and write most Quicktime files as well.

Visit [www.bento4.com](http://www.bento4.com) for details

Features
--------

A number of formats and features based on the ISO-MP4 format and related technologies are also supported, including:

 * MPEG DASH with fragmented MP4 files, as defined in ISO/IEC 23009-1
 * CMAF (Common Media Application Format) as defined in ISO/IEC 23000-19
 * MPEG Common Encryption (CENC) as specified in ISO/IEC 23001-7
 * PIFF (Protected Interoperable File Format): encrypted, fragmented MP4 format specified by Microsoft and used for encrypted HTTP Smooth Streaming.
 * Reading and writing 3GPP and iTunes-compatible metadata.
 * ISMA Encrytion and Decryption as defined in the ISMA E&A specification
 * OMA 2.0 and 2.1 DCF/PDCF Encryption and Decryption as defined in the OMA specifications.
 * ISO-MP4 files profiled as part of the 3GPP family of standards.
 * The UltraViolet (DECE) CFF (Common File Format).
 * Parsing and multiplexing of H.264 (AVC) video and AAC audio elementary streams
 * Support for multiple DRM systems that are compatible with MP4-formatted content (usually leveraging CENC Common Encryption), such as Marlin, PlayReady and Widevine.
 * Support for a wide range of codecs, including H.264 (AVC), H.265 (HEVC), AAC, AC-3, EC-3 (Dolby Digital Plus), AC-4, Dolby ATMOS, DTS, ALAC, and many more.

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
|mp42hls	    | converts an MP4 file to an HLS (HTTP Live Streaming) presentation, including the generation of the segments and .m3u8 playlist.
|mp42ts	        | converts an MP4 file to an MPEG2-TS file.
|mp4-dash	    | creates an MPEG DASH output from one or more MP4 files, including encryption.                                                                   
|mp4-dash-clone	| creates a local clone of a remote or local MPEG DASH presentation, optionally encrypting the segments as they are cloned.

Building
--------

The code can be built either by using the pre-configured IDE project files (Mac OSX, iOS and Windows), or compiled/cross-compiled using the SCons build system or CMake, or compiled using Make.
Target platform specific build files and configurations are located under subdirectories Buid/Targets/xxxx where xxxx takes the form ```<architecture>-<vendor>-<os>```. For example, the Linux x86 target specific files are located under ```Build/Targets/x86-unknown-linux```. The XCode project files for Mac OSX are located under ```Build/Targets/universal-apple-macosx```.

### Mac OSX and iOS using XCode
Open the XCode project file Build/Targets/universal-apple-macosx/Bento4.xcodeproj and build

### Windows using Visual Studio
Open the Visual Studio solution file Build/Targets/x86-microsoft-win32-vs2010/Bento4.sln and build

### On Linux and other platforms, Using CMake
CMake can generate Makefiles, Xcode project files, or Visual Studios project files.

#### CMake/Make

	mkdir cmakebuild
	cd cmakebuild
	cmake -DCMAKE_BUILD_TYPE=Release ..
	make

#### CMake/Xcode

	mkdir cmakebuild
	cd cmakebuild
	cmake -G Xcode ..
    cmake --build . --config Release

#### CMake/Visual Studio
	mkdir cmakebuild
	cd cmakebuild
	cmake -DCMAKE_BUILD_TYPE=Release ..
    cmake --build . --config Release

#### CMake for Android NDK
    mkdir cmakebuild
    cd cmakebuild
    cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=$ABI -DANDROID_NATIVE_API_LEVEL=$MINSDKVERSION ..
    make

    See https://developer.android.com/ndk/guides/cmake for details on the choice of ABI and other parameters.
    
    Where $NDK is set to the directory path where you have installed the NDK, $ABI is the Android ABI (ex: arm64-v8a) and $MINSDKVERSION is the minimum SDK version (ex: 23)

### On Linux and other platforms, using SCons (deprecated)
Make sure you the the SCons build tool installed on your host machine (http://www.scons.org).
To build the Debug configuration, simply enter the command:

```scons -u```

in a terminal from any directory (either from the top level directory where you downloaded the Bento4 distribution, or from the Build/Targets/xxx subdirectory for your specific target).

To build the Release configuration, use the command:

```scons -u build_config=Release```

To cross-compile for a target other than your host architecture, specify target=xxxx as an argument to the scons build command.

Example:

```scons -u build_config=Release target=x86_64-unknown-linux```

### Using Make
From a command shell, go to your build target directory.

For Debug builds:
	```make```

For Release builds:
```make AP4_BUILD_CONFIG=Release```

## Installing Bento4 (vcpkg)

Alternatively, you can build and install Bento4 using [vcpkg](https://github.com/Microsoft/vcpkg/) dependency manager:

    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg integrate install
    ./vcpkg install bento4

The Bento4 port in vcpkg is kept up to date by Microsoft team members and community contributors. If the version is out of date, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.

Release Notes
-------------

### 1.6.0-638
  * support multi-bitrate audio
  * new docs using MkDocs
  * add av1 files and remove deprecated option from vs2019 build
  * add AV1 support
  * better handling of USAC signaling
  * add UTF-8 support on Windows
  * fix LGTM warnings
  * account for last sample when at EOS
  * new inspector API
  * bug fixes

### 1.6.0-636
Dolby Vision encryption now properly encrypts in a NAL-unit-aware mode

### Previous releases
(no seaparate notes, please refer to commit logs)
