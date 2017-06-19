#include <iostream>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

DEFINE_int32(num_of_threads,
		4,
		"Number of threads (default: 4)");

#include "IMMHandler.h"
#include <string>
#include <fstream>
#include <folly/init/Init.h>
#include "mongo/client/dbclient.h"

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;
using namespace std;
using namespace mongo;

using std::cout;
using std::endl;
using std::shared_ptr;
using std::unique_ptr;
using std::to_string;


int main(int argc, char* argv[]) {
	folly::init(&argc, &argv);

	// Initialize MongoDB C++ driver.
	client::initialize();
	DBClientConnection conn;
	string mongo_addr;
	if (const char* env_p = getenv("MONGO_PORT_27017_TCP_ADDR")) {
		print("MongoDB: " << env_p);
		mongo_addr = env_p;
	} else {
		print("MongoDB: localhost");
		mongo_addr = "localhost";
	}
	conn.connect(mongo_addr);
	print("Connection is ok");
	auto_ptr<DBClientCursor> cursor = conn.query(
			"lucida.service_info", MONGO_QUERY("name" << "imagematching"));
	BSONObj p;
	int port = 0;
	while (cursor->more()) {
		p = cursor->next();
		string port_str = p.getField("port").String();
		port = atoi(port_str.c_str());
	}


	auto handler = std::make_shared<IMMHandler>();
	auto server = folly::make_unique<ThriftServer>();

	server->setPort(port);
	server->setNWorkerThreads(FLAGS_num_of_threads);
	server->setInterface(std::move(handler));
	server->setIdleTimeout(std::chrono::milliseconds(0));
	server->setTaskExpireTime(std::chrono::milliseconds(0));

	cout << "IMM at " << port << endl;

	server->serve();

	return 0;
}
