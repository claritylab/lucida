//#include <memory>
//#include <gflags/gflags.h>
//#include <glog/logging.h>
#include <iostream>

#include <thrift/lib/cpp2/server/ThriftServer.h>

DEFINE_int32(port,
		8082,
		"Port for ASR service (default: 8082)");

DEFINE_int32(num_of_threads,
		4,
		"Number of threads (default: 4)");

#include "FakeIMMHandler.h"

using namespace apache::thrift;
using namespace apache::thrift::async;

using namespace cpp2;

int main(int argc, char* argv[]) {
	std::cout << "IMM at 8082" << std::endl;
	google::InitGoogleLogging(argv[0]);
	google::ParseCommandLineFlags(&argc, &argv, true);

	auto handler = std::make_shared<FakeImmHandler>();
	auto server = folly::make_unique<ThriftServer>();

	server->setPort(FLAGS_port);
	server->setNWorkerThreads(FLAGS_num_of_threads);
	server->setInterface(std::move(handler));
	server->setIdleTimeout(std::chrono::milliseconds(0));
	server->setTaskExpireTime(std::chrono::milliseconds(0));

	server->serve();

	return 0;
}
