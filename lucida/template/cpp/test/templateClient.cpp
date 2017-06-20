#include <cstdlib>
#include <iostream>

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

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;
using namespace std;

DEFINE_string(hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

int main(int argc, char* argv[]) {
	if (argc != 2){
		cerr << "Wrong argument!" << endl;
		exit(EXIT_FAILURE);
	}
	int port = atoi(argv[1]);

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
