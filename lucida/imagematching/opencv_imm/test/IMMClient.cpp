#include <unistd.h>
#include <gflags/gflags.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <folly/futures/Future.h>
#include "gen-cpp2/LucidaService.h"
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace cpp2;
using namespace std;

namespace fs = boost::filesystem;

DEFINE_int32(port,
		8087,
		"Port for IMM service (default: 8087)");

DEFINE_string(hostname,
		"127.0.0.1",
		"Hostname of the server (default: localhost)");

string getImageData(string image_path) {
	string img_name(image_path);
	ifstream fin(img_name.c_str(), ios::binary);
	ostringstream ostrm;
	ostrm << fin.rdbuf();
	string image(ostrm.str());
	if (!fin) {
		cerr << "Could not open the file!" << endl;
		exit(1);
	}
	return image;
}

int main(int argc, char* argv[]) {
	google::InitGoogleLogging(argv[0]);
	google::ParseCommandLineFlags(&argc, &argv, true);
	EventBase event_base;
	std::shared_ptr<TAsyncSocket> socket(
			TAsyncSocket::newSocket(&event_base, FLAGS_hostname, FLAGS_port));
	LucidaServiceAsyncClient client(
			std::unique_ptr<HeaderClientChannel, DelayedDestruction::Destructor>(
					new HeaderClientChannel(socket)));
	// Open the images.
	string db = fs::current_path().string() + "/test_db";
	fs::path p = fs::system_complete(db);
	assert(fs::is_directory(p));
	fs::directory_iterator end_iter;
	for(fs::directory_iterator dir_itr(p); dir_itr != end_iter; ++dir_itr) {
		string image = getImageData(dir_itr->path().string());
		// Create a QuerySpec.
		QuerySpec query_spec;
		// Create a QueryInput for each image and add it to the QuerySpec.
		QueryInput query_input;
		query_input.type = "image";
		query_input.data.push_back(image);
		string label = dir_itr->path().stem().string();
		cout << "Label: " << label << endl;
		cout << "Image size: " << image.size() << endl;
		query_input.tags.push_back(label);
		query_spec.content.push_back(query_input);
		// Make request.
		client.future_learn("Johann", std::move(query_spec)).then(
				[](folly::Try<folly::Unit>&& t) mutable {
			cout << "Done" << endl;
		});
		cout << "Going to loop" << endl;
		event_base.loop();
	}

	// Infer.
	string image = getImageData("test.jpg");
	// Create a QuerySpec.
	QuerySpec query_spec;
	// Create a QueryInput for the query image and add it to the QuerySpec.
	QueryInput query_input;
	query_input.type = "image";
	query_input.data.push_back(image);
	query_spec.content.push_back(query_input);
	// Make request.
	for (int i = 0; i < 2; ++i) {
		cout << i << " Sending request to IMM at 8082" << endl;
		auto result = client.future_infer("Johann", std::move(query_spec)).then(
				[](folly::Try<std::string>&& t) mutable {
			cout << "Result: " << t.value() << endl;
			return t.value();
		});
	}
	cout << "Going to loop" << endl;
	event_base.loop();
	return 0;
}
