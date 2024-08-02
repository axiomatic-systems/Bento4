HTTP LIVE STREAMING (HLS)
=========================

The Bento4 tools support HLS in several ways

# HLS With MP4


# HLS With MPEG2 TS

## Using `mp4hls`
`mp4hls` is a high level tool that facilitates the use of the lower-level tool `mp42hls`. `mp4hls` will invoke the `mp42hls` tool, once for each input, in order to create a multi-bitrate HLS presentation.

!!! tip "Use `mp4dash` for HLS with MP4 segments (a.k.a `fMP4`)"
    The `mp4hls` tool only supports creating "legacy" HLS presentations, where the media segments are in MPEG2 TS format. Unless it isn't supported by the players you are targetting, we recommend using HLS with fMP4 segments, as this is compatible with MPEG DASH, and likely what is expected by modern environments.
    See the [MPEG DASH page](../dash/index.md) for details on how to use the `mp4dash` tool, which supports both MPEG DASH and HLS

See the [`mp4hls` documenation](../../documentation/mp4hls.md) for details on how to use the tool.

## Using `mp42hls`
`mp42hls` is the low-level tool that can create an HLS output for a single MP4 input file.
See the [`mp42hls` documentation](../../documentation/mp42hls.md) for details on how to use the tool.