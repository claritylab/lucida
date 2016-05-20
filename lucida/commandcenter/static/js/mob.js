// Global UI elements:
//  - log: event log
//  - trans: transcription window

// Global objects:
//  - isConnected: true iff we are connected to a worker
//  - tt: simple structure for managing the list of hypotheses
//  - dictate: dictate object with control methods 'init', 'startListening', ...
//       and event callbacks onResults, onError, ...
var isConnected = false;
var isMicrophoneInitialized = false;

var tt = new Transcription();

var startPosition = 0;
var endPosition = 0;
var doUpper = false;
var doPrependSpace = true;
var textToVoice = true;

function capitaliseFirstLetter(string) {
	return string.charAt(0).toUpperCase() + string.slice(1);
}

function updateDisabledState() {
	var disabled = true;
	// var text = "{{ _('Dikteerimiseks vajuta nuppu') }}";
	// if (!isMicrophoneInitialized) {
	//   disabled = true;
	//   text = "{{ _('Mikrofon initsialiseerimata') }}";
	// } else if (isConnected) {
	//   disabled = false;
	//   text = "{{ _('Räägi...') }}";
	// } else if (numWorkersAvailable == 0) {
	//   disabled = true;
	//   text = "{{ _('Server ülekoormatud või rivist väljas') }}";
	// }
	if (disabled) {
		$("#recbutton").addClass("disabled");
		//$("#helptext").html(text);
	} else {
		$("#recbutton").removeClass("disabled");
		//$("#helptext").html(text);
	}
}

function prettyfyHyp(text, doCapFirst, doPrependSpace) {
	if (doCapFirst) {
		text = capitaliseFirstLetter(text);
	}
	tokens = text.split(" ");
	text = "";
	if (doPrependSpace) {
		text = " ";
	}
	doCapitalizeNext = false;
	tokens.map(function(token) {
		if (text.trim().length > 0) {
			text = text + " ";
		}
		if (doCapitalizeNext) {
			text = text + capitaliseFirstLetter(token);
		} else {
			text = text + token;
		}
		if (token == "." ||  /\n$/.test(token)) {							
			doCapitalizeNext = true;
		} else {
			doCapitalizeNext = false;
		}						
	});
	
	text = text.replace(/ ([,.!?:;])/g,  "\$1");
	text = text.replace(/ ?\n ?/g,  "\n");
	return text;
}	

