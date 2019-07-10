var app = function() {};

app.video = null;
app.player = null;

app.init = function() {
    // Install built-in polyfills to patch browser incompatibilities.
    shaka.polyfill.installAll();

    // Check to see if the browser supports the basic APIs Shaka needs.
    // This is an asynchronous check.
    shaka.Player.probeSupport().then(function(support) {
        app.initPlayer();
    });
}

app.initPlayer = function() {
    // Create a Player instance.
    app.video = /** @type {!HTMLVideoElement} */ document.getElementById('video');
    app.player = new shaka.Player(app.video);

    // Attach player to the window to make it easy to access in the JS console.
    window.player = app.player;

    // Listen for error events.
    app.player.addEventListener('error', app.onPlayerError, false);

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
    this.supportedDrms = [];
    if (app.video.msSetMediaKeys && (typeof app.video.msSetMediaKeys == 'function')) {
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
  } else if (preset.emeConfig == 'license-url') {
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

app.loadStream = function() {
  var mediaUrl = document.getElementById('manifestUrlInput').value;

  // (re)configure the player
  var drmConfig = {
      drm: {
          servers: {
              'com.widevine.alpha': 'https://widevine-proxy.appspot.com/proxy'
          }
      }
  }
  var drmConfigSetttings = document.getElementById('drmConfigList').value;
  if (drmConfigSetttings == 'override-url') {
      var licenseUrl = document.getElementById('licenseUrl').value;
      if (licenseUrl) {
          var drmIds = ['com.widevine.alpha', 'com.microsoft.playready'];
          drmIds.forEach(function(drmId) {
             drmConfig.drm.servers[drmId] = licenseUrl;
          });
      }
  } else if (drmConfigSetttings == 'clearkey-override') {
      drmConfig = {
          drm: {
              clearKeys: {
              }
          }
      }
      drmConfig.drm.clearKeys[document.getElementById('kid').value] = document.getElementById('key').value;
  }
  app.player.configure(drmConfig);

  // Try to load a manifest.
  // This is an asynchronous process.
  player.load(mediaUrl).then(function() {
    // This runs if the asynchronous load is successful.
    console.log('The video has now been loaded!');
  }).catch(app.onPlayerError);  // onError is executed if the asynchronous load fails.
};

app.onPlayerError = function(event) {
  console.error('Player error', event);
}

if (document.readyState == 'complete' ||
    document.readyState == 'interactive') {
  app.init();
} else {
  document.addEventListener('DOMContentLoaded', app.init);
}
