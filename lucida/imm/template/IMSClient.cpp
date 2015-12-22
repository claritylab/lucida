/*
 *
 *
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
 
#include "gen-cpp/ImageMatchingService.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

int main(int argc, char** argv) {
	int port = 9082;
	string imagePath;
	
	if (argv[1]) {
		port = atoi(argv[1]);
	} else {
		cout << "Using default port for imm..." << endl;
	} 

	if(argv[2]) {
		imagePath = argv[2];
	} else {
		cerr << "Usage: ./client <imm_port> <path_to_image>" << endl;
		return -1;
	}

	boost::shared_ptr<TTransport> socket(new TSocket("localhost", port));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	ImageMatchingServiceClient client(protocol);
	
	struct timeval tv1, tv2;
	try{
		ifstream fin(imagePath.c_str(), ios::binary);
		ostringstream ostrm;
		ostrm << fin.rdbuf();
		string image(ostrm.str());
		if (!fin) cerr << "Could not open the file!" << endl;

		transport->open();
		string response;
		client.match_img(response, image);	
		cout << "client sent the image successfully..." << endl;
		cout << response << endl;
		
		transport->close();
	}catch(TException &tx){
		cout << "ERROR: " << tx.what() << endl;
	}

	return 0;
}
