var thrift = require('thrift');
var FileTransferSvc = require('./gen-nodejs/FileTransferSvc');
var CCDaemon = require('./gen-nodejs/CommandCenter');
var ttypes = require('./gen-nodejs/commandcenter_types');
var responses = [];

var ftsHandler = {
	ping: function(result) {
		console.log("Ping");
		result(null, "Hello from server");
	},

	send_file: function(data, uuid) {
		console.log("Client message recieved");

		var connection = thrift.createConnection("localhost", cmdcenterport);
		var client = thrift.createClient(CCDaemon, connection);

		client.handleRequest(
			data,
			function(err, response) {
			  console.log('ANSWER = ' + response);
			  if(!response) {
			  	console.log("Response is empty");
			  	response = "I'm not sure";
			  }
			  responses[uuid] = response;
			  connection.end();
		});
	},

	get_response: function(uuid, result) {
		result(null, responses[uuid]);
		responses[uuid] = null;
	},
}

var ftService = {
	transport: thrift.TBufferedTransport,
	protocol: thrift.TJSONProtocol,
	processor: FileTransferSvc,
	handler: ftsHandler
};

var ServerOptions = {
	files: ".",
	services: {
		"/fts": ftService,
	}
}

var server = thrift.createWebServer(ServerOptions);
var port = process.argv[2]; //8585
var cmdcenterport = process.argv[3]; //8081
server.listen(port);
console.log("Http/Thrift Server running on port: " + port);
