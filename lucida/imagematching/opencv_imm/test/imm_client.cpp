/*
 * Implementation for the IMM testing client.
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
	if (argc != 3) {
		cerr << "Usage: ./imm_client <imm_port> <path_to_image>" << endl;
		exit(1);
	}
	int server_port = atoi(argv[1]);
	string image_path = argv[2];
	// Initialize Thrift objects.
	boost::shared_ptr<TTransport> socket(new TSocket("localhost", server_port));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	LucidaServiceClient client(protocol);
	try {
		// Open the image.
		ifstream fin(image_path.c_str(), ios::binary);
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
		string LUCID = "QLL";
		// Invoke the infer service.
		cout << "Sending image " << image_path << " to the server..." << endl;
		client.infer(response, LUCID, query_spec);
		// Print the result.
		cout << "Result: " << response << endl;
		// Close the connection.
		transport->close();
	} catch (TException &tx){
		cout << "ERROR: " << tx.what() << endl;
	}
	return 0;
}
