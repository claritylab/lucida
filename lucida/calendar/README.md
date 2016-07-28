# Calendar

Calendar is a Lucida service that parses the input text query into a date range.
The command center is responsible for sending the date range to front end,
and the front end fetches events from your Google Calendar.
For example, if the input question is `"What was on my Google calendar last year?"`, 
the response of the calendar service may be `"2015-01-01T00:00:00 2015-12-31T23:59:59"`
(based on the current time).

## Calendar Local Development

- From this directory, type `make` to compile, and `make start_server` to start the service. It requires Java 8. 

- From this directory, type `make start_test` to test the running server.
