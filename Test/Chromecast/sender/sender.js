var APPLICATION_ID = 'B5E83E18'; //'CC1AD845'
var MESSAGE_NAMESPACE = 'urn:x-cast:com.axiosys.sample.mediaplayer';
var currentMedia = null;
var progressFlag = 1;
var mediaCurrentTime = 0;
var session = null;
var initialTimeIndexSeconds = 0;
var timer = null;

if (!chrome.cast || !chrome.cast.isAvailable) {
  setTimeout(initializeCastApi, 1000);
}

function debugLog(message) {
  var dw = document.getElementById("debugmessage");
  dw.innerHTML += '\n' + JSON.stringify(message);
  return message;
};

function initializeCastApi() {
  var sessionRequest = new chrome.cast.SessionRequest(APPLICATION_ID);
  var apiConfig = new chrome.cast.ApiConfig(sessionRequest, sessionListener, receiverListener);

  chrome.cast.initialize(apiConfig, onInitSuccess, onError);
};

function onInitSuccess() {
  debugLog("init success");
}

function onError() {
  debugLog("error");
}

function onSuccess(message) {
  debugLog(message);
}

function onStopAppSuccess() {
  debugLog('session stopped');
}

function sessionListener(e) {
  debugLog('New session ID:' + e.sessionId);
  session = e;
  if (session.media.length != 0) {
    debugLog('Found ' + session.media.length + ' existing media sessions.');
    onMediaDiscovered('sessionListener', session.media[0]);
  }
  session.addMediaListener(onMediaDiscovered.bind(this, 'addMediaListener'));
  session.addUpdateListener(sessionUpdateListener.bind(this));
  session.addMessageListener(MESSAGE_NAMESPACE, onReceiverMessage.bind(this));
}

function onReceiverMessage(namespace, message) {
  debugLog(namespace + ':' + message);
}

function sessionUpdateListener(isAlive) {
  var message = isAlive ? 'Session Updated' : 'Session Removed';
  message += ': ' + session.sessionId;
  debugLog(message);
  if (!isAlive) {
    session = null;
    var playpauseresume = document.getElementById("playpauseresume");
    playpauseresume.innerHTML = 'Play';
    if( timer ) {
      clearInterval(timer);
    }
  }
}

function receiverListener(e) {
  debugLog('receiver listener callback: ' + e);
  if (e === chrome.cast.ReceiverAvailability.AVAILABLE) {
    debugLog("receiver found");
  } else {
    debugLog("receiver list empty");
  }
}

function launchApp() {
  debugLog("launching app...");
  chrome.cast.requestSession(onRequestSessionSuccess, onLaunchError);
}

function onRequestSessionSuccess(e) {
  debugLog("session success: " + e.sessionId);
  session = e;
  session.addMessageListener(MESSAGE_NAMESPACE, onReceiverMessage.bind(this));
}

function onLaunchError() {
  debugLog("launch error");
}

function stopApp() {
  session.stop(onStopAppSuccess, onError);
  if (timer) {
    clearInterval(timer);
  }
}

function loadMedia() {
  var mediaUrl = document.getElementById('mediaUrl').value;
  if (mediaUrl.length > 0) {
    loadMediaUrl(mediaUrl);
  }
}

function loadMediaUrl(mediaURL) {
  if (!session) {
    debugLog("no session");
    return;
  }
  var mediaInfo = new chrome.cast.media.MediaInfo(mediaURL);

  mediaInfo.contentType = 'video/mp4';
  var request = new chrome.cast.media.LoadRequest(mediaInfo);
  request.currentTime = initialTimeIndexSeconds;
  request.autoplay = true;

  request.customData = {
    licenseCustomData: document.getElementById('customData').value,
    licenseUrl: document.getElementById('licenseUrl').value
  };

  session.loadMedia(request,
    onMediaDiscovered.bind(this, 'loadMedia'),
    onMediaError);
}

