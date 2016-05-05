#include <unistd.h>
#include <gflags/gflags.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "gen-cpp2/LucidaService.h"
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;

using std::cout;
using std::endl;
using std::vector;
using std::mutex;
using std::thread;

mutex m;

DEFINE_int32(port,
		8082,
		"Port for IMM service (default: 8082)");

DEFINE_string(hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

void client(int i) {
	EventBase event_base;

	std::shared_ptr<TAsyncSocket> socket(
			TAsyncSocket::newSocket(&event_base, FLAGS_hostname, FLAGS_port));

	LucidaServiceAsyncClient client(
			std::unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor>(
					new HeaderClientChannel(socket)));

	QuerySpec q;

	m.lock();
	cout << i << " Going to send request to IMM at 8082" << endl;
	m.unlock();
	std::string result = client.future_infer("Johann", q).getVia(&event_base);
	m.lock();
	cout << i << " Result: " << result << endl;
	m.unlock();
}

int main(int argc, char* argv[]) {
	google::InitGoogleLogging(argv[0]);
	google::ParseCommandLineFlags(&argc, &argv, true);

	vector<thread> threads;
	for (int i = 0; i < 10; ++i) {
		threads.emplace_back(client, i);
	}
	for (thread &t : threads) {
		t.join();
	}

	return 0;
}
