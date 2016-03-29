/*
 * Implementation for the Image Matching service testing client.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
 
#include "gen-cpp/LucidaService.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

int main(int argc, char** argv) {
	// shared_ptr the port number and  image path.
	int port = 8081;
	string imagePath;
	if (argc != 3) {
		cerr << "Usage: ./client <imm_port> <path_to_image>" << endl;
		return 1;
	}
	port = atoi(argv[1]);
	imagePath = argv[2];
	// Initialize Thrift objects.
	boost::shared_ptr<TTransport> socket(new TSocket("localhost", port));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	LucidaServiceClient client(protocol);
	try {
		// Open the image.
		ifstream fin(imagePath.c_str(), ios::binary);
		ostringstream ostrm;
		ostrm << fin.rdbuf();
		string image(ostrm.str());
		if (!fin) {
			cerr << "Could not open the file!" << endl;
			return 1;
		}
		// Prepare to interact with the IMM server.
		transport->open();
		string response;
		// Create a QueryInput and a QuerySpec.
		QueryInput query_input;
		query_input.type = "image";
		query_input.data.push_back(image);
		QuerySpec query_spec;
		query_spec.content.push_back(query_input);
		string LUCID = "Johann";
		// Invoke the infer service.
		client.infer(response, LUCID, query_spec);
		// Print the result.
		cout << "Client sent the image successfully..." << endl;
		cout << response << endl;
		// Close the connection.
		transport->close();
	} catch(TException &tx){
		cout << "ERROR: " << tx.what() << endl;
	}

	return 0;
}
