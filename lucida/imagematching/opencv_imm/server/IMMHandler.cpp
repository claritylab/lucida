#include "IMMHandler.h"

#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <folly/futures/Future.h>
#include <folly/MoveWrapper.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

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
	QuerySpec query_save = *query;
	folly::MoveWrapper<folly::Promise<unique_ptr<string>>> promise;
	auto future = promise->getFuture();
	// Async.
	folly::RequestEventBase::get()->runInEventBaseThread(
			[=]() mutable {
		try {
			if (countImages(LUCID_save) == 0) {
				promise->setValue(
						unique_ptr<string>(new string(
								"Cannot match in empty collection")));
				return;
			}
			if (query_save.content.empty()
					|| query_save.content[0].data.empty()) {
				throw runtime_error("IMM received empty infer query");
			}
			vector<unique_ptr<StoredImage>> images = getImages(LUCID_save);
			int best_index = Image::match(images,
					unique_ptr<QueryImage>(new QueryImage(
							move(Image::imageToMatObj(
									query_save.content[0].data[0])))));
			string IMM_result = getImageLabelFromId(
				LUCID_save, images[best_index]->getImageId());
			print("Result: " << IMM_result);
				promise->setValue(unique_ptr<string>(
						new string(IMM_result)));
				return;

			

		} catch (Exception &e) {
			print(e.what()); // program aborted although exception is caught
			promise->setValue(unique_ptr<string>(new string(e.what())));
			return;
		}
	}
	);
	return future;
}

int IMMHandler::countImages(const string &LUCID) {
	auto_ptr<DBClientCursor> cursor = conn.query(
			"lucida.images_" + LUCID, BSONObj());
	int rtn = 0;
	while (cursor->more()) {
		++rtn;
		cursor->next();
	}
	return rtn;
}

void IMMHandler::addImage(const string &LUCID,
		const string &image_id, const string &data) {
	print("@@@ image_id: " << image_id);
	print("@@@ Size: " << data.size());
	// Insert the descriptors matrix into MongoDB.
	string mat_str = Image::imageToMatString(data); // written to file system
	GridFS grid(conn, "lucida");
	BSONObj result = grid.storeFile(mat_str.c_str(), mat_str.size(),
			"opencv_" + LUCID + "/" + image_id);
}

void IMMHandler::deleteImage(const string &LUCID, const string &image_id) {
	print("~~~ image_id: " << image_id);
	GridFS grid(conn, "lucida");
	grid.removeFile("opencv_" + LUCID + "/" + image_id); // match addImage()
}

vector<unique_ptr<StoredImage>> IMMHandler::getImages(const string &LUCID) {
	vector<unique_ptr<StoredImage>> rtn;
	// Retrieve all images of the user from MongoDB.
	auto_ptr<DBClientCursor> cursor = conn.query(
			"lucida.images_" + LUCID, BSONObj());
	GridFS grid(conn, "lucida");
	while (cursor->more()) {
		string image_id = cursor->next().getStringField("image_id");
		ostringstream out;
		GridFile gf = grid.findFileByName("opencv_" + LUCID + "/" + image_id);
		if (!gf.exists()) {
			print("opencv_" + LUCID + "/" + image_id + " not found!");
			continue;
		}
		gf.write(out);
		rtn.push_back(unique_ptr<StoredImage>(new StoredImage(
				image_id,
				move(Image::matStringToMatObj(out.str())))));
	}
	return rtn;
}

string IMMHandler::getImageLabelFromId(const string &LUCID, const string &image_id) {
	auto_ptr<DBClientCursor> cursor = conn.query(
			"lucida.images_" + LUCID, MONGO_QUERY("image_id" << image_id));
	while (cursor->more()) {
		string image_label = cursor->next().getStringField("label");
		print(image_label);
		return image_label;
	}
	return "";
}
}
