MPEG DASH ENCRYPTION AND DRM
============================

# Encrypting The Media

MPEG DASH supports a Common Encryption mode (CENC), which is implemented by Bento4. The simplest way to produce encrypted MPEG DASH streams is to instruct `mp4dash` to perform the encryption for you automatically, by using the `--encryption-key` option. This is the easiest option, as it will internally call the `mp4encrypt` tool with the right parameters. Encrypted MPEG DASH presentations should also include the proper signaling in the MPD to inform the player of what DRM(s) can be used to obtain the decryption keys for the streams. An MPD can contain DRM signaling for several DRMs (either just one or multiple entries if the same stream can reach players with different DRM technologies).

## Marlin DRM

When using Marlin DRM, you may request that the proper signaling be inserted in the MPD by using the `--marlin` option.

!!! example
    With a KID = `121a0fca0f1b475b8910297fa8e0a07e`, an encryption KEY = `a0a1a2a3a4a5a6a7a8a9aaabacadaeaf` you would package a multi-bitrate presentation, with input video files `source_400kbps.mp4` and `source_800kbps.mp4` using a command-line like:

    ```
    mp4dash --marlin --encryption-key=121a0fca0f1b475b8910297fa8e0a07e:a0a1a2a3a4a5a6a7a8a9aaabacadaeaf source_400kbps.mp4 source_800kbps.mp4
    ```

## PlayReady DRM

When using PlayReady DRM, the DRM signaling is in the form of a PlayReady Header Object (PRO). This header object includes fields like the license acquisition URL (LA_URL), the KID, and other fields. There are 2 places where the PRO can be conveyed:

  * a PSSH box in the init segment(s)
  * a `<ContentProtection>` element in the MPD

With most PlayReady DASH clients, it is a requirement that at least the init segment PSSH box be present (when it is not, some players will not correctly detect that the media is encrypted). The Bento4 tools will automatically insert a PlayReady PSSH box in the init segments. If a PRO is found in the MPD, it takes priority over a PRO in an init segment PSSH box. Finally, PlayReady-enabled players often have an API that allows the playback application to override the License Acquisition URL field of the PRO, and/or the CustomData field. The PlayReady Header Object (PRO) can be provided in one of 3 ways, using the `--playready-header` command line option:

  * Stored in a file that contains the binary representation of the PRO
  * Directly as a command-line argument, encoded in Base64
  * By specifying individual fields, like `LA_URL`, and letting the tool construct the binary representation of the PRO.

See the [command line argument help page](../../documentation/mp4dash.md) for details. The most flexible option is to specify the PRO from individual fields.
Typically, only the `LA_URL` field is required, and it is set to the License Acquistion URL for your PlayReady server (consult your DRM provider for details if you use an online DRM service provider).

!!! warning "IMPORTANT NOTE"
    some PlayReady players are not fully compliant with the specification, in that they do not support content encrypted with 16-byte IVs. This is a known implementation defect of clients like SilverLight-based players. In order to be compatible with those players, it is necessary to use a special encryption option that instructs the encrypter to use 8-byte IVs in stead of the default 16-byte IVs. This is done automatically by the Bento4 tool when any of the PlayReady command line options are specified. If you want to encrypt content this way even when not using PlayReady, this can be done using the `--global-option mpeg-cenc.piff-compatible:true` option of `mp4encrypt`, or by simply using the `mp4dash` command line option: `--encryption-args="--global-option mpeg-cenc.piff-compatible:true"`.

!!! example
    With a KID = `09e367028f33436ca5dd60ffe6671e70`, an encryption KEY = `b42ca3172ee4e69bf51848a59db9cd13` and a License Acquisition URL = `http://playready.directtaps.net/pr/svc/rightsmanager.asmx`, you would package a single video file `video-source.mp4` using a command line like:

    ```
    mp4dash --encryption-key=09e367028f33436ca5dd60ffe6671e70:b42ca3172ee4e69bf51848a59db9cd13 --playready-header=LA_URL:http://playready.directtaps.net/pr/svc/rightsmanager.asmx video-source.mp4
    ```

## Widevine DRM

For Widevine, the `--widevine-header` option is used to specify the value of the Widevine DRM data. This can be directly the content of a PSSH box (obtained from a key server for example), expressed as a # sign followed by the hex-encoded PSSH payload data, or as a set of `name:value` pairs.

!!! example
    With a KID = `90351951686b5e1ba222439ecec1f12a`, an encryption KEY = `0a237b0752cbf1a827e2fecfb87479a2`, a Widevine Service Provider ID = `widevine_test`, and a content ID = `*` (2a in hexadecimal), you would package a single video file `video-source.mp4` using a command line like:
    ```
    mp4dash --widevine-header provider:widevine_test#content_id:2a --encryption-key 90351951686b5e1ba222439ecec1f12a:0a237b0752cbf1a827e2fecfb87479a2 video-source.mp4
    ```
    or with an externally-generated PSSH payload:
    ```
    mp4dash --widevine-header "#CAESEJA1GVFoa14boiJDns7B8SoaDXdpZGV2aW5lX3Rlc3QiASo=" --encryption-key 90351951686b5e1ba222439ecec1f12a:0a237b0752cbf1a827e2fecfb87479a2 video-source.mp4
    ```

## FairPlay DRM

## Multiple DRMs at the same time

You can produce a DASH output that is compatible with multiple DRMs at the same time. Simply combine the DRM-specific options documented above.

!!! example  "Example: Marlin, PlayReady and Widevine"
    ```
    mp4dash --encryption-key=09e367028f33436ca5dd60ffe6671e70:b42ca3172ee4e69bf51848a59db9cd13 --marlin --playready-header=LA_URL:http://playready.directtaps.net/pr/svc/rightsmanager.asmx --widevine-header=provider:widevine_test#content_id:2a video-source.mp4
    ```

## Encrypting as a Separate Step

If you need more precise control over encryption (for example only encryption certain tracks and not others, or use special options), you can encrypt the media before using `mp4dash`. This is done by calling `mp4encrypt` directly on the input files before calling `mp4dash`. The `mp4encrypt` command line tool will allow you to encrypt an MP4 video in MPEG DASH CENC mode.

!!! example
    ```
    mp4encrypt --method MPEG-CENC --key 1:a0a1a2a3a4a5a6a7a8a9aaabacadaeaf:0123456789abcdef --property 1:KID:121a0fca0f1b475b8910297fa8e0a07e --key 2:a0a1a2a3a4a5a6a7a8a9aaabacadaeaf:aaaaaaaabbbbbbbb --property 2:KID:121a0fca0f1b475b8910297fa8e0a07e hbb_578kbps.mp4 hbb_578kbps-cenc.mp4
    ```

!!! note
    The source video needs to be a fragmented MP4 file. If your source is not fragmented, you can use `mp4fragment` to convert it to a fragmented file.