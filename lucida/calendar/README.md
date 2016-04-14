# Calendar

Calendar is a Lucida service that returns the upcoming (up to 10) events on your Google calendar.

## Calendar Local Development

- From this directory, type: `./gradlew run` (or `./gradlew run -Pargs="8082"` to specify a port number). This will start the server.
- Go to CalendarClient, and type `./compile-Calendar-client.sh` followed by `./start-Calendar-client.sh` (or `./start-Calendar-client.sh 8082` to specify a port number) to compile and run the testing client. You will be prompted to log into your Google account and grant access to this application. After that, credentials will be saved on your computer at home_directory/.credentials/calendar-java.json.
