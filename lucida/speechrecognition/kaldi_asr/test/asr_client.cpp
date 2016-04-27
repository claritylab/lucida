/*
 * Implementation for the ASR testing client.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <thrift/protocol/TBinaryProtocol.h>             
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TSocket.h>                    
#include <thrift/transport/TBufferTransports.h>          
#include <thrift/transport/TTransportUtils.h>

#include "gen-cpp/LucidaService.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

int main(int argc, char ** argv){
	// Creating a server_port
	int server_port = 8081;
	if (argc != 3) {
		cerr << "Usage: ./asr_client <asr_port> <path_to_audio>" << endl;
		exit(1);
	}
	server_port = atoi(argv[1]);
	string audio_path = argv[2];
	// Initialize Thrift objects.
	boost::shared_ptr<TTransport> socket(new TSocket("localhost", server_port));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	LucidaServiceClient client(protocol);
	try {
		// Open the audio.
		ifstream fin(audio_path.c_str(), ios::binary);
		if (!fin) cerr << "Could not open the file!" << endl;
		ostringstream ostrm;
		ostrm << fin.rdbuf();
		string audio_file_to_send(ostrm.str());
		// Prepare to interact with the ASR server.
		transport->open();
		string answer;
		// Create a QueryInput and a QuerySpec.
		QueryInput query_input;
		query_input.type = "audio";
		query_input.data.push_back(audio_file_to_send);
		QuerySpec query_spec;
		query_spec.content.push_back(query_input);
		string LUCID = "QLL";
		// Invoke the infer service.
		cout << "Sending audio " << audio_path << " to the server..." << endl;
		client.infer(answer, LUCID, query_spec);
		cout << "Result: " << answer << endl;
		// Close the connection.
		transport->close();
	} catch (TException &tx){
		cout << "ERROR: " << tx.what() << endl;
	}
	return 0;
}
