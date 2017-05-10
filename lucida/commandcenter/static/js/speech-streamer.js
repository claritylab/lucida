var transcript = '';
var recognizing = false;
var micLoc = 'static/image/';
if (!('webkitSpeechRecognition' in window)) {
    upgrade();
} else {
    var recognition = new webkitSpeechRecognition();
    recognition.continuous = false;
    recognition.interimResults = true;

    recognition.onstart = function() {
        recognizing = true;
        document.getElementById('startImg').src = micLoc + 'microphone_off.png';
    };

    recognition.onerror = function(event) {
        //TODO: Handle errors
    };

    recognition.onend = function() {
        recognizing = false;
        document.getElementById('startImg').src = micLoc + 'microphone.png';
            if (!transcript) {
                return;
            }
    };

    recognition.onresult = function(event) {
        if (typeof(event.results) == 'undefined') {
            recognition.onend = null;
            recognition.stop();
            upgrade();
            return;
        }
        transcript = '';
        for (var i = event.resultIndex; i < event.results.length; ++i) {
            transcript += event.results[i][0].transcript;
        }
        transcript = capitalize(transcript);
        document.getElementById('trans').value = linebreak(transcript);
    };
}

function upgrade() {
    startButton.style.visibility = 'hidden';
}

var twoLine = /\n\n/g;
var oneLine = /\n/g;
function linebreak(s) {
    return s.replace(twoLine, '<p></p>').replace(oneLine, '<br>');
}

var firstChar = /\S/;
function capitalize(s) {
    return s.replace(firstChar, function(m) { return m.toUpperCase(); });
}

function startButtonFunc() {
    if (recognizing) {
        recognition.stop();
        return;
    }
    transcript = '';
    recognition.lang = 'en-US';
    recognition.start();
    document.getElementById('trans').value = '';
    document.getElementById('startImg').src = micLoc + 'microphone.png';
}
