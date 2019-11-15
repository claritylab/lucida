var sendInterval;
var socket;
var input;

// Private methods
function startUserMedia(stream) {
  input = audioContext.createMediaStreamSource(stream);

  // make the analyser available in window context
  window.userSpeechAnalyser = audioContext.createAnalyser();
  input.connect(window.userSpeechAnalyser);

  recorder = new Recorder(input, { workerPath : 'static/js/recorderWorker.js' });
}

startListening = function() {
  if (!recorder) {
    alert("Recorder undefined");
    return;
  }
  socket.emit('stt_control', {command: 'I start recording...'});
  sendInterval = setInterval(function() {
    recorder.export16kMono(function(blob) {
      console.log(blob);
      socket.emit('stt_audio', blob);
      recorder.clear();
    }, 'audio/x-raw');
  }, 250);
  recorder.record();
  clearTranscription();
  document.getElementById('startImg').src = 'static/image/microphone_off.png';
}

stopListening = function() {
  clearInterval(sendInterval);
  sendInterval = undefined;
  if (recorder) {
    recorder.stop();
    recorder.export16kMono(function(blob) {
      socket.emit('stt_audio',blob);
      socket.emit('stt_control', {command: 'I stop recording!!!'});
      recorder.clear();
    }, 'audio/x-raw');
  } else {
    alert("Recorder undefined");
  }
  document.getElementById('startImg').src = 'static/image/microphone.png';
}

function clearTranscription() {
  $("#trans").val("");
  $("#trans").prop("selectionStart", 0);
  $("#trans").prop("selectionEnd", 0);

  $("#clinc").val("");
  $("#clinc").prop("selectionStart", 0);
  $("#clinc").prop("selectionEnd", 0);
}

// Public methods (called from the GUI)
function startButtonFunc() {
  if (sendInterval) {
    stopListening();
  } else {
    startListening();
  }
}

// Document ready function
$(document).ready(function() {
  var audioSourceConstraints = {};
  try {
    window.AudioContext = window.AudioContext || window.webkitAudioContext;
    navigator.getUserMedia = navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia;
    window.URL = window.URL || window.webkitURL;
    audioContext = new AudioContext();
  } catch (e) {
    alert("Error initializing Web Audio browser: " + e);
  }

  if (navigator.getUserMedia) {
//    if(config.audioSourceId) {
//      audioSourceConstraints.audio = { optional: [{ sourceId: config.audioSourceId }] };
//    } else {
      audioSourceConstraints.audio = true;
//    }
    navigator.getUserMedia(audioSourceConstraints, startUserMedia, function(e) { alert("No live audio input in this browser: " + e); });
  } else {
    alert("No user media support");
  }

  socket = io();
  socket.on('stt_status', function(message) {
    console.log(JSON.stringify(message));
  });
});