var dictate = new Dictate({
  server : $("#servers").val().split('|')[0],
	serverStatus : $("#servers").val().split('|')[1],
	recorderWorkerPath : 'static/js/recorderWorker.js',
	onReadyForSpeech : function() {
		isConnected = true;
		__message("READY FOR SPEECH");
		$("#buttonToggleListening").html('Stop');
		$("#buttonToggleListening").addClass('highlight');
		$("#buttonToggleListening").prop("disabled", false);
		$("#buttonCancel").prop("disabled", false);
		$("#recbutton").addClass("playing");
		startPosition = $("#trans").prop("selectionStart");
		endPosition = startPosition;
		var textBeforeCaret = $("#trans").val().slice(0, startPosition);
		if ((textBeforeCaret.length == 0) || /\. *$/.test(textBeforeCaret) ||  /\n *$/.test(textBeforeCaret)) {
		doUpper = true;
		} else {
		doUpper = false;
		}
		doPrependSpace = (textBeforeCaret.length > 0) && !(/\n *$/.test(textBeforeCaret));
	},
	onEndOfSpeech : function() {
		__message("END OF SPEECH");
		$("#buttonToggleListening").html('Stopping...');
		$("#buttonToggleListening").prop("disabled", true);
	},
	onEndOfSession : function() {
		isConnected = false;
		__message("END OF SESSION");
		$("#buttonToggleListening").html('Start');
		$("#buttonToggleListening").removeClass('highlight');
		$("#buttonToggleListening").prop("disabled", false);
		$("#buttonCancel").prop("disabled", true);
		$("#recbutton").removeClass("playing");
	},
	onServerStatus : function(json) {
		__serverStatus(json.num_workers_available);
		$("#serverStatusBar").toggleClass("highlight", json.num_workers_available == 0);
		// If there are no workers and we are currently not connected
		// then disable the Start/Stop button.
		if (json.num_workers_available == 0 && ! isConnected) {
		$("#buttonToggleListening").prop("disabled", true);
		} else {
		$("#buttonToggleListening").prop("disabled", false);
		}
	},
	onPartialResults : function(hypos) {
		hypText = prettyfyHyp(hypos[0].transcript, doUpper, doPrependSpace);
		val = $("#trans").val();
		$("#trans").val(val.slice(0, startPosition) + hypText + val.slice(endPosition));        
		endPosition = startPosition + hypText.length;
		$("#trans").prop("selectionStart", endPosition);
	},
	onResults : function(hypos) {
		hypText = prettyfyHyp(hypos[0].transcript, doUpper, doPrependSpace);
		val = $("#trans").val();
		$("#trans").val(val.slice(0, startPosition) + hypText + val.slice(endPosition));        
		startPosition = startPosition + hypText.length;			
		endPosition = startPosition;
		$("#trans").prop("selectionStart", endPosition);
		if (/\. *$/.test(hypText) ||  /\n *$/.test(hypText)) {
		doUpper = true;
		} else {
		doUpper = false;
		}
		doPrependSpace = (hypText.length > 0) && !(/\n *$/.test(hypText));
	},
	onClinc : function(hypos) {
		hypText = prettyfyHyp(hypos, doUpper, doPrependSpace);
		val = $("#clinc").val();
		$("#clinc").val(val.slice(0, startPosition) + hypText + val.slice(endPosition));
		if (textToVoice){
			speakable = hypText.split('\n')[0];
			var u = new SpeechSynthesisUtterance();
			u.text = speakable;
			u.lang = 'en-US';
			u.rate = 1.0;
			speechSynthesis.speak(u);
		}
		startPosition = startPosition + hypText.length;			
		endPosition = startPosition;
		$("#clinc").prop("selectionStart", endPosition);
		if (/\. *$/.test(hypText) ||  /\n *$/.test(hypText)) {
		doUpper = true;
		} else {
		doUpper = false;
		}
		doPrependSpace = (hypText.length > 0) && !(/\n *$/.test(hypText));
	},
	onError : function(code, data) {
		dictate.cancel();
		__error(code, data);
		// TODO: show error in the GUI
	},
	onEvent : function(code, data) {
		__message(code, data);
			if (code == 3 /* MSG_INIT_RECORDER */) {
				isMicrophoneInitialized = true;
				updateDisabledState();
			}
		}
});

// Private methods (called from the callbacks)
function __message(code, data) {
	log.innerHTML = "msg: " + code + ": " + (data || '') + "\n" + log.innerHTML;
}

function __error(code, data) {
	log.innerHTML = "ERR: " + code + ": " + (data || '') + "\n" + log.innerHTML;
}

function __serverStatus(msg) {
	serverStatusBar.innerHTML = msg;
}

function __updateTranscript(text) {
	$("#trans").val(text);
}

// Public methods (called from the GUI)
function toggleListening() {
	if (isConnected) {
		dictate.stopListening();
		$("#recbutton").addClass("disabled");
	} else {
		dictate.startListening();
	}
}

function cancel() {
	dictate.cancel();
}

function clearTranscription() {	
	$("#trans").val("");
	// needed, otherwise selectionStart will retain its old value
	$("#trans").prop("selectionStart", 0);	
	$("#trans").prop("selectionEnd", 0);	

	$("#clinc").val("");
	// needed, otherwise selectionStart will retain its old value
	$("#clinc").prop("selectionStart", 0);	
	$("#clinc").prop("selectionEnd", 0);	
}

$(document).ready(function() {
  dictate.init();

	$("#servers").change(function() {
		dictate.cancel();
		var servers = $("#servers").val().split('|');
		dictate.setServer(servers[0]);
		dictate.setServerStatus(servers[1]);
	});

});

function readURL(input) {
	if (input.files && input.files[0]) {
		var reader = new FileReader();

		reader.onload = function (e) {
			$('#image_upload_preview').attr('src', e.target.result);
		}

		reader.readAsDataURL(input.files[0]);
	}
}

$("#file_input").change(function () {
	readURL(this);
});
