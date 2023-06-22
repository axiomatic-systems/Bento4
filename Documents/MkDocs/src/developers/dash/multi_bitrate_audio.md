MULTI-BITRATE AUDIO
===================

The most common use of MPEG DASH is for delivering adaptive streaming presentations to players. The adaptive part is achieved by offering the same video stream encoded at different bitrates (and typically different resolutions). When properly encoded, the segments of media for the different video bitrates are such that seamless transition from one bitrate set to another can be done on the fly without glitches. 

For audio, however, it is typical to only have one bitrate. Offering multiple audio variants is often limited to offering a choice of stereo and multi-channel audio, where the stereo stream is usually encoded with AAC, and the multi-channel stream(s) with Dolby Digital or similar. The main reason for only having a single bitrate is the fact that with AAC and other codecs, seamless transition between audio streams encoded at different bitrates isn't easy or often not even possible. A second reason is that the audio bitrate is usually much lower than the video bitrate, so varrying the audio bitrate doesn't offer much of a difference in terms of the absolute total bitrate that the player has to receive.

With new generations of codecs, like xHE-AAC, which combines the "Extended High Efficiency AAC" profile of the  MPEG-D USAC standard (ISO/IEC 23003-3) and appropriate parts of the MPEG-D DRC Loudness Control Profile or Dynamic Range Control Profile, it is now possible to offer audio encoded at different bitrates, and allow the player to seamlessly switch between the different bitrates on the fly


To learn more about the xHE-AAC audio codec, please visit Fraunhofer's [www.xhe-aac.com](https://www.xhe-aac.com) website

The `mp4dash` tool supports input audio files that are encoded with xHE-AAC.

# xHE-AAC with MPEG DASH

No special option is needed when creating an MPEG DASH presentation with xHE-AAC audio tracks. Simply pass all the audio files as input to the packager, along with any video files if applicable.

!!! example "Example: 4 audio bitrates and 2 video bitrates"
    ```
    mp4dash audio-16kbps.mp4 audio_32kbps.mp4 audio-64kbps.mp4 audio-128kbps.mp4 video-500kbps.mp4 video-2000kbps.mp4
    ```

# xHE-AAC with HLS

With HLS, more precise control may be needed. With HLS, every possible combination of audio and video streams must be listed explicitly in the playlist (as opposed to an MPEG DASH manifest, where the audio and video streams are listed separately in the manifest, and it is up to the player to pick the right combination of audio and video streams).

By default, the `mp4dash` packager, when creating the HLS playlist, will list all possible combinations of audio and video streams. This means that if you have N audio streams and M video streams, the HLS playlist will have N*M `#EXT-X-STREAM-INF` entries, one for each possible (audio,video) pair.
If you need more control over the different combinations listed in the playlist, you can use the `hls_group` and `hls_group_match` input specifiers. The `hls_group` specifier for audio input files assigns a group name to the audio track. The `hls_group_match` specifier for video input files specifies which should be combined with that video track If `hls_group_match` is not set, or set to `*`, all audio groups will be combined with the video track. For an explicit list of audio groups that should be combined with a video stream, the `hls_group_match` specifier can be set to a list of group names, separated by `&`.

!!! example "Example: audio-only, 4 bitrates"
    ```
    mp4dash --hls audio-16kbps.mp4 audio-32kbps.mp4 audio-64kbps.mp4 audio-128kbps.mp4
    ```

!!! example "Example: 4 audio bitrates, 1 video bitrate"
    ```
    mp4dash --hls audio-16kbps.mp4 audio-32kbps.mp4 audio-64kbps.mp4 audio-128kbps.mp4 video_00500.mp4
    ```
    The playlist will have 4 `#EXT-X-STREAM-INF` entries

!!! example "Example: 4 audio bitrates, 3 video bitrates, all combinations listed"
    ```
    mp4dash --hls audio-16kbps.mp4 audio-32kbps.mp4 audio-64kbps.mp4 audio-128kbps.mp4 video_00500.mp4 video_00800.mp4 video_01400.mp4
    ```
    The playlist will have 12 `#EXT-X-STREAM-INF` entries

!!! example "Example: 4 audio bitrates, 3 video bitrates, with only the lowest bitrate video combined with all audio bitrates"
    ```
    mp4dash --hls [+hls_group=audio_16]audio-16kbps.mp4 [+hls_group=audio_32]audio-32kbps.mp4 [+hls_group=audio_64]audio-64kbps.mp4 [+hls_group=audio_128]audio-128kbps.mp4 "[+hls_group_match=*]video_00500.mp4" "[+hls_group_match=audio128]video_00800.mp4" "[+hls_group_match=audio128]video_01400.mp4"
    ```
    The playlist will have 6 `#EXT-X-STREAM-INF` entries (4 with the first video bitrate, and one with each of the other two)


!!! example "Example: 4 audio bitrates, 3 video bitrates, with only the lowest bitrate video combined with 3 audio bitrates"
    ```
    mp4dash --hls [+hls_group=audio_16]audio-16kbps.mp4 [+hls_group=audio_32]audio-32kbps.mp4 [+hls_group=audio_64]audio-64kbps.mp4 [+hls_group=audio_128]audio-128kbps.mp4 "[+hls_group_match=audio_16&audio_32&audio_64&audio_64]video_00500.mp4" "[+hls_group_match=audio128]video_00800.mp4" "[+hls_group_match=audio128]video_01400.mp4"
    ```
    The playlist will have 5 `#EXT-X-STREAM-INF` entries (3 with the first video bitrate, and one with each of the other two)


