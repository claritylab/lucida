#include "IMMHandler.h"

#include <sstream>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

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
using namespace mongo;

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

	folly::RequestEventBase::get()->runInEventBaseThread(
			[=]() mutable {
		try {
			vector<unique_ptr<StoredImage>> images = getImages(LUCID_save);
			if (images.empty()) {
				print("Error! No images for " + LUCID_save);
				promise->setValue(unique_ptr<string>(new string()));
			}
			int best_index = Image::match(images,
					unique_ptr<QueryImage>(new QueryImage(
							move(Image::imageToMatObj(
									query_save.content[0].data[0])))));
			print("Result: " << images[best_index]->getLabel());
			promise->setValue(unique_ptr<string>(
					new string(images[best_index]->getLabel())));
		} catch (Exception &e) {
			print(e.what());
			promise->setValue(unique_ptr<string>(new string()));
		}
		//		EventBase event_base;
		//
		//		std::shared_ptr<TAsyncSocket> socket(
		//				TAsyncSocket::newSocket(&event_base, FLAGS_QA_hostname, FLAGS_QA_port));
		//
		//		unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor> channel(
		//				new HeaderClientChannel(socket));
		//
		//		channel->setClientType(THRIFT_FRAMED_DEPRECATED);
		//
		//		LucidaServiceAsyncClient client(std::move(channel));
		//
		//		QuerySpec q;
		//
		//		cout << "Sending request to QA at 8083" << endl;
		//		client.future_infer("Johann", q).then(
		//				[&](folly::Try<string>&& t) mutable {
		//			cout << "Result: " << t.value();
		//			unique_ptr<string> result = folly::make_unique<std::string>(t.value());
		//			promise->setValue(std::move(result));
		//		});
		//
		//		event_base.loop();
	}
	);

	return future;
}

void IMMHandler::addImage(const string &LUCID,
		const string &label, const string &data) {
	print("@@@ Label: " << label);
	print("@@@ Size: " << data.size());
	// Insert the descriptors matrix into MongoDB.
	unique_ptr<DBClientBase> conn = move(getConnection());
	string mat_str = Image::imageToMatString(data); // written to file system
	GridFS grid(*conn, "lucida");
	BSONObj result = grid.storeFile(mat_str.c_str(), mat_str.size(),
			"opencv_" + LUCID + "/" + label);
	string e = conn->getLastError();
	if (!e.empty()) {
		throw runtime_error("Insert failed " + e);
	}
}

vector<unique_ptr<StoredImage>> IMMHandler::getImages(const string &LUCID) {
	vector<unique_ptr<StoredImage>> rtn;
	// Retrieve all images of the user from MongoDB.
	unique_ptr<DBClientBase> conn = move(getConnection());
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

unique_ptr<DBClientBase> IMMHandler::getConnection() {
	string uri = "localhost:27017"; // specify where MongoDB is running
	string errmsg;
	ConnectionString cs = ConnectionString::parse(uri, errmsg);
	if (!cs.isValid()) {
		throw runtime_error("Error parsing connection string "
				+ uri + ": " + errmsg);
	}
	unique_ptr<DBClientBase> conn(cs.connect(errmsg));
	if (!conn) {
		throw runtime_error("Couldn't connect: " + errmsg);
	}
	return conn;
}
}
