// import the thrift headers
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>
#include <thrift/transport/TSocket.h>

// import common utility headers
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <fstream>
#include <sys/time.h>
#include <cstdlib> // 06-12-15 for taking cmd line args

//boost threading libraries
#include <boost/thread.hpp>
#include <boost/bind.hpp>

// import the service headers
#include "gen-cpp/ImageMatchingService.h"
#include "../common/detect.h"

// define the namespace
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

// define the constant
#define THREAD_WORKS 16

class ImageMatchingServiceHandler : public ImageMatchingServiceIf {
public:
	// put the model training here so that it only needs to
	// be trained once
	ImageMatchingServiceHandler(){
		this->matcher = new FlannBasedMatcher();
		cout << "building the image matching model..." << endl;
		build_model(this->matcher, &(this->trainImgs));
	}
	
	void match_img(string &response, const string &image){
		gettimeofday(&tp, NULL);
		long int timestamp = tp.tv_sec * 1000 + tp.tv_usec / 1000;
		string image_path = "input-" + to_string(timestamp) + ".jpg";
		ofstream imagefile(image_path.c_str(), ios::binary);
		imagefile.write(image.c_str(), image.size());
		imagefile.close();
		response = exec_match(image_path, this->matcher, &(this->trainImgs));
	}

	void ping() {
		cout << "pinged" << endl;
	}
private:
	struct timeval tp;
	DescriptorMatcher *matcher;
	vector<string> trainImgs;
};

int main(int argc, char **argv){
	int port = 9082;
	if (argv[1]) {
		port = atoi(argv[1]);
	} else {
		cout << "Using default port for imm..." << endl;
	}

	// initialize the transport factory
	boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
	boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
	// initialize the protocal factory
	boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
	// initialize the request handler
	boost::shared_ptr<ImageMatchingServiceHandler> handler(new ImageMatchingServiceHandler());
	// initialize the processor
	boost::shared_ptr<TProcessor> processor(new ImageMatchingServiceProcessor(handler));
	// initialize the thread manager and factory
	boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(THREAD_WORKS);
	boost::shared_ptr<PosixThreadFactory> threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
	threadManager->threadFactory(threadFactory);
	threadManager->start();

	// initialize the image matching server
	TThreadPoolServer server(processor, serverTransport, transportFactory, protocolFactory, threadManager);

	cout << "Starting the image matching server on port " << port << endl;
	server.serve();

	return 0;
}
