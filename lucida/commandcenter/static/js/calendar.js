var access_token = ''; // used to call Google APIs

// Client ID can be retrieved from project in the Google
// Developer Console, https://console.developers.google.com
var CLIENT_ID = '888626176723-b7u0q9fgqvqsujf7q76qvta5glrv59oq.apps.googleusercontent.com';

var SCOPES = ["https://www.googleapis.com/auth/calendar.readonly"];

/**
* Check if current user has authorized this application.
*/
function checkAuth() {
	gapi.auth.authorize(
		{
			'client_id': CLIENT_ID,
			'scope': SCOPES.join(' '),
			'immediate': true
		}, handleAuthResult);
}

/**
 * Initiate auth flow in response to user clicking authorize button.
 */
function getEventsMain() {
	gapi.auth.authorize(
		{client_id: CLIENT_ID, scope: SCOPES, immediate: false},
	handleAuthResult);
	return false;
}

/**
 * Handle response from authorization server.
 *
 * @param {Object} authResult Authorization result.
 */
function handleAuthResult(authResult) {
	if (authResult && !authResult.error) {
		// Load client library.
		access_token = authResult.access_token
		loadCalendarApi();
	} else {
		


	}
}

/**
 * Load Google Calendar client library. List upcoming events
 * once client library is loaded.
 */
function loadCalendarApi() {
	gapi.client.load('calendar', 'v3', listUpcomingEvents);
}

/**
 * Return the time zone offset in the form of "-04:00".
 */
 function getTimeZoneOffset() {
 	function pad(number, length){
		var str = "" + number
		while (str.length < length) {
			str = "0" + str
		}
		return str
	}
	var offset = new Date().getTimezoneOffset();
	return ((offset < 0 ? "+" : "-") +
		pad(parseInt(Math.abs(offset / 60)), 2) + ":" +
		pad(Math.abs(offset % 60), 2));
 }


/**
 * Print the summary and start datetime/date of the next ten events in
 * the authorized user's calendar. If no events are found an
 * appropriate message is printed.
 */
function listUpcomingEvents() {
	var dates = $('#dates').data().name;
	var min = dates.split(' ')[0];
	var max = dates.split(' ')[1];
	var options = {
		'calendarId': 'primary',        
		'showDeleted': false,
		'singleEvents': true,
		'maxResults': 10,
		'orderBy': 'startTime'
	}
	var zone_offset = getTimeZoneOffset();
	if (min !== 'null') {
		options['timeMin'] = min + zone_offset;
		console.log(options['timeMin']);
	}
	if (max !== 'null') {
		options['timeMax'] = max + zone_offset;
		console.log(options['timeMax']);
	}
	if (min === 'null' && max === 'null') {
		// Just return the upcoming 10 events.
		options['timeMin'] = (new Date()).toISOString();
	}
	var request = gapi.client.calendar.events.list(options);
	request.execute(function(resp) {
		var events = resp.items;
		appendMsg('Google calendar events:');
		if (events !== undefined && events.length > 0) {
		  for (i = 0; i < events.length; i++) {
			var event = events[i];
			var start_time = event.start.dateTime;
			if (!start_time) {
			  start_time = event.start.date;
			}
			var end_time = event.end.dateTime;
			if (!end_time) {
			  end_time = event.end.date;
			}
			appendMsg((i + 1) + '. ' + event.summary + ' (' + start_time + ' -- ' + end_time + ')')
		  }
		} else {
		  appendMsg('No events.');
		}
		textToVoice(document.getElementById('clinc').value);
	});
	// Revoke access given to Lucida.
	$.ajax({
	type: 'GET',
	url: 'https://accounts.google.com/o/oauth2/revoke?token=' + access_token,
	async: false,
	contentType: "application/json",
	dataType: 'jsonp',
	success: function(nullResponse) {},
	error: function(e) {}
	});
}

/**
 * Append a message to the text area 'clinc'.
 *
 * @param {string} message Text to be placed in the text area.
 */
function appendMsg(message) {
	document.getElementById('clinc').value += (message + '\n');
}

function watch() { 
	function callback() {
		if (gapi.auth !== undefined) {
		  clearInterval(watcher)
			getEventsMain();
		}
	}
	return callback;
}

var watcher = setInterval(watch(), 100); // check gapi.auth every 0.1 sec
