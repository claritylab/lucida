#include "FakeIMMHandler.h"

#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <sstream>
#include <unistd.h>
#include <iostream>

DEFINE_int32(QA_port,
		8083,
		"Port for QA service (default: 8083)");

DEFINE_string(QA_hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;

using std::cout;
using std::endl;

namespace cpp2 {
FakeImmHandler::FakeImmHandler() {}

folly::Future<folly::Unit> FakeImmHandler::future_create
(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> spec) {
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();

	folly::RequestEventBase::get()->runInEventBaseThread(
			[promise, this]() mutable {
		// do stuff
	}
	);

	return future;
}

folly::Future<folly::Unit> FakeImmHandler::future_learn
(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> knowledge) {
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();

	folly::RequestEventBase::get()->runInEventBaseThread(
			[promise, this]() mutable {
		// do stuff
	}
	);

	return future;
}


folly::Future<std::unique_ptr<std::string>> FakeImmHandler::future_infer
(std::unique_ptr<std::string> LUCID, std::unique_ptr< ::cpp2::QuerySpec> query) {
	std::cout << "Infer" << std::endl;
	folly::MoveWrapper<folly::Promise<std::unique_ptr<std::string> > > promise;
	auto future = promise->getFuture();

	folly::RequestEventBase::get()->runInEventBaseThread(
			[promise, this]() mutable {
		std::string QA_result = askQA();
		std::unique_ptr<std::string> result = folly::make_unique<std::string>(QA_result);
		promise->setValue(std::move(result));
	}
	);

	return future;
}

std::string FakeImmHandler::askQA() {
	EventBase event_base;

	std::shared_ptr<TAsyncSocket> socket(
			TAsyncSocket::newSocket(&event_base, FLAGS_QA_hostname, FLAGS_QA_port));

	std::unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor> channel(
						new HeaderClientChannel(socket));

	channel->setClientType(THRIFT_FRAMED_DEPRECATED);

	LucidaServiceAsyncClient client(std::move(channel));

	QuerySpec q;

	cout << "Sending request to QA at 8083" << endl;
	std::string result = client.future_infer("Johann", q).getVia(&event_base);
	cout << "Result: " << result << endl;
	return result;
}
}
