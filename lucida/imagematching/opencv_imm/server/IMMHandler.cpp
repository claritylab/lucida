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

Mat to_compare;

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
	cout << "Learn" << endl;
	// Save LUCID and knowledge.
	string LUCID_save = *LUCID;
	::cpp2::QuerySpec knowledge_save = *knowledge;
	folly::MoveWrapper<folly::Promise<folly::Unit>> promise;
	auto future = promise->getFuture();

	folly::RequestEventBase::get()->runInEventBaseThread(
			[=]() mutable {
		// Go through all images and store them into MongoDB.
		for (const QueryInput &query_input : knowledge_save.content) {
			for (int i = 0; i < (int) query_input.data.size(); ++i) {
				try {
					this->addImage(LUCID_save, query_input.tags[i],
							query_input.data[i]);
				} catch (exception &e) {
					cout << e.what() << endl;
				}
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
	// Save LUCID and query.
	string LUCID_save = *LUCID;
	::cpp2::QuerySpec query_save = *query;
	folly::MoveWrapper<folly::Promise<unique_ptr<string>>> promise;
	auto future = promise->getFuture();

	folly::RequestEventBase::get()->runInEventBaseThread(
			[=]() mutable {
		vector<unique_ptr<StoredImage>> images = getImages(LUCID_save);
		int best_index = Image::match(images,
				unique_ptr<QueryImage>(new QueryImage(
						move(Image::imageToMatObj(
								query_save.content[0].data[0])))));
		promise->setValue(unique_ptr<string>(
				new string(images[best_index]->getLabel())));
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
	cout << "@@@ Label: " << label << endl;
	cout << "@@@ Size: " << data.size() << endl;
	// Insert the image into MongoDB.
	unique_ptr<DBClientBase> conn = move(getConnection());
	BSONObj p = BSONObjBuilder().append("label", label).append("data", data)
			.append("size", (int) data.size()).obj();
	conn->insert("lucida.images_" + LUCID, p); // insert the image data
	// If the machine crashes here, good luck with the accuracy!
	string mat_str = Image::imageToMatString(data);
	p = BSONObjBuilder().append("label", label)
			.append("desc", mat_str)
			.append("size", (int) mat_str.size()).obj();
	conn->insert("lucida.opencv_" + LUCID, p); // insert the image desc
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
			"lucida.opencv_" + LUCID, BSONObj()); // retrieve desc, NOT data
	while (cursor->more()) {
		BSONObj p = cursor->next();
		rtn.push_back(unique_ptr<StoredImage>(new StoredImage(
				p.getStringField("label"),
				move(Image::matStringToMatObj(
						string(p.getStringField("desc"),
								p.getIntField("size")))))));
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
