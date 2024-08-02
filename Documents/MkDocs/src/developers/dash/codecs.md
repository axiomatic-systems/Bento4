MPEG DASH & CODECS
==================

An MPEG DASH presentation includes one more media steams, each containing encoded media. The specifics of how the media is encoded is referred to as a "codec" (COder DECoder). The Bento4 tools support a large number of codecs, most of which are handled in the same way, but some may have codec-specific options.

# Video Codecs

The two most most common codecs you are likely to work with are H.264 (a.k.a AVC, or "Advanced Video Coding"), and H.265 (a.k.a HEVC, or "High Efficiency Video Coding"). A more recent addition to the codec landscape is the AV1 codec ("AOMedia Video 1"). In all cases, the input files for the `mp4dash` packager are in MP4 format, which includes codec metadata and media samples.

## HDR

Some video streams may include extensions that encode extra information that allow the video to be displayed with a higher dynamic range. This is often called HDR ("High Dynamic Range"). The most common extensions/formats are Dolby Vision and HDR10.

### Dolby Vision

### HDR10

# Audio Codecs

The most common audio codec for MP4 files is AAC ("Advanced Audio Coding"), including it different "modes": HE-AAC v1 and HE-AAC v2 ("High-Efficiency AAC"), and more recently xHE-AAC ("Extended High Efficiency AAC").

## Multi Channel Audio
Some codecs support encoding more than two channels. Multi-channel streams are often labled as X.Y, where X is the number of full-bandwidth audio channels, and Y is the number of low-frequency channels, like, for example "5.1".
While AAC does have multi-channel support, it is much more common to see Dolby or DTS codecs used for multi-channel audio. This includes Dolby AC-3 (a.k.a "Dolby Digital"), Dolby E-AC-3 (a.k.a "Dolby Digital Plus"), Dolby AC-4 and Dolby Atmos.

Because not all players/decoders support all codecs, it is usually recommended to always include at least one AAC audio stream in an MPEG DASH presentation, which all players should be able to handle.

## Multi Bitrate Audio
See the [Multi Bitrate Audio page](multi_bitrate_audio.md)