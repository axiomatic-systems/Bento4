MPEG DASH Adaptive Streaming
============================

# Specifications

MPEG DASH is MPEG’s standardized **D**ynamic **A**daptive **S**treaming over **H**TTP. It is specified in the following international standards:

  * ISO/IEC 23009-1 specifies the overall DASH architecture and the XML syntax for the MPD (Media Presentation Document)
  * ISO/IEC 23001-7 specifies the Common Encryption for MP4 fragments
  * ISO/IEC 14496-12/AMD 3 specifies the extensions to 14496-12 that are necessary to support DASH with fragmented MP4 media

# Quick introduction to MPEG DASH

At a high level, an MPEG DASH presentation consists of an initial XML manfifest, called the **Media Presentation Document** (MPD for short), which describes media segments that form a complete presentation. Along with a number of attributes, the MPD allows the MPEG DASH player to compute the URL of each segment, and to download and render them.
The `mp4dash` tool takes care of creating the DASH MPD document, as well as packaging the media segment in their proper form and output location, including pre- and post- processing if necessary (like encryption for DRM support).

# Using the `mp4dash` packager tool

`mp4dash` is the name of the Bento4 tool that can convert one or more input media files into a complete MPEG DASH presentation. 

!!! tip "Using `mp4dash` for HLS as well"
    Recent versions of the HLS specification support uusing MP4 fragments. Since those fragments are essentially the same as those used for MPEG DASH, the `mp4dash` tool can be used to output HLS presentations as well as DASH, from a single set of input files. Use the `--hls` option to enable that mode. See the [HLS](../hls/index.md) page for more details.

