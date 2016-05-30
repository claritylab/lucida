#include "IMMHandler.h"

#include <sstream>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

DEFINE_int32(QA_port,
		8083,
		"Port for QA service (default: 8083)");

DEFINE_string(QA_hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cv;

using mongo::DBClientConnection;
using mongo::DBException;
using mongo::BSONObj;
using mongo::BSONObjBuilder;
using mongo::GridFS;
using mongo::GridFile;
using mongo::DBClientCursor;

using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::shared_ptr;

// cout lock.
std::mutex cout_lock_cpp;

namespace cpp2 {
IMMHandler::IMMHandler() {}

folly::Future<folly::Unit> IMMHandler::future_create
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> spec) {
	folly::MoveWrapper<folly::Promise<folly::Unit > > promise;
	auto future = promise->getFuture();
	promise->setValue(Unit{});
	return future;
}

folly::Future<folly::Unit> IMMHandler::future_learn
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> knowledge) {
	print("Learn");
	// Save LUCID and knowledge.
	string LUCID_save = *LUCID;
	::cpp2::QuerySpec knowledge_save = *knowledge;
	folly::MoveWrapper<folly::Promise<folly::Unit>> promise;
	auto future = promise->getFuture();
	// Async.
	folly::RequestEventBase::get()->runInEventBaseThread(
			[=]() mutable {
		try {
			// Go through all images and store them into MongoDB.
			for (const QueryInput &query_input : knowledge_save.content) {
				for (int i = 0; i < (int) query_input.data.size(); ++i) {
					this->addImage(LUCID_save, query_input.tags[i],
							query_input.data[i]);
				}
			}
		} catch (Exception &e) {
			print(e.what());
		}
		promise->setValue(Unit{});
	}
	);
	return future;
}

folly::Future<unique_ptr<string>> IMMHandler::future_infer
(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> query) {
	print("Infer");
	// Save LUCID and query.
	string LUCID_save = *LUCID;
	::cpp2::QuerySpec query_save = *query;
	folly::MoveWrapper<folly::Promise<unique_ptr<string>>> promise;
	auto future = promise->getFuture();
	// Async.
	folly::RequestEventBase::get()->runInEventBaseThread(
			[=]() mutable {
		try {
			if (query_save.content.empty()
					|| query_save.content[0].data.empty()) {
				throw runtime_error("IMM received empty infer query");
			}
			vector<unique_ptr<StoredImage>> images = getImages(LUCID_save);
			int best_index = Image::match(images,
					unique_ptr<QueryImage>(new QueryImage(
							move(Image::imageToMatObj(
									query_save.content[0].data[0]))))); // use opencv
			print("Result: " << images[best_index]->getLabel());
			// Check if the query needs to be further sent to QA.
			if (!query_save.content[0].tags.empty() &&
					query_save.content[0].tags[0] != "") {
				if (query_save.content.size() == 1
						|| query_save.content[1].data.empty()) {
					throw runtime_error("IMM wants to further request"
							" to QA but no text query");
				}
				vector<string> words;
				boost::split(words,query_save.content[0].tags[0],
						boost::is_any_of(", "), boost::token_compress_on);
				if (words.size() != 2) {
					throw runtime_error("tags[0] must have the following format"
							": localhost, 8083");
				}
				print("Sending request to QA at "
						<< words[0] << " " << words[1]);
				EventBase event_base;
				std::shared_ptr<TAsyncSocket> socket(
						TAsyncSocket::newSocket(&event_base, FLAGS_QA_hostname, FLAGS_QA_port));
				unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor>
				channel(new HeaderClientChannel(socket));
				channel->setClientType(THRIFT_FRAMED_DEPRECATED);
				//channel->setClientType(THRIFT_FRAMED_DEPRECATED);
				LucidaServiceAsyncClient client(std::move(channel));
				QuerySpec qa_query_spec;
				// Pop the front QueryInput from the current content
				// to get the new QuerySpec content for QA.
				qa_query_spec.content.assign(query_save.content.begin() + 1,
						query_save.content.end());
				// Append the result of IMM to the end of the text query.
				qa_query_spec.content[0].data[0].append(" "
						+ images[best_index]->getLabel());
				print("Query to QA " << qa_query_spec.content[0].data[0]);
				string IMM_result = images[best_index]->getLabel();
				client.future_infer(LUCID_save, qa_query_spec).then(
						[promise, IMM_result](folly::Try<string>&& t) mutable {
					print("QA result: " << t.value());
					unique_ptr<string> result = folly::make_unique<std::string>(
							"IMM: " + IMM_result
							+ "; QA: " + t.value());
					promise->setValue(std::move(result)); // exit
				});
				event_base.loop();
			} else {
				promise->setValue(unique_ptr<string>(
						new string(images[best_index]->getLabel()))); // exit
			}
		} catch (Exception &e) {
			print(e.what());
			promise->setValue(unique_ptr<string>(new string())); // exit
		}
	}
	);
	return future;
}

void IMMHandler::addImage(const string &LUCID,
		const string &label, const string &data) {
	print("@@@ Label: " << label);
	print("@@@ Size: " << data.size());
	// Insert the descriptors matrix into MongoDB.
	unique_ptr<DBClientConnection> conn = move(getConnection());
	string mat_str = Image::imageToMatString(data); // written to file system
	GridFS grid(*conn, "lucida");
	BSONObj result = grid.storeFile(mat_str.c_str(), mat_str.size(),
			"opencv_" + LUCID + "/" + label);
}

vector<unique_ptr<StoredImage>> IMMHandler::getImages(const string &LUCID) {
	vector<unique_ptr<StoredImage>> rtn;
	// Retrieve all images of the user from MongoDB.
	unique_ptr<DBClientConnection> conn = move(getConnection());
	auto_ptr<DBClientCursor> cursor = conn->query(
			"lucida.images_" + LUCID, BSONObj()); // retrieve desc, NOT data
	GridFS grid(*conn, "lucida");
	while (cursor->more()) {
		string label = cursor->next().getStringField("label");
		ostringstream out;
		GridFile gf = grid.findFile("opencv_" + LUCID + "/" + label);
		if (!gf.exists()) {
			print("opencv_" + LUCID + "/" + label + " not found!");
			continue;
		}
		gf.write(out);
		rtn.push_back(unique_ptr<StoredImage>(new StoredImage(
				label,
				move(Image::matStringToMatObj(out.str())))));
	}
	return rtn;
}

unique_ptr<DBClientConnection> IMMHandler::getConnection() {
	print("###");
	mongo::client::initialize();
	print("qwvt");
	unique_ptr<DBClientConnection> conn(new DBClientConnection);
	print("q3g4");
	try {
		conn->connect("localhost:27017");
	} catch( DBException &e ) {
		cout << "Caught " << endl;
	}
	return conn;
}
}
