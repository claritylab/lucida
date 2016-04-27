/*
 * Implementation for the Automatic Speech Recognition daemon.
 */

// Import utility headers.
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sox.h>
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
#include <ctime>

#include "gen-cpp/LucidaService.h"
#include "../kaldi/subproc.h"
//#include "CommandCenter.h"

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

// Define the namespaces.
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
// using boost::shared_ptr;
// using namespace cmdcenterstubs;

// define the constant
#define THREAD_WORKS 16

class LucidaServiceHandler : virtual public LucidaServiceIf {
public:
	cmd_subproc kaldi;
	string answer;

	/*
	 * Default constructor.
	 */
	LucidaServiceHandler();

	/*
	 * Custom constructor.
	 * Executes a binary in a form of an array.
	 */
	LucidaServiceHandler(const char* const argv[]);

	/*
	 * Creates a new service instance.
	 * TODO
	 */
	void create(const string& LUCID, const ::QuerySpec& spec);


	/*
	 * Learns new knowledge.
	 * TODO
	 */
	void learn(const string& LUCID, const ::QuerySpec& knowledge);

	/*
	 * Infers from query by matching the image using opencv2.
	 */
	void infer(string& _return, const string& LUCID, const ::QuerySpec& query);


private:
	/*
	 * Runs the sox() library to convert audio files to  8000Hz and normalize.
	 */
	void sox(const string &input_audio, const string &input_audio_1);

	/*
	 * Extracts the audio data from query.
	 * Throws runtime_error if query content is empty,
	 * or query content's type is not "audio".
	 */
	void extractAudioFromQuery(const ::QuerySpec& query, string &audio);
};

LucidaServiceHandler::LucidaServiceHandler() {};

LucidaServiceHandler::LucidaServiceHandler(const char* const argv[]) {
	kaldi.cmd_exe(argv,false);			
}

void LucidaServiceHandler::create(
		const string& LUCID, const ::QuerySpec& spec) {
	cout << "Create" << endl;
}

void LucidaServiceHandler::learn(
		const string& LUCID, const ::QuerySpec& knowledge) {
	cout << "Learn" << endl;
}

void LucidaServiceHandler::infer(
		string& _return, const string& LUCID, const ::QuerySpec& query) {
	// Extract audio data from query.
	string audio;
	try {
		extractAudioFromQuery(query, audio);
	} catch (exception &e) {
		cout << e.what() << "\n";
		_return = "Error! " + string(e.what());
		return;
	}

	// Reconstruct the audio file.
	time_t now = time(0);
	string audio_path = "inputserver_" + to_string(now) + ".wav";
	string audio_path_1 = "inputserver1_" + to_string(now) + ".wav";
	ofstream audiofile(audio_path.c_str(), ios::binary);
	audiofile.write(audio.c_str(),audio.size());
	audiofile.close();
	cout << "Audio File Recieved..."<< endl;

	cout << "Converting audio file...." << audio_path << endl;
	sox(audio_path, audio_path_1); // running SOX
	cout << "Converting audio file complete..." << audio_path_1 << endl;

	cout << "Running Kaldi Algorithm..."<< endl;
	kaldi.stdin << audio_path_1 << endl; // giving the audio file via stdin (pipes)
	getline(kaldi.stdout, answer);// grabbing the answer
	cout << "Finished Running Kaldi Algorithm..."<< endl;
	cout << "Now Returing Answer: "<< answer << endl;

	_return = answer;

	// Delete the temporary audio files from disk.
	if (remove(audio_path.c_str()) == -1) {
		cout << audio_path << " can't be deleted from file system." << endl;
	}
	if (remove(audio_path_1.c_str()) == -1) {
		cout << audio_path_1 << "can't be deleted from file system." << endl;
	}
}

void LucidaServiceHandler::extractAudioFromQuery(
		const ::QuerySpec& query, string &audio) {
	if (query.content.empty()) {
		throw runtime_error("Empty query is not allowed.");
	}
	if (query.content.front().type != "audio") {
		throw runtime_error(query.content[0].type + " type is not allowed. "
				+ "Type must be \"audio\".");
	}
	audio = query.content.front().data.front(); // the rest is ignored
}

