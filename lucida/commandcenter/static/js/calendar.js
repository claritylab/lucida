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
	if (min !== 'null') {
		options['timeMin'] = min;
	}
	if (max !== 'null') {
		options['timeMax'] = max;
	}
	if (min === 'null' && max === 'null') {
		// Just return the upcoming 10 events.
		options['timeMin'] = (new Date()).toISOString();
	}
	var request = gapi.client.calendar.events.list(options);
	request.execute(function(resp) {
		var events = resp.items;
		appendMsg('Google calendar events:');
		if (events.length > 0) {
		  for (i = 0; i < events.length; i++) {
			var event = events[i];
			var when = event.start.dateTime;
			if (!when) {
			  when = event.start.date;
			}
			appendMsg(event.summary + ' (' + when + ')')
		  }
		} else {
		  appendMsg('No events.');
		}
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