function onMediaDiscovered(how, mediaSession) {
  debugLog("new media session ID:" + mediaSession.mediaSessionId + ' (' + how + ')');
  currentMedia = mediaSession;
  mediaSession.addUpdateListener(onMediaStatusUpdate);
  mediaCurrentTime = currentMedia.currentTime;
  playpauseresume.innerHTML = 'Pause';
  if (!timer) {
    timer = setInterval(updateCurrentTime.bind(this), 1000);
  }
}

function onMediaError(e) {
  debugLog("media error");
}

function onMediaStatusUpdate(isAlive) {
  if (progressFlag) {
    document.getElementById("progress").value = parseInt(100 * currentMedia.currentTime / currentMedia.media.duration);
    document.getElementById("mediaposition").innerHTML = currentMedia.currentTime;
    document.getElementById("mediaduration").innerHTML = currentMedia.media.duration;
  }
  document.getElementById("playerstate").innerHTML = currentMedia.playerState;
}

function playMedia() {
  if (!currentMedia) {
    return;
  }

  if (timer) {
    clearInterval(timer);
  }

  var playpauseresume = document.getElementById("playpauseresume");
  if (playpauseresume.innerHTML == 'Play') {
    currentMedia.play(null,
      mediaCommandSuccessCallback.bind(this,"playing started for " + currentMedia.sessionId),
      onError);
      playpauseresume.innerHTML = 'Pause';
      currentMedia.addUpdateListener(onMediaStatusUpdate);
      debugLog("play started");
      timer = setInterval(updateCurrentTime.bind(this), 1000);
  } else if (playpauseresume.innerHTML == 'Pause') {
    currentMedia.pause(null,
      mediaCommandSuccessCallback.bind(this,"paused " + currentMedia.sessionId),
      onError);
    playpauseresume.innerHTML = 'Resume';
    debugLog("paused");
  } else if (playpauseresume.innerHTML == 'Resume') {
    currentMedia.play(null,
      mediaCommandSuccessCallback.bind(this,"resumed " + currentMedia.sessionId),
      onError);
    playpauseresume.innerHTML = 'Pause';
    debugLog("resumed");
    timer = setInterval(updateCurrentTime.bind(this), 1000);
  }
}

function stopMedia() {
  if (!currentMedia) {
    return;
  }

  currentMedia.stop(null,
    mediaCommandSuccessCallback.bind(this,"stopped " + currentMedia.sessionId),
    onError);
  var playpauseresume = document.getElementById("playpauseresume");
  playpauseresume.innerHTML = 'Play';
  debugLog("media stopped");
  if (timer) {
    clearInterval(timer);
  }
}

function seekMedia(pos) {
  debugLog('Seeking ' + currentMedia.sessionId + ':' +
    currentMedia.mediaSessionId + ' to ' + pos + "%");
  progressFlag = 0;
  var request = new chrome.cast.media.SeekRequest();
  request.currentTime = pos * currentMedia.media.duration / 100;
  request.resumeState = chrome.cast.media.PlayerState.PAUSED;
  currentMedia.seek(request,
    onSeekSuccess.bind(this, 'media seek done'),
    onError);
}

function onSeekSuccess(info) {
  debugLog(info);
  setTimeout(function(){progressFlag = 1},1500);
}

function mediaCommandSuccessCallback(info) {
  debugLog(info);
}

function updateCurrentTime() {
  if (!session || !currentMedia) {
    return;
  }

  if (currentMedia.media && currentMedia.media.duration != null) {
    var cTime = currentMedia.getEstimatedTime();
    document.getElementById("progress").value = parseInt(100 * cTime / currentMedia.media.duration);
    document.getElementById("mediaposition").innerHTML = cTime;
  } else {
    document.getElementById("progress").value = 0;
    document.getElementById("mediaposition").innerHTML = 0;
    if( timer ) {
      clearInterval(timer);
    }
  }
}
