var app = function() {};

app.init = function() {
  app.onMpdChange();
};

app.onMpdChange = function() {
  document.getElementById('manifestUrlInput').value = document.getElementById('mpdList').value;
};

app.onMpdCustom = function() {
  document.getElementById('mpdList').value = '';
};

app.loadStream = function() {
    if (!app.player) {
        app.player = new MediaPlayer(new Dash.di.DashContext());
        app.player.startup();
        app.player.attachView(document.querySelector("#videoPlayer"));
    }
    var protectionData = {
    };
    var laUrl = document.getElementById('laUrlInput').value;
    if (laUrl) {
      protectionData['com.widevine.alpha'] = {
        'serverURL': laUrl
      }
      protectionData['com.microsoft.playready'] = {
        'serverURL': laUrl
      }
    } else {
      protectionData['com.widevine.alpha'] = {
          'serverURL': 'https://widevine-proxy.appspot.com/proxy'
      }
    }

    var clearKey = document.getElementById('clearKeyInput').value;
    if (clearKey) {
      var clearKeys = clearKey.split(':');
      var clearKeysField = {}
      clearKeysField[clearKeys[0]] = clearKeys[1];
      protectionData['org.w3.clearkey'] = {
        'clearkeys': clearKeysField
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
