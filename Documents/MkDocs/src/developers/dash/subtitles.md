MPEG DASH SUBTITLES
===================

The `mp4dash` tool supports including one or more subtitles track(s) in an MPEG DASH presentation.
There are two ways for subtitles to be used with MPEG DASH. Subtitles data may be encapsulated in MP4 tracks, just like audio and video frames, or may be stored in a separate, standalone text or XML file without MP4 encapsulation.

# Formats

Subtitles can be encoded in a varietry of formats. Most of those formats belong to two family of related formats:

## IMSC1 Conformant Profiles
[IMSC1](https://dvcs.w3.org/hg/ttml/raw-file/tip/ttml-ww-pro-files/ttml-ww-profiles.html) conformant profiles of TTML include:
  * W3C TTML
  * SMPTE Timed Text (SMPTE-TT)
  * EBU-TT

## WebVTT
WebVTT is a text format, based on the SRT text format. The specification can be found here: [https://w3c.github.io/webvtt](https://w3c.github.io/webvtt)

## 3GPP Timed Text
Another timed text format defined by 3GPP. This format is currently not supported.

# Using Subtitles with `mp4dash`
The `mp4dash` packager can take one or more subtitles file(s) as input.

## External Subtitles
This is the simplest mode to support, as no MP4 encapsulation is necessary, so a simple WebVTT text or IMSC1-conformant TTML XML file is all that’s needed to hold the subtitles.
When the subtitles are in non-MP4 external files, the input filenames must be prefixed with an input specifier `[+format=webvtt]` for WebVTT-formatted subtitles, or `[+format=ttml]` for IMSC1-conformant TTML files. Also, since WebVTT files do not carry language information, you may want to specify the language code for the input file by adding the parameter `+language=xxx` to the input specifier.

!!! example "Example: single english-language WebVTT subtitles file"
    ```
    mp4dash video.mp4 [+format=webvtt,+language=eng]sub_eng_webvtt.txt
    ```

!!! example "Example: single TTML subtitles file"
    ```
    mp4dash video.mp4 [+format=ttml]sub_fre_ttml.xml
    ```

!!! example "Example: two WebVTT subtitles files"
    ```
    mp4dash video_00500.mp4 [+format=webvtt,+language=eng]sub_eng_webvtt.txt [+format=webvtt,+language=fre]sub_fre_webvtt.txt
    ```

## MP4-Encapsulated Subtitles
MPEG- 4 Part 30 (ISO/IEC 14496-30) defines a way to carry IMSC1-conformat TTM XML in MP4 tracks. Those tracks have a codec 4-character code of `stpp`.
When the subtitles are encapsulated in MP4 tracks, you must specify the `--subtitles` command line option to tell mp4dash to include those tracks in the DASH output.

!!! example "Example: single MP4 input file that contains video, audio and subtitles tracks"
    ```
    mp4dash --subtitles video_file_with_subtitle_track.mp4
    ```

!!! example "Example: separate MP4 file for video/audio and subtitles"
    ```
    mp4dash --subtitles video.mp4 subtitles.mp4
    ```
# References

  * [W3C TTML Profiles for Internet Media Subtitles and Captions 1.0.1 (IMSC1)](https://www.w3.org/TR/ttml-imsc1.0.1/)
  * [SMPTE ST 2052-1:2013 “Timed Text Format (SMPTE-TT)](https://ieeexplore.ieee.org/document/7291854)
