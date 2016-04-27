/*
 * Implementation for the Image Matching service daemon.
 */

// Import the thrift headers.
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

// Import common utility headers.
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <fstream>
#include <sys/time.h>
#include <cstdlib>
#include <stdexcept>
#include <stdio.h>
#include <thread>

// Import the service headers.
#include "gen-cpp/LucidaService.h"
#include "../opencv/detect.h"
//#include "CommandCenter.h"
//#include "commandcenter_types.h"

// Define the namespaces.
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
// using namespace cmdcenterstubs;

// define the constant
#define THREAD_WORKS 16

class LucidaServiceHandler : public LucidaServiceIf {
private:
	/*
	 * Extracts the image data from query.
	 * Throws runtime_error if query content is empty,
	 * or query content's type is not "image".
	 */
	void extractImageFromQuery(const ::QuerySpec& query, string &image) {
		if (query.content.empty()) {
			throw runtime_error("Empty query is not allowed.");
		}
		if (query.content.front().type != "image") {
			throw runtime_error(query.content[0].type + " type is not allowed. "
					+ "Type must be \"image\".");
		}
		image = query.content.front().data.front(); // the rest is ignored
	}

public:
	/*
	 * Constructor.
	 * Trains the model.
	 */
	LucidaServiceHandler(){
		this->matcher = new FlannBasedMatcher();
		cout << "Building the image matching model..." << endl;
		build_model(this->matcher, &(this->trainImgs));
	}

	/*
	 * Creates a new service instance.
	 * TODO
	 */
	void create(const string& LUCID, const ::QuerySpec& spec) {
		cout << "Create" << endl;
	}

	/*
	 * Learns new knowledge.
	 * TODO
	 */
	void learn(const string& LUCID, const ::QuerySpec& knowledge) {
		cout << "Learn" << endl;
	}

	/*
	 * Infers from query by matching the image using opencv2.
	 */
	void infer(string& _return, const string& LUCID, const ::QuerySpec& query) {
		// Extract image data from query.
		string image;
		try {
			extractImageFromQuery(query, image);
		} catch (exception &e) {
			cout << e.what() << "\n";
			_return = "Error! " + string(e.what());
			return;
		}
		// Write the image to the current working directory.
		gettimeofday(&tp, NULL);
		long int timestamp = tp.tv_sec * 1000 + tp.tv_usec / 1000;
		string image_path = "input-" + to_string(timestamp) + ".jpg";
		ofstream imagefile(image_path.c_str(), ios::binary);
		imagefile.write(image.c_str(), image.size());
		imagefile.close();
		// Let opencv match the image.
		_return = exec_match(image_path, this->matcher, &(this->trainImgs));
		// Delete the image from disk.
		if (remove(image_path.c_str()) == -1) {
			cout << image_path << " can't be deleted from disk." << endl;
		}
	}

private:
	struct timeval tp;
	DescriptorMatcher *matcher;
	vector<string> trainImgs;
};

int main(int argc, char **argv){
	// Register with the command center. TODO
	int port = 8081;
	if (argc >= 2) {
		port = atoi(argv[1]);
	}
	// int ccport = atoi(argv[2]);

	// Initialize the transport factory.
	boost::shared_ptr<TTransportFactory> transportFactory(
			new TBufferedTransportFactory());
	boost::shared_ptr<TServerTransport> serverTransport(
			new TServerSocket(port));
	// Initialize the protocal factory.
	boost::shared_ptr<TProtocolFactory> protocolFactory(
			new TBinaryProtocolFactory());
	// Initialize the request handler.
	boost::shared_ptr<LucidaServiceHandler> handler(
			new LucidaServiceHandler());
	// Initialize the processor.
	boost::shared_ptr<TProcessor> processor(
			new LucidaServiceProcessor(handler));
	// Initialize the thread manager and factory.
	boost::shared_ptr<ThreadManager> threadManager =
			ThreadManager::newSimpleThreadManager(THREAD_WORKS);
	boost::shared_ptr<PosixThreadFactory> threadFactory =
			boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
	threadManager->threadFactory(threadFactory);
	threadManager->start();

	// Initialize the image matching server.
	TThreadPoolServer server(
			processor, serverTransport, transportFactory,
			protocolFactory, threadManager);

	thread serverThread(&TThreadPoolServer::serve, &server);

	// Register with command center. TODO
	//  boost::shared_ptr<TTransport> cmdsocket(new TSocket("localhost", ccport));
	//  boost::shared_ptr<TTransport> cmdtransport(new TBufferedTransport(cmdsocket));
	//  boost::shared_ptr<TProtocol> cmdprotocol(new TBinaryProtocol(cmdtransport));
	//  CommandCenterClient cmdclient(cmdprotocol);
	//  cmdtransport->open();
	//  cout << "Registering image matching server with command center..." << endl;
	//  MachineData mDataObj;
	//  mDataObj.name = "localhost";
	//  mDataObj.port = port;
	//  cmdclient.registerService("IMM", mDataObj);
	//  cmdtransport->close();

	cout << "Start listening to requests." << endl;
	serverThread.join();

	return 0;
}