void LucidaServiceHandler::sox(const string &input_audio,
		const string &input_audio_1) {
	static sox_format_t * in, * out; /* input and output files */
	sox_effects_chain_t * chain;
	sox_effect_t * e;
	char * args[10];
	sox_signalinfo_t interm_signal; /* @ intermediate points in the chain. */

	sox_signalinfo_t out_signal = {
			8000,
			1,
			SOX_DEFAULT_PRECISION,
			SOX_UNKNOWN_LEN,
			NULL
	};

	assert(sox_init() == SOX_SUCCESS);
	assert(in = sox_open_read(input_audio.c_str(), NULL, NULL, NULL));
	assert(out = sox_open_write(
			input_audio_1.c_str(), &out_signal, NULL, NULL, NULL, NULL));
	chain = sox_create_effects_chain(&in->encoding, &out->encoding);

	interm_signal = in->signal; /* NB: deep copy */

	e = sox_create_effect(sox_find_effect("input"));
	args[0] = (char *)in, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
	assert(sox_add_effect(chain, e, &in->signal, &in->signal) == SOX_SUCCESS);
	free(e);

	if (in->signal.rate != out->signal.rate) {
		e = sox_create_effect(sox_find_effect("rate"));
		assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
		assert(sox_add_effect(chain, e, &interm_signal, &out->signal)
				== SOX_SUCCESS);
		free(e);
	}

	if (in->signal.channels != out->signal.channels) {
		e = sox_create_effect(sox_find_effect("channels"));
		assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
		assert(sox_add_effect(chain, e, &interm_signal, &out->signal)
				== SOX_SUCCESS);
		free(e);
	}

	// Create the `flanger' effect, and initialize it with default parameters.
	e = sox_create_effect(sox_find_effect("norm"));
	assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
	// Add the effect to the end of the effects processing chain.
	assert(sox_add_effect(chain, e, &interm_signal, &out->signal)
			== SOX_SUCCESS);
	free(e);

	e = sox_create_effect(sox_find_effect("output"));
	args[0] = (char *)out, assert(sox_effect_options(e, 1, args)
			== SOX_SUCCESS);
	assert(sox_add_effect(chain, e, &interm_signal, &out->signal)
			== SOX_SUCCESS);
	free(e);

	sox_flow_effects(chain, NULL, NULL);
	// Clean up.
	sox_delete_effects_chain(chain);
	sox_close(out);
	sox_close(in);
	sox_quit();
}


int main(int argc, char **argv) {

	const char* const argvc[]={"../kaldi/src/online2bin/online2-wav-nnet2-latgen-faster",
			"--do-endpointing=false",
			"--online=true",
			"--config=../kaldi/nnet_a_gpu_online/conf/online_nnet2_decoding.conf",
			"--max-active=7000",
			"--beam=15.0",
			"--lattice-beam=6.0",
			"--acoustic-scale=0.1",
			"--word-symbol-table=../kaldi/graph/words.txt",
			"../kaldi/nnet_a_gpu_online/smbr_epoch2.mdl",
			"../kaldi/graph/HCLG.fst",
			"\"ark:echo utterance-id1 utterance-id1|\"",
			"\"scp:echo utterance-id1 null|\"",
			"ark:/dev/null",(char*)NULL};

	// Register with the command center. TODO
	int port = 8081;
	if (argc >= 2) {
		port = atoi(argv[1]);
	}
	// int ccport = atoi(argv[2]);

	//Register with the command center 
	//	boost::shared_ptr<TTransport> cmdsocket(new TSocket("localhost", cmdcenterport));
	//	boost::shared_ptr<TTransport> cmdtransport(new TBufferedTransport(cmdsocket));
	//	boost::shared_ptr<TProtocol> cmdprotocol(new TBinaryProtocol(cmdtransport));
	//	CommandCenterClient cmdclient(cmdprotocol);
	//	cmdtransport->open();
	//	std::cout << "Registering automatic speech recognition server with command center..." << std::endl;
	//	MachineData mDataObj;
	//	mDataObj.name="localhost";
	//	mDataObj.port=port;
	//	cmdclient.registerService("ASR", mDataObj);
	//	cmdtransport->close();

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
			new LucidaServiceHandler(argvc));
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

	cout << "Start listening to requests at port " << port << endl;
	serverThread.join();
	return 0;
}

