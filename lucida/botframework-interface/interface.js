/*-----------------------------------------------------------------------------
Filename    : interface.js
Author      : Kamal Galrani
Description : This file handles dialogs from bot framework, forwards messages
		to Lucida and forwards response to users
-----------------------------------------------------------------------------*/

var restify = require('restify')
var server = restify.createServer()
var builder = require('botbuilder')
var calling = require('botbuilder-calling')
var request = require('request')
var credentials = require('./credentials')
var url = require('url')
var bfw_port
var cc_api_host
var cc_ws_host
var WebSocket = require('ws')

var util = require('util')

//=========================================================
// Bot Setup
//=========================================================

function check_bfw_port(str_port) {
  port = parseInt(str_port)
  if ( ! isNaN(port) && port.toString() === str_port ) {
    if ( port < 1 || port > 65535 ) {
      console.log("[ERROR] Port should be a number between 1 and 65535")
      return false
    }
    // Setup Restify Server
    server.listen(port, function () {
      console.log("[INFO] Listening to bot channels on port " + port + "...")
    })
    return true
  }
  return false
}

function check_cc_host(host) {
  match = host.match(/^((\d+\.\d+\.\d+\.\d+)|localhost)(:\d+)?(.*)?$/)
  if ( match != null ) {
    cc_api_host = "http://" + host.replace(/\/$/, "") + '/api'
    cc_ws_host = "ws://" + host.replace(/\/$/, "") + '/ws'
    console.log("[INFO] Remote command center API host is set to " + cc_api_host)
    console.log("[INFO] Remote command center WS host is set to " + cc_ws_host)
    return true
  } else if ( url.parse(host)['host'] != null ) {
    cc_api_host = host.replace(/\/$/, "") + '/api'
    cc_ws_host = cc_api_host.replace(/^http/, "ws") + '/ws'
    console.log("[INFO] Remote command center API host is set to " + cc_api_host)
    console.log("[INFO] Remote command center WS host is set to " + cc_ws_host)
    return true
  } else {
    return false
  }
}

function check_args() {
  args = process.argv.slice(2)
  if (args.length == 0) {
    console.log("[WARN] Neither interface port nor command center host specified. Using defaults...")
    check_bfw_port("3728")
    check_cc_host("http://localhost:3000")
  } else if (args.length == 1) {
    if ( check_bfw_port(args[0]) ) {
      console.log("[WARN] No command center host specified. Using default...")
      check_cc_host("http://localhost:3000")
    } else if ( check_cc_host(args[0]) ) {
      console.log("[WARN] No interface port specified. Using default...")
      check_bfw_port("3728")
    } else {
      console.log("[ERROR] Argument provided is neither a valid interface port nor a valid command center host.")
      process.exit()
    }
  } else if (args.length == 2) {
    if ( ! ( ( check_bfw_port(args[0]) && check_cc_host(args[1]) ) || ( check_bfw_port(args[1]) && check_cc_host(args[0]) ) ) ) {
      console.log("[ERROR] Either an invalid interface port or an invalid command center host is provided.")
      process.exit()
    }
  } else {
    console.log("[ERROR] Usage: node interface.js <interface_port> <command_center_host>")
    process.exit()
  }
}
check_args()

// Create web socket
var ws = new WebSocket(cc_ws_host + '/status')

ws.on('message', function incoming(data) {
  console.log("WS RECVD: " + data);
});

// Create chat bot
var chat_connector = new builder.ChatConnector(credentials.chat_credentials)
var chat_bot = new builder.UniversalBot(chat_connector)
server.post('/api/messages', chat_connector.listen())
var call_connector = new calling.CallConnector(credentials.call_credentials)
var call_bot = new calling.UniversalCallBot(call_connector);
server.post('/api/calls', call_connector.listen());

//=========================================================
// Bots Dialogs
//=========================================================

var chat_addresses = {}

chat_bot.dialog('/', [
  function (session) {
    console.log(util.inspect(session.message.address))
    var address   = session.message.address
    chat_addresses[address.channelId] = {channelId: address.channelId, bot: {id: address.bot.id, name: address.bot.name}, serviceUrl: address.serviceUrl, useAuth: address.useAuth}
    request.post({
      headers: {'content-type' : 'application/x-www-form-urlencoded'},
      form:    { interface: session.message.address.channelId, username: session.message.address.user.id, text_input: session.message.text },
      url:     cc_api_host + '/infer',
      form:    { interface: session.message.address.channelId, username: session.message.address.user.id, speech_input: session.message.text }
    }, function(error, response, body){
      address = chat_addresses[session.message.address.channelId]
      address['user'] = { id: session.message.address.user.id }
      if (error) {
        text = "Error occured '" + error.code + "'!!! Is command center running?"
      } else if ( response.statusCode == 200 ) {
        body = JSON.parse(body)
        text = body.result
      } else if ( response.statusCode == 403 ) {
        var regex = /^Verify ([0-9a-zA-Z-_\.]*)$/
        var result = session.message.text.match(regex)
        if ( result ) {
          request.post({
            headers: {'content-type' : 'application/x-www-form-urlencoded'},
            url:     cc_api_host + '/add_interface',
            form:    { interface: session.message.address.channelId, token: result[1], username: session.message.address.user.id }
          }, function(error, response, body){
            if (error) {
              text = "Error occured '" + error.code + "'!!! Is command center running?"
            } else if ( response.statusCode == 200 ) {
              text = body
            } else if ( response.statusCode == 401 ) {
              text = "Token expired... Please regenerate token on the web interface"
            } else if ( response.statusCode == 403 ) {
              text = "You must be kidding me"
            } else {
              text = response.statusCode + " " + response.statusMessage + " received. Go through the logs and figure. Otherwise create an issue on github with logs attached."
            }
            var reply = new builder.Message().address(address).text(text)
            chat_bot.send(reply)
          })
          return
        } else {
          text = "You are not authorized on this interface. Log on to the web interface, click on your email id and follow the instructions there."
        }
      } else if ( response.statusCode == 500 ) {
        text = "Internal server error occured!!! Are all required microservices running?"
      } else {
        text = response.statusCode + " " + response.statusMessage + " received. Go through the logs and figure. Otherwise create an issue on github with logs attached."
      }
      var reply = new builder.Message().address(address).text(text)
      chat_bot.send(reply)
    })
  }
])


call_bot.dialog('/', [
    function (session) {
        session.send('Hey there! How can I help you?');
        console.log(util.inspect(session.message))
    }
])