Running this tool requires that you have python version 3.7 or above installed on your system (python 2.x is no longer supported). Please visit [www.python.org](https://www.python.org) if you need to install python.

!!! note "Python on Windows"
    On Windows, the launch scripts assume that python3 is launched with the command python, but on all other systems, the command python3 is used.

If you are using the Bento4 precompiled SDK distribution, you will find it convenient to use the `mp4dash` command, located in the `bin/` directory, which will automatically run the `mp4-dash.py` python script for you. If you are running from a source distribution, you may need to invoke the python script (located under `Source/Python/utils`) directly, like this:

```bash
python3 <path-to-bento4-python-utils>/mp4-dash.py <arguments>
```

In the examples below, we assume that you are running from a standard precompiled distribution, thus we will simply use the `mp4dash` command.

!!! tip "Running with self-compiled binaries"
    The `mp4dash` tool needs to invoke the Bento4 command line binaries `mp4dump`, `mp4encrypt`, `mp4info`, and `mp4split`. If you are invoking the tool as distributed in the pre-compiled SDK, the binaries will automatically be found in the SDK package as installed. If you are running the tool from the Source SDK, with binaries that you compiled from the source, so you’ll need to use the `--exec-dir` option to specify the directory where your built binaries reside (platform dependent).

If you just run the `mp4dash` tool with no arguments, it will print out the options that are supported.
For a complete list of command line options for `mp4dash`, visit the [mp4dash](../../documentation/mp4dash.md) command line details page.

## Preparing the input files

To create DASH MP4 content, you need to start with fragmented MP4 files. The input files you will be working with may or may not already be in fragmented MP4 form. Ideally, your encoder will already produce MP4 files that way.
The command line tool `mp4info` can tell you if an MP4 file is fragmented or not: here’s an example of what you would see in the `Movie:` part of the `mp4info` output for a non-fragmented MP4 file (the line `fragments: no`)

```
Movie:
    duration:   147188 ms
    time scale: 2997
    fragments:  no

Found 2 Tracks
```

If you have non-fragmented MP4 files, you can use the mp4fragment tool to fragment them.

!!! example "`mp4fragment` example"
    ```
    mp4fragment video.mp4 video-fragmented.mp4
    ```
  
### Dealing with ISMV and ISMA input files

ISMV and ISMA files are basically fragmented MP4 files, and as such can be used as input to `mp4dash`. However, most legacy tools (encoder and packagers) that produce ISMV and ISMA files generate fragmented MP4 files that lack a `tfdt` timestamp box. While these files appear to be normal fragmented MP4 files, the lack of tfdt renders those media files unsuitable for playback with many DASH clients, including most HTML5 based clients.
If you have such files, you can fix this problem by refragmenting your ISMV and ISMA files into compliant fragmented MP4 files, using the `mp4fragment` tool.

!!! example "Example: re-fragmenting ISMV/ISMA files"
    ```
    mp4fragment video.ismv video.mp4
    ```

    This should print something like

    ```
    NOTICE: file is already fragmented, it will be re-fragmented 
    found regular I-frame interval: 1 frames (at 25.000 frames per second)
    ```

    You can then use the re-fragmented files as input to mp4dash.

!!! tip
    You can tell whether your fragmented input file has a tfdt box or not by using mp4dump.
    A file with tfdt will show entries like this:

    ```
    [moof] size=8+816
    [mfhd] size=12+4
        sequence number = 490
    [traf] size=8+792
        [tfhd] size=12+4, flags=20000
        track ID = 2
        [tfdt] size=12+8, version=1
        base media decode time = 32281600
        [trun] size=12+744, flags=301
        sample count = 92
        data offset = 832
    ```
    with a tfdt box as a child of the traf container box

## Generating a DASH presentation

Once you have fragmented MP4 files to work with as input, you can generate a DASH presentation, including an MPD and media files or media segments. For single-bitrate streaming, a single MP4 file is required. 
The general usage for the `mp4dash` tool is: `mpdash <input files>`, which produces a DASH presentation in an output directory (`output` by default), including the MPD file that points to the media segments (`output/stream.mpd` by default).

### Single Bitrate

!!! example "Example: single MP4 input file"
    ```
    mp4dash video.mp4
    ``` 

### Multiple Bitrates

While it is entirely possible to use MPEG DASH for simple, single-bitrate Audio/Video streams, multi-bitrate support is where MPEG DASH shines. The most common use case is to package multiple video streams, each encoded at a different bitrate, into a single presentation, where the player will adaptively switch between segments of the different streams depending on the network and playback conditions.
When supplying multiple video streams as input (one video track per input file) to `mp4dash`, it is important that the files have been encoded in a way that is compatible for this use case. You will need a set of MP4 files that have been encoded with closed GOPs (Group Of Pictures) with equal durations. Also, the audio tracks in all the files should be encoded with the same parameters. Once you have your input files, you can use the `mp4dash` tool to automatically generate the DASH MPD and, optionally, split the MP4 file into individual file segments.

!!! example "Example: multi-bitrate set of MP4 files"
    ```
    mp4dash video_1000.mp4 video_2000.mp4 video_3000.mp4
    ```

Codecs other than xHE-AAC aren't well suited for seamless dynamic bitrate switching, so when using traditional codecs like AAC or Dolby Digital, it is usually the case that a single audio stream (per codec) will be present as input. If more than one input includes an audio track (which may be expected with multi-bitrate packaging, where each of the input files may contain both an audio and video track), only one of the audio tracks will be retained, unless they differ by more than 10% in bitrate.
For the xHE-AAC codec, however, seamless dynamic bitrate switching is possible. For more details on how to support that use case, read the [Multi Bitrate Audio page](multi_bitrate_audio.md).

## Advanced usage

### Input specifiers

The name of input files may be preceded by an input qualifier, which can be used for stream selection as well as setting properties for the input. The general syntax for input qualifiers is: `[<selector-name>=<selector-value>,+<property-name>=<property-value>,...]`.

#### Selectors

By default, `mp4dash` will create an output that contains all the audio tracks found in its each input, as well as one video track from each input (it is assumed that each input file only has one video track, but multi-track input files may still be used, using a selector to select one of the tracks). It is possible to limit the selected tracks by using input stream selectors.
The supported selectors are:

  Selector   | Value                                     | Descripton
  -----------|-------------------------------------------|--------------------------------------------------------
  `type`     | `audio` or `video`                        | Select the `audio` or `video` track only from the input 
  `track`    | `<track-id>` (integer)                    | Select track ID <track-id> only from the input
  `language` | `<language-code>` (2 or 3 character code) | Only keep audio tracks with a specific language
  
#### Properties

Setting stream properties via input specifiers allows invoking `mp4dash` with extra information about the input that may not be otherwise inferred, or should override some of the streams' properties. This can be used, for example, to override the language code for an audio track, or specify labels or group names.

  Property            | Value                                     | Description
  --------------------|-------------------------------------------|--------------------------
  `language`          | `<language-code>` (2 or 3 character code) | Set/override the language code for an input
  `language_name`     | `<language-name>` (string)                | Set the language name (i.e human-readable string for the language) for an input
  `representation_id` | `<representation-id>` (string)            | |
  `scan_type`         | `<scan-type>` (string)                    | |
  `label`             | `<label>` (string)                        | |
  `key`               | `<key>` (hex string)                      | |
  `hls_group`         | `<hls-group>` (string)                    | (only with `--hls`  |
  `hls_group_match`   | `<hls-group-match>`                       | (only with `--hls`) |
  `hls_default`       | `YES` or `NO`                             | (only with `--hls`) |
  `hls_autoselect`    | `YES` or `NO`                             | (only with `--hls`) |
  `hls_characteristic`| `<hls-characteristic>` (string)           | (only with `--hls`) |

### Multi-language

Use a `language=<language-code>` selector to select one of the audio tracks. The language code  is a 2 or 3 letter code as defined by the DASH specification – (see [RFC 5646](https://datatracker.ietf.org/doc/html/rfc5646)]. It is possible to combine the language selector with a type selector, by separating the two by a comma (ex: `[type=audio,language=fr]`). If the language selector is used by itself without a type selector, the video track as well as the matching audio track will be selected.

#### Examples

!!! example
    Select the French audio track and the video track from video.mp4:

    ```
    mp4dash [language=fr]video.mp4
    ```

!!! example
    Select the French audio track from `video1.mp4` and the video track from `video2.mp4`:

    ```
    mp4dash [type=audio,language=fr]video1.mp4 [type=video]video2.mp4
    ```

!!! example
    Select track ID 3 from video1.mp4 and all tracks from video2.mp4:

    ```
    mp4dash [track=3]video1.mp4 video2.mp4
    ```

!!! example
    Select the track with `undefined` language from `video1.mp4` and remap the language to `fr`

    ```
    mp4dash --language-map=und:fr [language=und]video1.mp4
    ```

!!! tip
    Using the `--verbose` option may be useful when debugging language selection options

### Encryption and DRM

MPEG DASH streams can be encrypted, and played on clients that have a DRM-enabled DASH player.
Visit the [MPEG DASH Encryption & DRM page](encryption_and_drm.md) for details.

### Codec Support

The `mp4dash` packager supports input files with streams encoded with a variety of difference codecs. For details on what's supported and what options may be used, read the [Codecs page](codecs.md).

# Serving DASH Streams

By default, `mp4dash` produces an output that can be served by any regular HTTP server. In order to make this possible, each one of the media segments and the init segments are stored in separate files. The MPD then refers to relative URLs for those files. Care should be taken to configure the HTTP server to return the correct `Content-Type` for the MPD document and segment files. The MPD document (named `stream.mpd` by default by the `mp4dash` tool) should be served with `Content-Type: application/dash+xml` and the init and media segments should be served with a `Content-Type: video/mp4`

## Serving DASH without splitting segments

For players that support the "on demand" profile, input files can be kept as single files on the server, and the players use an index embedded in the files to access individual segments using HTTP range requests. 
For the "live" profile, however, there is no such embedded index. With special support from the server, it is possible to serve such DASH streams without splitting the segments into individual files. This is achieved by having a server-side module that can virtualize URLs, so as to be able to expose each segment as an separate URL, mapping it back to a byte range in a source file when requests are made. MPEG does not specify a common standard for such URL virtualization. Each server that supports this model is free do do it any way it chooses. For example, the Microsoft IIS server can be used with a IIS Media Services extension for Smooth Streaming to deliver MPEG DASH streams. More details can be found on the Serving MPEG DASH with Microsoft IIS page. Another useful way to do the same thing is to use the open-source [Hippo Media Server](https://github.com/barbibulle/hippo) package, which knows how to virtualize URLs for MPEG DASH and Smooth Streaming.
The `--hippo` option of `mp4dash` is used to generate the server-side manifest needed by the Hippo Media Server.