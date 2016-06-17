#include <iostream>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

DEFINE_int32(port,
		8082,
		"Port for IMM service (default: 8082)");

DEFINE_int32(num_of_threads,
		4,
		"Number of threads (default: 4)");

#include "IMMHandler.h"

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;

using std::cout;
using std::endl;
using std::shared_ptr;
using std::unique_ptr;
using std::to_string;


int main(int argc, char* argv[]) {
	google::InitGoogleLogging(argv[0]);
	google::ParseCommandLineFlags(&argc, &argv, true);

	auto handler = std::make_shared<IMMHandler>();
	auto server = folly::make_unique<ThriftServer>();

	server->setPort(FLAGS_port);
	server->setNWorkerThreads(FLAGS_num_of_threads);
	server->setInterface(std::move(handler));
	server->setIdleTimeout(std::chrono::milliseconds(0));
	server->setTaskExpireTime(std::chrono::milliseconds(0));

	cout << "IMM at 8082" << endl;

	server->serve();

	return 0;
}
