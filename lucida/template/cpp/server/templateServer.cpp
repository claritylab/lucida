#include <iostream>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

DEFINE_int32(num_of_threads,
		4,
		"Number of threads (default: 4)");

#include "templateHandler.h"
#include <cstdlib>
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

	if (argc != 2){
		cerr << "Wrong argument!" << endl;
		exit(EXIT_FAILURE);
	}
	int port = atoi(argv[1]);

	auto handler = std::make_shared<templateHandler>();
	auto server = folly::make_unique<ThriftServer>();

	server->setPort(port);
	server->setNWorkerThreads(FLAGS_num_of_threads);
	server->setInterface(std::move(handler));
	server->setIdleTimeout(std::chrono::milliseconds(0));
	server->setTaskExpireTime(std::chrono::milliseconds(0));

	cout << "TPL at port " << to_string(port) << endl;
	server->serve();

	return 0;
}
