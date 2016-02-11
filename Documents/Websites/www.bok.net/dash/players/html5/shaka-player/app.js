var app = function() {};

app.video_ = null;
app.player_ = null;

app.init = function() {
  app.video_ =
      /** @type {!HTMLVideoElement} */ (document.getElementById('video'));

  var fields = location.search.split('?').pop();
  fields = fields ? fields.split(';') : [];
  var params = {};
  for (var i = 0; i < fields.length; ++i) {
    var kv = fields[i].split('=');
    params[kv[0]] = kv[1];
  }

  if ('debug' in params && shaka.log) {
    shaka.log.setLevel(shaka.log.Level.DEBUG);
  }
  if ('v' in params && shaka.log) {
    shaka.log.setLevel(shaka.log.Level.V1);
  }

  app.onMpdChange();
};

app.onMpdChange = function() {
  document.getElementById('manifestUrlInput').value = document.getElementById('mpdList').value;
};


app.onMpdCustom = function() {
  document.getElementById('mpdList').value = '';
};

app.loadStream = function() {
  if (!app.player_) {
    shaka.polyfill.installAll();
    app.player_ = new shaka.player.Player(/** @type {!HTMLVideoElement} */ (app.video_));
    app.player_.addEventListener('error', app.onPlayerError_, false);
  }

  var mediaUrl = document.getElementById('manifestUrlInput').value;
  var estimator = new shaka.util.EWMABandwidthEstimator();
  var source = new shaka.player.DashVideoSource(mediaUrl, app.interpretContentProtection_, estimator);
  app.player_.load(source);
};

app.onPlayerError_ = function(event) {
  console.error('Player error', event);
};

app.interpretContentProtection_ = function(schemeIdUri, contentProtection) {
  var Uint8ArrayUtils = shaka.util.Uint8ArrayUtils;

  if (schemeIdUri == 'urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed') {
    var licenseServerUrl = '//widevine-proxy.appspot.com/proxy';
    return [{
        keySystem: 'com.widevine.alpha',
        licenseServerUrl: licenseServerUrl
    }]
  }

  var clearkey = document.getElementById('clearKeyInput');
  if (clearkey.value) {
    var Uint8ArrayUtils = shaka.util.Uint8ArrayUtils;

    var keyId = Uint8ArrayUtils.fromHex(clearkey.value.substring(0,32));
    var key   = Uint8ArrayUtils.fromHex(clearkey.value.substring(33));
    console.log("Clear Key: ", keyId, key);

    var keyObj = {
      kty: 'oct',
      kid: Uint8ArrayUtils.toBase64(keyId, false),
      k: Uint8ArrayUtils.toBase64(key, false)
    };
    var jwkSet = {keys: [keyObj]};
    var license = JSON.stringify(jwkSet);
    var initData = {
      initDataType: 'keyids',
      initData: Uint8ArrayUtils.fromString(JSON.stringify({
          kids: [Uint8ArrayUtils.toBase64(keyId, false)]
      }))
    };
    var licenseServerUrl = 'data:application/json;base64,' + window.btoa(license);
    return [{
        keySystem: 'org.w3.clearkey',
        licenseServerUrl: licenseServerUrl,
        initData: initData
    }]
  }

  console.warn('Unrecognized scheme: ' + schemeIdUri);
  return null;
};

if (document.readyState == 'complete' ||
    document.readyState == 'interactive') {
  app.init();
} else {
  document.addEventListener('DOMContentLoaded', app.init);
}
