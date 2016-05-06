#include "IMMHandler.h"

#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <sstream>
#include <unistd.h>
#include <iostream>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>


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
using std::string;
using std::unique_ptr;
using std::shared_ptr;

namespace cpp2 {
IMMHandler::IMMHandler() {
	this->matcher = new FlannBasedMatcher();
}

folly::Future<folly::Unit> IMMHandler::future_create
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> spec) {
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();

	folly::RequestEventBase::get()->runInEventBaseThread(
			[promise, this]() mutable {
		// do stuff
	}
	);

	return future;
}

folly::Future<folly::Unit> IMMHandler::future_learn
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> knowledge) {
	cout << "Learn" << endl;
	// Save LUCID and knowledge.
	string LUCID_save = *LUCID;
	::cpp2::QuerySpec knowledge_save = *knowledge;
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();
	folly::RequestEventBase::get()->runInEventBaseThread(
			[=]() mutable {
		for (const QueryInput &query_input : knowledge_save.content) {
			for (int i = 0; i < (int) query_input.data.size(); ++i) {
			    mongocxx::instance inst{};
			    mongocxx::client conn{mongocxx::uri{}};
			    bsoncxx::builder::stream::document document{};
			    auto collection = conn["lucida"]["images_" + LUCID_save];
			    document << "image_label" << query_input.tags[i]
						<< "image_data" << query_input.data[i];
			    cout << "@@@ Label: " << query_input.tags[i] << endl;
			    cout << "@@@ Size: " << query_input.data[i].size() << endl;
			    collection.insert_one(document.view());
			}
		}
		promise->setValue(Unit{});
	}
	);
	return future;
}

folly::Future<unique_ptr<string>> IMMHandler::future_infer
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> query) {
	cout << "Infer" << endl;
	folly::MoveWrapper<folly::Promise<unique_ptr<string> > > promise;
	auto future = promise->getFuture();

	folly::RequestEventBase::get()->runInEventBaseThread(
			[&]() mutable {

		string match_result = infer(std::move(LUCID), std::move(query));


		EventBase event_base;

		shared_ptr<TAsyncSocket> socket(
				TAsyncSocket::newSocket(&event_base, FLAGS_QA_hostname, FLAGS_QA_port));

		unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor> channel(
				new HeaderClientChannel(socket));

		channel->setClientType(THRIFT_FRAMED_DEPRECATED);

		LucidaServiceAsyncClient client(std::move(channel));

		QuerySpec q;

		cout << "Sending request to QA at 8083" << endl;
		client.future_infer("Johann", q).then(
				[&](folly::Try<string>&& t) mutable {
			cout << "Result: " << t.value();
			unique_ptr<string> result = folly::make_unique<std::string>(t.value());
			promise->setValue(std::move(result));
		});

		event_base.loop();
	}
	);

	return future;
}

/*
 * Infers from query by matching the image using opencv2.
 */
string IMMHandler::infer(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> query) {
	// Extract image data from query.
	string image;
	try {
		extractImageFromQuery(std::move(query), image);
	} catch (exception &e) {
		cout << e.what() << "\n";
		return "Error! " + string(e.what());
	}
	// Write the image to the current working directory.
	gettimeofday(&tp, NULL);
	long int timestamp = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	string image_path = "input-" + to_string(timestamp) + ".jpg";
	ofstream imagefile(image_path.c_str(), ios::binary);
	imagefile.write(image.c_str(), image.size());
	imagefile.close();
	// Let opencv match the image.
	string rtn = exec_match(image_path, this->matcher, &(this->trainImgs));
	// Delete the image from disk.
	if (remove(image_path.c_str()) == -1) {
		cout << image_path << " can't be deleted from disk." << endl;
	}
	return rtn;
}

/*
 * Extracts the image data from query.
 * Throws runtime_error if query content is empty,
 * or query content's type is not "image".
 */
void IMMHandler::extractImageFromQuery(std::unique_ptr< ::cpp2::QuerySpec> query, string &image) {
	if (query->content.empty()) {
		throw runtime_error("Empty query is not allowed.");
	}
	if (query->content.front().type != "image") {
		throw runtime_error(query->content[0].type + " type is not allowed. "
				+ "Type must be \"image\".");
	}
	image = query->content.front().data.front(); // the rest is ignored
}

}
