#include <unistd.h>
#include <gflags/gflags.h>
#include <iostream>
#include <vector>

#include <folly/futures/Future.h>
#include "gen-cpp2/LucidaService.h"
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;

using std::cout;
using std::endl;

DEFINE_int32(port,
		8082,
		"Port for IMM service (default: 8082)");

DEFINE_string(hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

int main(int argc, char* argv[]) {
	google::InitGoogleLogging(argv[0]);
	google::ParseCommandLineFlags(&argc, &argv, true);

	EventBase event_base;

	std::shared_ptr<TAsyncSocket> socket(
			TAsyncSocket::newSocket(&event_base, FLAGS_hostname, FLAGS_port));

	LucidaServiceAsyncClient client(
			std::unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor>(
					new HeaderClientChannel(socket)));

	for (int i = 0; i < 5; ++i) {
		QuerySpec q;
		cout << i << " Going to send request to IMM at 8082" << endl;
		auto result = client.future_infer("Johann", std::move(q)).then(
				[](folly::Try<std::string>&& t) mutable {
			cout << "Result: " << t.value() << endl;
			return t.value();
		});
	}

	cout << "Going to loop" << endl;
	event_base.loop();

	return 0;
}
