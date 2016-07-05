#include "IMMHandler.h"

#include <cstdlib>
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

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cv;
using namespace mongo;

using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::getenv;

// cout lock.
std::mutex cout_lock_cpp;

namespace cpp2 {
IMMHandler::IMMHandler() {
		// Initialize MongoDB C++ driver.
		client::initialize();
		string mongo_addr;
		try {
			if (const char* env_p = getenv("MONGO_PORT_27017_TCP_ADDR")) {
				print("MongoDB: " << env_p);
				mongo_addr = env_p;
			} else {
				print("MongoDB: localhost");
				mongo_addr = "localhost";
			}
			conn.connect(mongo_addr);
			print("Connection is ok");
		} catch (const DBException &e) {
			print("Caught " << e.what());
		}
}


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
			// Go through all images and store their descriptors matices
			// into MongoDB GridFS.
			for (const QueryInput &query_input : knowledge_save.content) {
				if (query_input.type == "image") {
					for (int i = 0; i < (int) query_input.data.size(); ++i) {
						this->addImage(LUCID_save, query_input.tags[i],
								query_input.data[i]);
					}
				} else if (query_input.type == "unlearn") {
					for (int i = 0; i < (int) query_input.data.size(); ++i) {
						this->deleteImage(LUCID_save, query_input.tags[i]);
					}
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
									query_save.content[0].data[0])))));
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
				EventBase event_base;
				string QA_addr = "127.0.0.1"; // cannot be "localhost"
				int QA_port = stoi(words[1], nullptr);
				if (const char* env_p = getenv("QA_PORT_8083_TCP_ADDR")) {
					QA_addr = env_p;
				}
				print("Sending request to QA at " << QA_addr << " " << QA_port);
				std::shared_ptr<TAsyncSocket> socket(
						TAsyncSocket::newSocket(&event_base, QA_addr, QA_port));
				unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor>
				channel(new HeaderClientChannel(socket));
				channel->setClientType(THRIFT_FRAMED_DEPRECATED);
				LucidaServiceAsyncClient client(std::move(channel));
				QuerySpec qa_query_spec;
				// Pop the front QueryInput from the current content
				// to get the new QuerySpec content for QA.
				qa_query_spec.content.assign(query_save.content.begin() + 1,
						query_save.content.end());
				// Append the result of IMM to the end of the text query.
				string IMM_result = images[best_index]->getLabel();
				qa_query_spec.content[0].data[0].append(" " + IMM_result);
				print("Query to QA " << qa_query_spec.content[0].data[0]);
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
	string mat_str = Image::imageToMatString(data); // written to file system
	GridFS grid(conn, "lucida");
	BSONObj result = grid.storeFile(mat_str.c_str(), mat_str.size(),
			"opencv_" + LUCID + "/" + label);
}

void IMMHandler::deleteImage(const string &LUCID, const string &label) {
	print("~~~ Label: " << label);
	GridFS grid(conn, "lucida");
	grid.removeFile("opencv_" + LUCID + "/" + label); // match addImage()
}

vector<unique_ptr<StoredImage>> IMMHandler::getImages(const string &LUCID) {
	vector<unique_ptr<StoredImage>> rtn;
	// Retrieve all images of the user from MongoDB.
	auto_ptr<DBClientCursor> cursor = conn.query(
			"lucida.images_" + LUCID, BSONObj()); // retrieve desc, NOT data
	GridFS grid(conn, "lucida");
	while (cursor->more()) {
		string label = cursor->next().getStringField("label");
		ostringstream out;
		GridFile gf = grid.findFileByName("opencv_" + LUCID + "/" + label);
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
}
