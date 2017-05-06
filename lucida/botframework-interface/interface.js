start_time = process.hrtime()
var time_offset = Date.now() / 1000 - start_time[0] - start_time[1] / 1000000000;
delete start_time;

function time() {
  var hr_time = process.hrtime()
  return time_offset + hr_time[0] + hr_time[1] / 1000000000;
}

var restify = require('restify');
var builder = require('botbuilder');
var request = require('request');
var credentials = require('./credentials');

//=========================================================
// Bot Setup
//=========================================================

// Setup Restify Server
var server = restify.createServer();
server.listen(3728, function () {
  console.log("Listening to bot channels on port 3728...");
});

// Create chat bot
var connector = new builder.ChatConnector(credentials.credentials);

var bot = new builder.UniversalBot(connector);
server.post('/api/messages', connector.listen());

//=========================================================
// Bots Dialogs
//=========================================================

var addresses = {};

bot.dialog('/', [
  function (session) {
    var address   = session.message.address;
    addresses[address.channelId] = {channelId: address.channelId, bot: {id: address.bot.id, name: address.bot.name}, serviceUrl: address.serviceUrl, useAuth: address.useAuth};
    request.post({
      headers: {'content-type' : 'application/x-www-form-urlencoded'},
      url:     'http://localhost:3000/api/infer',
      form:    { interface: session.message.address.channelId, username: session.message.address.user.id, text_input: session.message.text }
    }, function(error, response, body){
      address = addresses[session.message.address.channelId];
      address['user'] = { id: session.message.address.user.id };
      if (error) {
        text = "Error occured '" + error.code + "'!!! Is command center running?";
      } else if ( response.statusCode == 200 ) {
        body = JSON.parse(body);
        text = body.result;
      } else if ( response.statusCode == 403 ) {
        var regex = /^Verify ([0-9a-zA-Z-_\.]*)$/
        var result = session.message.text.match(regex);
        if ( result ) {
          request.post({
            headers: {'content-type' : 'application/x-www-form-urlencoded'},
            url:     'http://localhost:3000/api/add_interface',
            form:    { interface: session.message.address.channelId, token: result[1], username: session.message.address.user.id }
          }, function(error, response, body){
            if (error) {
              text = "Error occured '" + error.code + "'!!! Is command center running?";
            } else if ( response.statusCode == 200 ) {
              text = body;
            } else if ( response.statusCode == 401 ) {
              text = "Token expired... Please regenerate token on the web interface";
            } else if ( response.statusCode == 403 ) {
              text = "You must be kidding me";
            } else {
              text = response.statusCode + " " + response.statusMessage + " received. Go through the logs and figure. Otherwise create an issue on github with logs attached.";
            }
            var reply = new builder.Message().address(address).text(text);
            bot.send(reply);
          });
          return;
        } else {
          text = "You are not authorized on this interface. Log on to the web interface, click on your email id and follow the instructions there."
        }
      } else if ( response.statusCode == 500 ) {
        text = "Internal server error occured!!! Are all required microservices running?"
      } else {
        text = response.statusCode + " " + response.statusMessage + " received. Go through the logs and figure. Otherwise create an issue on github with logs attached.";
      }
      var reply = new builder.Message().address(address).text(text);
      bot.send(reply);
      console.log(error);
      console.log(body);
    });
  }
]);
