#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <thrift/protocol/TBinaryProtocol.h>             
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TSocket.h>                    
#include <thrift/transport/TBufferTransports.h>          
#include <thrift/transport/TTransportUtils.h>

#include "gen-cpp/KaldiService.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;



int main(int argc, char ** argv){
	//Audio file from command line
	string audio_file;
	//Creating a server_port
	int server_port =9090;	
	if(argc==3){
		server_port = atoi(argv[1]);
		audio_file = argv[2];
	}
	else if (argc==2){
		audio_file = argv[1];
	}
	else{
		cerr << "At least provide a input file in .wav format" << endl;
		exit(0);
	}
	boost::shared_ptr<TTransport> socket(new TSocket("localhost", server_port));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
	KaldiServiceClient client(protocol);

	struct timeval tv1, tv2;
	try{	
		string answer;
	
		ifstream fin(audio_file.c_str(), ios::binary);
		if (!fin) cerr << "Could not open the file!" << endl;
	
		ostringstream ostrm;
  	ostrm << fin.rdbuf();
		string audio_file_to_send(ostrm.str());
		transport->open();
		gettimeofday(&tv1, NULL);
		client.kaldi_asr(answer, audio_file_to_send);
		gettimeofday(&tv2, NULL);
		unsigned int query_latency = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);	
		
		cout<< answer;
		// cout << "answer replied within " << fixed << setprecision(2) << (double)query_latency / 1000 << " ms" << endl;
		cout << fixed << setprecision(2) << (double)query_latency / 1000 << endl;

		transport->close();
	}catch (TException &tx){
		cout << "ERROR: " << tx.what() << endl;
	}
	return 0;
}
