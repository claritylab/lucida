#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <gflags/gflags.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string> 

#include <folly/futures/Future.h>
#include "gen-cpp2/LucidaService.h"
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <folly/init/Init.h>
#include "mongo/client/dbclient.h"

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;
using namespace std;

namespace fs = boost::filesystem;

DEFINE_string(hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

string getImageData(const string &image_path) {
	ifstream fin(image_path.c_str(), ios::binary);
	ostringstream ostrm;
	ostrm << fin.rdbuf();
	string image(ostrm.str());
	if (!fin) {
		cerr << "Could not open the file " << image_path << endl;
		exit(1);
	}
	return image;
}

int main(int argc, char* argv[]){
	folly::init(&argc, &argv);
	EventBase event_base;

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
	auto_ptr<mongo::DBClientCursor> cursor = conn.query(
		"lucida.service_info", MONGO_QUERY("name" << "facerecognition"));
	mongo::BSONObj p;
	int port = 0;
	while (cursor->more()) {
		p = cursor->next();
		string port_str = p.getField("port").String();
		port = atoi(port_str.c_str());
	}

	std::shared_ptr<apache::thrift::async::TAsyncSocket> socket_t(
			TAsyncSocket::newSocket(&event_base, FLAGS_hostname, port));
	LucidaServiceAsyncClient client(
			std::unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor>(
					new HeaderClientChannel(socket_t)));

	int num_tests = 3;
	for (int i=0; i < num_tests; ++i){
		string image = getImageData("test" + to_string(i) + ".jpg");
		// Create a QuerySpec.
		QuerySpec query_spec;
		// Create a QueryInput for the query image and add it to the QuerySpec.
		QueryInput query_input;
		query_input.type = "image";
		query_input.data.push_back(image);
		query_input.tags.push_back("localhost");
		query_input.tags.push_back(to_string(port));
		query_input.tags.push_back("0");
		query_spec.content.push_back(query_input);
		cout << i << " Sending request to FACE at " << port << endl;
		auto result = client.future_infer("Johann", std::move(query_spec)).then(
				[=](folly::Try<std::string>&& t) mutable {
			cout << i << " result: " << t.value() << endl;
			return t.value();
		});
	}
	cout << "Going to loop" << endl;
	event_base.loop();
	return 0;
}