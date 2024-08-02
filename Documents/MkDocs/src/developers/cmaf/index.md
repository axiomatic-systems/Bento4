CMAF
====

The Common Media Application Format ([CMAF](https://developer.apple.com/documentation/http_live_streaming/about_the_common_media_application_format_with_http_live_streaming)) is a standard for the encoding and packaging of segmented media objects for adaptive streaming. CMAF unifies MPEG DASH and HLS in a way that allows the authoring of an MPEG DASH manifest and an HLS playlist that point to shared media segments, thus having just one set of encoded segment files.

The `mp4dash` tool supports creating such HLS + DASH presentations, using the `--hls` command-line option. See the [MPEG DASH page](../dash/index.md) for details
