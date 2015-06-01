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
		// "org.w3.clearkey": {
  //           "clearkeys": {
  //               "IAAAACAAIAAgACAAAAAAAg" : "5t1CjnbMFURBou087OSj2w"
  //           }
  //       },
        "com.widevine.alpha": {
            "laURL": "https://widevine-proxy.appspot.com/proxy"
        }    	
    };
    var mediaUrl = document.getElementById('manifestUrlInput').value;
    app.player.attachSource(mediaUrl, null, protectionData);
}

if (document.readyState == 'complete' || document.readyState == 'interactive') {
  app.init();
} else {
  document.addEventListener('DOMContentLoaded', app.init);
}