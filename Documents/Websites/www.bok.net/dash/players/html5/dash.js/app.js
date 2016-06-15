// Convert a hex string to a byte array
function hexToBytes(hex) {
    for (var bytes = [], c = 0; c < hex.length; c += 2)
         bytes.push(parseInt(hex.substr(c, 2), 16));
    return bytes;
}

// Convert a byte array to a base-64 string
function bytesToBase64(bytes) {
    var base64map = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"; // URL-safe alphabet
    for(var base64 = [], i = 0; i < bytes.length; i += 3) {
        var triplet = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2];
        for (var j = 0; j < 4; j++) {
            if (i * 8 + j * 6 <= bytes.length * 8) {
                base64.push(base64map.charAt((triplet >>> 6 * (3 - j)) & 0x3F));
            } else {
                base64.push("=");
            }
        }
    }

    return base64.join("");
}

function log(msg) {
    var log_pane = document.getElementById('log');
    log_pane.appendChild(document.createTextNode(msg));
    log_pane.appendChild(document.createElement("br"));
    console.log(msg);
}

var app = function() {};

app.init = function() {
  var mpdList = document.getElementById('mpdList');
  for (var i=0; i<Presets.length; i++) {
      var preset = Presets[i];
      var option = document.createElement('option');
      option.textContent = preset.title;
      option.setAttribute('value', preset.url);
      mpdList.appendChild(option);
  }
  option = document.createElement('option');
  option.textContent = '(custom)';
  option.setAttribute('value', '');
  mpdList.appendChild(option);

  app.detectEmeDrms();
  app.onMpdChanged();
  app.onDrmConfigChanged();
};

app.detectEmeDrms = function() {
    var knownDrms = ['org.w3.clearkey', 'com.widevine.alpha', 'com.microsoft.playready', 'com.adobe.primetime', 'com.apple.fps.1_0', 'com.apple.fps.2_0', 'com.apple.fps.2_0'];

    var responseCount = 0;
    var self = this;
    function onRequestMediaKeySystemAccessResponse() {
        if (++responseCount == knownDrms.length) {
            document.getElementById('supportedDrms').innerHTML = self.supportedDrms.join(', ');
        }
    }
    var video = document.querySelector("#videoPlayer");
    this.supportedDrms = [];
    if (video.msSetMediaKeys && (typeof video.msSetMediaKeys == 'function')) {
        // IE
        knownDrms = ['com.microsoft.playready'];
        self.supportedDrms = knownDrms;
        onRequestMediaKeySystemAccessResponse();
    } else if (typeof navigator.requestMediaKeySystemAccess === 'function') {
        for (var i=0; i<knownDrms.length; i++) {
            navigator.requestMediaKeySystemAccess(knownDrms[i], [ { } ]).then(function(mediaKeySystemAccess) {
                self.supportedDrms.push(mediaKeySystemAccess.keySystem);
                onRequestMediaKeySystemAccessResponse();
            }).catch(function(){onRequestMediaKeySystemAccessResponse();});
        }
    }
}

app.onMpdChanged = function() {
  var options = document.getElementById('mpdList');
  var preset = Presets[options.selectedIndex];
  document.getElementById('manifestUrlInput').value = preset.url;
  document.getElementById('kid').value = preset.kid ? preset.kid : '';
  document.getElementById('key').value = preset.key ? preset.key : '';
  document.getElementById('licenseUrl').value = preset.licenseUrl ? preset.licenseUrl : '';
  if (preset.emeConfig == 'manual') {
      document.getElementById('drmConfigList').selectedIndex = 2;
  } else if (preset.emeConfig == 'override-url') {
      document.getElementById('drmConfigList').selectedIndex = 1;
  } else {
      document.getElementById('drmConfigList').selectedIndex = 0;
  }
  this.onDrmConfigChanged();
};

app.onMpdCustom = function() {
  document.getElementById('mpdList').value = '';
};

app.onDrmConfigChanged = function() {
    var drmConfig = document.getElementById('drmConfigList').value;
    document.getElementById('clearkey-drm-config').style.display = "none";
    document.getElementById('override-drm-config').style.display = "none";
    if (drmConfig == "clearkey-override") {
        document.getElementById('clearkey-drm-config').style.display = "inherit";
    } else if (drmConfig == "override-url") {
        document.getElementById('override-drm-config').style.display = "inherit";
    }
}

app.onPlayerErrorEvent = function(event) {
    log(event.event);
}

app.onPlayerLogEvent = function(event) {
    //console.log(event);
}

app.loadStream = function() {
    if (!app.player) {
        app.player = new MediaPlayer(new Dash.di.DashContext());
        app.player.startup();
        app.player.addEventListener(MediaPlayer.events.ERROR, app.onPlayerErrorEvent);
        app.player.addEventListener(MediaPlayer.events.LOG, app.onPlayerLogEvent);
        app.player.attachView(document.querySelector("#videoPlayer"));
    }
    var drmConfig = document.getElementById('drmConfigList').value;
    var protectionData = {};
    if (drmConfig == 'override-url') {
        var drmIds = ['com.widevine.alpha', 'com.microsoft.playready'];
        drmIds.forEach(function(drmId) {
           protectionData[drmId] = {};
        });
        var licenseUrl = document.getElementById('licenseUrl').value;
        if (licenseUrl) {
          drmIds.forEach(function(drmId) {
             protectionData[drmId].serverURL = licenseUrl;
          });
        }
        var httpHeaders = document.getElementById('httpHeaders').value;
        if (httpHeaders) {
            var extraHeaders = {}
            var pairs = httpHeaders.split(';');
            pairs.forEach(function(header) {
                var pair = header.split(':');
                if (pair.length == 2) {
                    extraHeaders[pair[0].trim()] = pair[1].trim();
                }
            });
            drmIds.forEach(function(drmId) {
               protectionData[drmId].httpRequestHeaders = extraHeaders;
            });
        }
    } else if (drmConfig == 'clearkey-override') {
        var kid = bytesToBase64(hexToBytes(document.getElementById('kid').value)).substr(0, 22); // base64, no padding
        var key = bytesToBase64(hexToBytes(document.getElementById('key').value)).substr(0, 22); // base64, no padding
        if (kid && key) {
          var clearKeysField = {}
          clearKeysField[kid] = key;
          protectionData['org.w3.clearkey'] = {
            'clearkeys': clearKeysField
          }
        }
    } else {
        protectionData['com.widevine.alpha'] = {
            // default for testing
            serverURL: 'https://widevine-proxy.appspot.com/proxy'
        }
    }

    var mediaUrl = document.getElementById('manifestUrlInput').value;
    app.player.attachSource(mediaUrl, null, protectionData);
}

if (document.readyState == 'complete' || document.readyState == 'interactive') {
  app.init();
} else {
  document.addEventListener('DOMContentLoaded', app.init);
}
