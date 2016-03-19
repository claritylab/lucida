#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <getopt.h> //for getopt_long()

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

// Thrift-generated stubs for RPC handling
#include "gen-cpp/CommandCenter.h"

using namespace std; 
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace cmdcenterstubs;

///////////////////////////////////////////////////////////////////////////////

void printHelpMsg();
void runQaTest(int port, string question);
void runAsrQaTest(int port, string audioFile);
void runImmAsrQaTest(int port, string audioFile, string imgFile);

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
	// getopt_long() for options parsing: Getopt makes it easier
	// to have options that may take arguments, as well as long and
	// short options that share the same meaning.
	bool qa_test = false;
	bool asr_test = false;
	bool imm_test = false;
	string qa_arg = "";
	string asr_arg = "";
	string imm_arg = "";
	int c;
	while (1) {
		static struct option long_options[] = {
			// These options don't set a flag
			{"qa", required_argument, 0, 'q'},
			{"asr", required_argument, 0, 'a'},
			{"imm", required_argument, 0, 'i'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		// getopt_long stores the option index here.
		int option_index = 0;

		c = getopt_long(argc, argv, "q:a:i:h", long_options, &option_index);
		
		// Detect end of options
		if (c == -1) {
			break;
		}

		switch (c) {
			case 'q':
				cout << "Client will run QA test with input " << optarg << endl;
				qa_test = true;
				qa_arg.assign(optarg);
				break;
			case 'a':
				cout << "Client will run ASR test with input " << optarg << endl;
				asr_test = true;
				asr_arg.assign(optarg);
				break;
			case 'i':
				cout << "Client will run IMM test with input " << optarg << endl;
				imm_test = true;
				imm_arg.assign(optarg);
				break;
			case 'h':
				printHelpMsg();
				return 0;
			case '?':
				// getopt_long has already printed an error message
				break;
			default:
				abort();
		}
	} // End while loop for options parsing

	// Collect the port number after options have been parsed
	int port = -1;
	if (optind < argc) {
		port = atoi(argv[optind]);
		cout << "Port = " << port << endl;
	} else {
		cout << "Please specify a port number" << endl;
		printHelpMsg();
		exit(1);
	}

	// Run tests specified by the user
	if (asr_test && imm_test) {
		cout << "Running ASR-IMM-QA pipeline..." << endl;
		runImmAsrQaTest(port, asr_arg, imm_arg);
	} else if (asr_test) {
		cout << "Running ASR-QA pipeline..." << endl;
		runAsrQaTest(port, asr_arg);
	} else if (qa_test) {
		cout << "Running QA test..." << endl;
		runQaTest(port, qa_arg);
	} else {
		cout << "This combination of tests is not allowed" << endl;
		printHelpMsg();
		exit(1);
	}
	return 0;
}

void printHelpMsg() {
  cout << "Usage: ./ccclient [--qa=(QUESTION)|--asr=(AUDIO_FILE)|--imm=(IMG_FILE)] (PORT)" << endl
    << "The following tests can be run: asr, qa, asr-imm" << endl
    << "Any other combination of tests is not supported." << endl;
}

void runQaTest(int port, string question) {
  string answer;
  // Initialize thrift RPC objects
  boost::shared_ptr<TTransport> socket(new TSocket("localhost", port));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  CommandCenterClient client(protocol);

  try {
    QueryData query;
    query.audioData = "";
    query.textData = question;
    query.imgData = "";

    // Talk to Sirius
    transport->open();
    cout << "///// QA /////" << endl;
    client.handleRequest(answer, query);
    cout << "Your answer is: " << answer << endl;
    transport->close();
  } catch (TException &tx) {
    //NOTE: execution resumes AFTER try-catch block
    cout << "CLIENT ERROR: " << tx.what() << endl;
  }
}

void runAsrQaTest(int port, string audioFile) {
  string answer;
  // Initialize thrift RPC objects
  boost::shared_ptr<TTransport> socket(new TSocket("localhost", port));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  CommandCenterClient client(protocol);

  try {
    //Audio File
    ifstream fin_audio(audioFile.c_str(), ios::binary);
    ostringstream ostrm_audio;
    ostrm_audio << fin_audio.rdbuf();
    string audioData(ostrm_audio.str());

    QueryData query;
    query.audioData = audioData;
    query.textData = "";
    query.imgData = "";

    // Talk to Sirius
    transport->open();
    cout << "///// ASR-QA /////" << endl;
    client.handleRequest(answer, query);
    cout << "Your answer is: " << answer << endl;
    transport->close();
  } catch (TException &tx) {
    cout << "CLIENT ERROR: " << tx.what() << endl;
  }
}

void runImmAsrQaTest(int port, string audioFile, string imgFile) {
  string answer;
  // Initialize thrift RPC objects
  boost::shared_ptr<TTransport> socket(new TSocket("localhost", port));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  CommandCenterClient client(protocol);
  try {
    // Audio File
    ifstream fin_audio(audioFile.c_str(), ios::binary);
    ostringstream ostrm_audio;
    ostrm_audio << fin_audio.rdbuf();
    string audioData(ostrm_audio.str());	

    //Image File
    ifstream fin_image(imgFile.c_str(), ios::binary);
    ostringstream ostrm_image;
    ostrm_image << fin_image.rdbuf();
    string imgData(ostrm_image.str());

    QueryData query;
    query.audioData = audioData;
    query.textData = "";
    query.imgData = imgData;

    // Talk to Sirius
    transport->open();
    cout << "\n///// ASR-QA-IMM /////" << endl;
    client.handleRequest(answer, query);
    cout << "Your answer is: " << answer << endl;
    transport->close();
  } catch (TException &tx) {
    cout << "CLIENT ERROR: " << tx.what() << endl;
  }
}
