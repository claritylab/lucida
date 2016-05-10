//#include <memory>
//#include <gflags/gflags.h>
//#include <glog/logging.h>
#include <iostream>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

DEFINE_int32(port,
		8082,
		"Port for ASR service (default: 8082)");

DEFINE_int32(num_of_threads,
		4,
		"Number of threads (default: 4)");

DEFINE_int32(CMD_port,
		8080,
		"Port for QA service (default: 8080)");

DEFINE_string(CMD_hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

#include "FakeIMMHandler.h"

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;

using std::cout;
using std::endl;
using std::shared_ptr;
using std::unique_ptr;
using std::to_string;

void registerToCMD() {
	EventBase event_base;
	shared_ptr<TAsyncSocket> socket(
			TAsyncSocket::newSocket(&event_base, FLAGS_CMD_hostname, FLAGS_CMD_port));
	unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor> channel(
			new HeaderClientChannel(socket));
	channel->setClientType(THRIFT_FRAMED_DEPRECATED);
	LucidaServiceAsyncClient client(std::move(channel));
	QuerySpec q;
	q.set_name(to_string(8082));
	auto result = client.future_create("IMM", q).then(
			[](folly::Try<folly::Unit>&& t) mutable {
		cout << "Successfully registered to command center" << endl;
	});

	event_base.loop();
}

int main(int argc, char* argv[]) {
	registerToCMD();

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
