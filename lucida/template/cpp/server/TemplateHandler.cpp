#include "TemplateHandler.h"

#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;

using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::getenv;

// cout lock.
std::mutex cout_lock_cpp;

namespace cpp2 {
TemplateHandler::TemplateHandler() {
	// TODO: adding your own initializatin of the handler
}

// TODO: implement your own future_create function.
folly::Future<folly::Unit> TemplateHandler::future_create
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> spec) {
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();
	promise->setValue(Unit{});
	return future;
}

// TODO: implement your own future_learn function.
folly::Future<folly::Unit> TemplateHandler::future_learn
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> knowledge) {
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();
	promise->setValue(Unit{});
	return future;
}

// TODO: implement your own future_infer function.
folly::Future<unique_ptr<string>> TemplateHandler::future_infer
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> query) {
	print("@@@@@ Infer; User: " + *(LUCID.get()));
	folly::MoveWrapper<folly::Promise<unique_ptr<string> > > promise;
	auto future = promise->getFuture();
	promise->setValue(unique_ptr<string>(new string("This is the sample answer")));
	return future;
}

// TODO: define the methods needed for your service.
}
