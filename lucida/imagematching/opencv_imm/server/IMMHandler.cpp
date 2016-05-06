#include "IMMHandler.h"

#include <sstream>
#include <unistd.h>
#include <iostream>
#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "client/dbclient.h"

DEFINE_int32(QA_port,
		8083,
		"Port for QA service (default: 8083)");

DEFINE_string(QA_hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cv;
using namespace std;
using namespace mongo;

using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::shared_ptr;

namespace cpp2 {
IMMHandler::IMMHandler() {}

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

			    cout << "@@@ Label: " << query_input.tags[i] << endl;
			    cout << "@@@ Size: " << query_input.data[i].size() << endl;
			    string uri = "localhost:27017";
			    string errmsg;
			    ConnectionString cs = ConnectionString::parse(uri, errmsg);
			    if (!cs.isValid()) {
			        std::cout << "Error parsing connection string " << uri << ": " << errmsg << std::endl;
			        return EXIT_FAILURE;
			    }

			    boost::scoped_ptr<DBClientBase> conn(cs.connect(errmsg));
			    if (!conn) {
			        cout << "couldn't connect : " << errmsg << endl;
			        return EXIT_FAILURE;
			    }





			    BSONObj p = BSONObjBuilder()
			    		.append("image_label", query_input.tags[i])
						.append("image_data", query_input.data[i]).obj();


			    conn->insert("lucida.images_" + LUCID_save, p);
			    cout << "lucida.images_" + LUCID_save << endl;
		        string e = conn->getLastError();
		        if (!e.empty()) {
		            cout << "insert #1 failed: " << e << endl;
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
	folly::MoveWrapper<folly::Promise<unique_ptr<string> > > promise;
	auto future = promise->getFuture();

	folly::RequestEventBase::get()->runInEventBaseThread(
			[&]() mutable {




		// Extract image data from query.
//		string image;
//		try {
//			extractImageFromQuery(std::move(query), image);
//		} catch (exception &e) {
//			cout << e.what() << "\n";
//			return "Error! " + string(e.what());
//		}







		EventBase event_base;

		std::shared_ptr<TAsyncSocket> socket(
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

//vector<unique_ptr<Image>> IMMHandler::getImages(unique_ptr<string> LUCID) {
//
//
//
//
//}
//
///*
// * Infers from query by matching the image using opencv2.
// */
//string IMMHandler::infer(unique_ptr<string> LUCID, unique_ptr< ::cpp2::QuerySpec> query) {
//	// Extract image data from query.
//	string image;
//	try {
//		extractImageFromQuery(std::move(query), image);
//	} catch (exception &e) {
//		cout << e.what() << "\n";
//		return "Error! " + string(e.what());
//	}
//	// Write the image to the current working directory.
//	gettimeofday(&tp, NULL);
//	long int timestamp = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//	string image_path = "input-" + to_string(timestamp) + ".jpg";
//	ofstream imagefile(image_path.c_str(), ios::binary);
//	imagefile.write(image.c_str(), image.size());
//	imagefile.close();
//	// Let opencv match the image.
//	string rtn = exec_match(image_path, this->matcher, &(this->trainImgs));
//	// Delete the image from disk.
//	if (remove(image_path.c_str()) == -1) {
//		cout << image_path << " can't be deleted from disk." << endl;
//	}
//	return rtn;
//}
//
///*
// * Extracts the image data from query.
// * Throws runtime_error if query content is empty,
// * or query content's type is not "image".
// */
//void IMMHandler::extractImageFromQuery(std::unique_ptr< ::cpp2::QuerySpec> query, string &image) {
//	if (query->content.empty()) {
//		throw runtime_error("Empty query is not allowed.");
//	}
//	if (query->content.front().type != "image") {
//		throw runtime_error(query->content[0].type + " type is not allowed. "
//				+ "Type must be \"image\".");
//	}
//	image = query->content.front().data.front(); // the rest is ignored
//}
//
}
