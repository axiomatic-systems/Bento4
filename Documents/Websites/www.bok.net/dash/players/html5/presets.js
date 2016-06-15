var Presets = [
    {
        title: 'Tears Of Steel, isoff-live',
        url: 'http://www.bok.net/dash/tears_of_steel/cleartext/stream.mpd'
    },
    {
        title: 'Tears Of Steel, isoff-live, CENC, Multi-DRM',
        url: 'http://www.bok.net/dash/multi_drm_example/stream.mpd',
        kid: '1c9c394f6cde5668a46f7625e2d3e781',
        key: '39b0d8687fb8f6db36f646caa2a1ea1a'
    },
    {
        title: 'Tears Of Steel, isoff-live, CENC, ClearKey',
        url: 'http://www.bok.net/dash/clearkey_example/stream.mpd',
        kid: '000102030405060708090a0b0c0d0e0f',
        key: '00112233445566778899aabbccddeeff',
        emeConfig: 'manual'
    },
    {
        title: 'Guido Bunny WV',
        url: 'https://s3.amazonaws.com/exp.tutorial/output_bunny/stream.mpd',
        licenseUrl: 'https://wv.test.expressplay.com/hms/wv/rights/?ExpressPlayToken=AQAAAw3paOwAAABQTIL_9kQ0ovZm4rsIOjZosklIDNdmy4olq4L9ivYyzS5RiMqVyPhTQT4Z2lWqQskU4Kn9Lno4H-pQSbJ8NxQiyB3Sb-w0OGBLhYlz29DiPoP8wTJginn-6qHfMsbxv5_03U5FMg',
        emeConfig: 'license-url'
    },
    {
        title: 'Local Bunny ClearKey',
        url: 'http://192.168.1.199:8081/bunny_wv/stream.mpd',
        kid: '12341234123412341234123412341234',
        key: '43215678123412341234123412341234',
        emeConfig: 'manual'
    },
    {
        title: 'Local Bunny WV',
        url: 'http://192.168.1.199:8081/bunny_wv/stream.mpd',
        licenseUrl: 'https://wv.test.expressplay.com/hms/wv/rights/?ExpressPlayToken=AQAAAw3paOwAAABQTIL_9kQ0ovZm4rsIOjZosklIDNdmy4olq4L9ivYyzS5RiMqVyPhTQT4Z2lWqQskU4Kn9Lno4H-pQSbJ8NxQiyB3Sb-w0OGBLhYlz29DiPoP8wTJginn-6qHfMsbxv5_03U5FMg',
        emeConfig: 'license-url'
    },
    {
        title: 'Local Bunny WV - Video Only',
        url: 'http://192.168.1.199:8081/bunny_video_wv/stream.mpd',
        licenseUrl: 'https://wv.test.expressplay.com/hms/wv/rights/?ExpressPlayToken=AQAAAw3paOwAAABQTIL_9kQ0ovZm4rsIOjZosklIDNdmy4olq4L9ivYyzS5RiMqVyPhTQT4Z2lWqQskU4Kn9Lno4H-pQSbJ8NxQiyB3Sb-w0OGBLhYlz29DiPoP8wTJginn-6qHfMsbxv5_03U5FMg',
        emeConfig: 'license-url'
    },
    {
        title: 'Local Bunny WV - FFMPEG',
        url: 'http://192.168.1.199:8081/bunny_ff_wv/stream.mpd',
        licenseUrl: 'https://wv.test.expressplay.com/hms/wv/rights/?ExpressPlayToken=AQAAAw3paOwAAABQTIL_9kQ0ovZm4rsIOjZosklIDNdmy4olq4L9ivYyzS5RiMqVyPhTQT4Z2lWqQskU4Kn9Lno4H-pQSbJ8NxQiyB3Sb-w0OGBLhYlz29DiPoP8wTJginn-6qHfMsbxv5_03U5FMg',
        emeConfig: 'license-url'
    },
    {
        title: 'Local Sintel ClearKey',
        url: 'http://192.168.1.199:8081/sintel_wv/stream.mpd',
        kid: '12341234123412341234123412341234',
        key: '43215678123412341234123412341234',
        emeConfig: 'manual'
    },
    {
        title: 'Local Sintel WV',
        url: 'http://192.168.1.199:8081/sintel_wv/stream.mpd',
        licenseUrl: 'https://wv.test.expressplay.com/hms/wv/rights/?ExpressPlayToken=AQAAAw3paOwAAABQTIL_9kQ0ovZm4rsIOjZosklIDNdmy4olq4L9ivYyzS5RiMqVyPhTQT4Z2lWqQskU4Kn9Lno4H-pQSbJ8NxQiyB3Sb-w0OGBLhYlz29DiPoP8wTJginn-6qHfMsbxv5_03U5FMg',
        emeConfig: 'license-url'
    },
    {
        title: 'Local Mango WV',
        url: 'http://192.168.1.199:8081/tears_wv/stream.mpd',
        licenseUrl: 'https://wv.test.expressplay.com/hms/wv/rights/?ExpressPlayToken=AQAAAw3paOwAAABQTIL_9kQ0ovZm4rsIOjZosklIDNdmy4olq4L9ivYyzS5RiMqVyPhTQT4Z2lWqQskU4Kn9Lno4H-pQSbJ8NxQiyB3Sb-w0OGBLhYlz29DiPoP8wTJginn-6qHfMsbxv5_03U5FMg',
        emeConfig: 'license-url'
    },
    {
        title: 'Local Bunny PR',
        url: 'http://192.168.1.199:8081/bunny_pr/stream.mpd',
        licenseUrl: 'http://pr.test.expressplay.com/playready/RightsManager.asmx?ExpressPlayToken=AQAAAw3pWq4AAABg6n4MVsv33YnWphWSMTHOCPWrKJmUxfMbfQ6Q8OaxvdWo5cuBvYZADKNNUG9gO45Y9IBO2VZk1PU7YQBr-_jE4hBswX6U0iHTZO2lw7dkcprR2eaP-X0dv7hcfkWhshL51vKV-ibDwSPJx5Or-5F-eZqAyWc',
        emeConfig: 'license-url'
    }
];
