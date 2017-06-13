#include <cstdlib>
#include <iostream>
#include "mongo/client/dbclient.h"

#include <unistd.h>
#include <gflags/gflags.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

#include <folly/futures/Future.h>
#include "gen-cpp2/LucidaService.h"
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <folly/init/Init.h>
#include "mongo/client/dbclient.h"

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;
using namespace std;

using mongo::DBClientConnection;
using mongo::DBClientBase;
using mongo::BSONObj;
using mongo::BSONObjBuilder;

DEFINE_string(hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

int main(int argc, char* argv[]) {
	// Initialize MongoDB C++ driver.
	mongo::client::initialize();
	mongo::DBClientConnection conn;
	string mongo_addr;
	if (const char* env_p = getenv("MONGO_PORT_27017_TCP_ADDR")) {
		cout << "MongoDB: " << env_p << endl;
		mongo_addr = env_p;
	} else {
		cout << "MongoDB: localhost" << endl;
		mongo_addr = "localhost";
	}
	conn.connect(mongo_addr);
	cout << "Connection is ok" << endl;
	// TODO: change your service name
	auto_ptr<mongo::DBClientCursor> cursor = conn.query(
			"lucida.service_info", MONGO_QUERY("name" << "template"));
	BSONObj q;
	int port = 0;
	while (cursor->more()) {
		q = cursor->next();
		string port_str = q.getField("port").String();
		port = atoi(port_str.c_str());
	}

	folly::init(&argc, &argv);
	EventBase event_base;
	std::shared_ptr<apache::thrift::async::TAsyncSocket> socket_t(
			TAsyncSocket::newSocket(&event_base, FLAGS_hostname, port));
	LucidaServiceAsyncClient client(
			std::unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor>(
					new HeaderClientChannel(socket_t)));
  
	// Infer.
	QuerySpec query_spec;
	// Create a QueryInput for the query image and add it to the QuerySpec.
	// TODO: Adding your own sample query
	QueryInput query_input;
	query_input.type = "query";
	query_input.data.push_back("What is the sample query?");
	query_input.tags.push_back("localhost");
	query_input.tags.push_back(to_string(port));
	query_input.tags.push_back("0");
	query_spec.content.push_back(query_input);
	cout << "///// Connecting to template... /////"  << endl;
	auto result = client.future_infer("Clinc", std::move(query_spec)).then(
			[=](folly::Try<std::string>&& t) mutable {
		cout << "///// Result: /////\n" << t.value() << endl;
		return t.value();
	});
	event_base.loop();
	return 0;
}
