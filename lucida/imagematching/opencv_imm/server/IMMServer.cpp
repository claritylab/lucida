#include <iostream>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

DEFINE_int32(num_of_threads,
		4,
		"Number of threads (default: 4)");

#include "IMMHandler.h"
#include "Parser.h"
#include <string>
#include <fstream>
#include <folly/init/Init.h>

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;
using namespace std;

using std::cout;
using std::endl;
using std::shared_ptr;
using std::unique_ptr;
using std::to_string;


int main(int argc, char* argv[]) {
	folly::init(&argc, &argv);
	
	Properties props;
	props.Read("../../../config.properties");
	string portVal;
	int port;
	if (!props.GetValue("IMM_PORT", portVal)) {
		cout << "IMM port not defined" << endl;
		return -1;
	} else {
		port = atoi(portVal.c_str());
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
