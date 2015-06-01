/**
 * Copyright 2014 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @fileoverview Implements the application layer of the test application.
 */


/** @class */
var app = function() {};

app.video_ = null;
app.player_ = null;
app.polyfillsInstalled_ = false;


/**
 * Initializes the application.
 */
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
    app.installPolyfills_();
    app.initPlayer_();
  }

  var mediaUrl = document.getElementById('manifestUrlInput').value;

  app.load_(
      new shaka.player.DashVideoSource(
          mediaUrl,
          app.interpretContentProtection_));
};

/**
 * Loads the given video source into the player.
 * @param {!shaka.player.IVideoSource} videoSource
 * @private
 */
app.load_ = function(videoSource) {
  console.assert(app.player_ != null);

  app.player_.load(videoSource);
};

/**
 * Requests fullscreen mode.
 */
app.requestFullscreen = function() {
  if (app.player_) {
    app.player_.requestFullscreen();
  }
};


/**
 * Installs the polyfills if the have not yet been installed.
 * @private
 */
app.installPolyfills_ = function() {
  if (app.polyfillsInstalled_)
    return;

  shaka.polyfill.Fullscreen.install();
  shaka.polyfill.MediaKeys.install();
  shaka.polyfill.VideoPlaybackQuality.install();

  app.polyfillsInstalled_ = true;
};


/**
 * Initializes the Player instance.
 * If the Player instance already exists then it is reinitialized.
 * @private
 */
app.initPlayer_ = function() {
  console.assert(app.player_ == null);
  if (app.player_) {
    return;
  }

  app.player_ =
      new shaka.player.Player(/** @type {!HTMLVideoElement} */ (app.video_));
  app.player_.addEventListener('error', app.onPlayerError_, false);
  app.player_.enableAdaptation(true);
  
};


/**
 * Called when the player generates an error.
 * @param {!Event} event
 * @private
 */
app.onPlayerError_ = function(event) {
  console.error('Player error', event);
};


/**
 * Called to interpret ContentProtection elements from the MPD.
 * @param {!shaka.dash.mpd.ContentProtection} contentProtection The MPD element.
 * @return {shaka.player.DrmSchemeInfo} or null if the element is not
 *     understood by this application.
 * @private
 */
app.interpretContentProtection_ = function(contentProtection) {
  var Uint8ArrayUtils = shaka.util.Uint8ArrayUtils;

  if (contentProtection.schemeIdUri ==
      'urn:uuid:edef8ba9-79d6-4ace-a3c8-27dcd51d21ed') {
    var licenseServerUrl = '//widevine-proxy.appspot.com/proxy';
    return new shaka.player.DrmSchemeInfo(
        'com.widevine.alpha', licenseServerUrl, false, null, null);
  }

  var clearkey = document.getElementById('clearKeyInput');
  if (clearkey.value) {
    var Uint8ArrayUtils = shaka.util.Uint8ArrayUtils;

    var keyid = Uint8ArrayUtils.fromHex(clearkey.value.substring(0,32));
    var key   = Uint8ArrayUtils.fromHex(clearkey.value.substring(33));
    console.log("Clear Key: ", keyid, key);

    var keyObj = {
      kty: 'oct',
      alg: 'A128KW',
      kid: Uint8ArrayUtils.toBase64(keyid, false),
      k: Uint8ArrayUtils.toBase64(key, false)
    };
    var jwkSet = {keys: [keyObj]};
    var license = JSON.stringify(jwkSet);
    var initData = {
      initData: keyid,
      initDataType: 'cenc'
    };
    var licenseServerUrl = 'data:application/json;base64,' +
        window.btoa(license);
    return new shaka.player.DrmSchemeInfo(
        'org.w3.clearkey', false, licenseServerUrl, false, initData, null);
  }

  console.warn('Unrecognized scheme: ' + contentProtection.schemeIdUri);
  return null;
};


if (document.readyState == 'complete' ||
    document.readyState == 'interactive') {
  app.init();
} else {
  document.addEventListener('DOMContentLoaded', app.init);
}
