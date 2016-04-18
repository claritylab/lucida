/*
* Kaldi Server Using Apache Thrift
*/

#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <fstream>
#include <sstream>
#include <sox.h>
#include <cstdlib> //07-12-15 for arg passing

#include "gen-cpp/KaldiService.h"
#include "../common/subproc.h"
#include "CommandCenter.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
//#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using boost::shared_ptr;
using namespace cmdcenterstubs;


class KaldiServiceHandler : virtual public KaldiServiceIf {
  public:
    //Variables
    cmd_subproc kaldi;
    std::string answer;

    //----Constructors-----//

    //default 
    KaldiServiceHandler();

    //executes a binary in a form of an array
    KaldiServiceHandler(const char* const argv[]);

    //-----Member Functions-------//

    void kaldi_asr(std::string& _return, const std::string& audio_file);
    void sox();

};

//Default Constructor
KaldiServiceHandler::KaldiServiceHandler(){};
    
//Constructor to execute binary
KaldiServiceHandler::KaldiServiceHandler(const char* const argv[]){
  kaldi.cmd_exe(argv,false);			
}

//Member Functions

//An Input is the  file in a form of a string
//processes the file and return the answer in the form of 
//text
void KaldiServiceHandler::kaldi_asr(std::string& _return, const std::string& audio_file) {
  
//Reconstructing the audio file
  std::string audio_path = "inputserver.wav";
  std::ofstream audiofile(audio_path.c_str(), std::ios::binary);
  audiofile.write(audio_file.c_str(),audio_file.size());
  audiofile.close();
  std::cout << "Audio File Recieved..."<< std::endl;

  std::cout << "Converting audio file...." << std::endl;
  sox();//Running SOX
  std::cout << "Converting audio file complete..." << std::endl;

  std::cout << "Running Kaldi Algorithm..."<< std::endl;
  kaldi.stdin << "inputserver1.wav"  << std::endl;//Giving the audio file via stdin (pipes)
  getline(kaldi.stdout, answer);//Grabbing the answer	
  std::cout << "Finished Running Kaldi Algorithm..."<< std::endl;	
  std::cout << "Now Returing Answer: "<<answer<< std::endl;
  _return = answer;
}
  
//Run the sox() library to conver audio files to
//8000Hz and normalize
void KaldiServiceHandler::sox(){
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
  assert(in = sox_open_read("inputserver.wav", NULL, NULL, NULL));
  assert(out = sox_open_write("inputserver1.wav", &out_signal, NULL, NULL, NULL, NULL));
  chain = sox_create_effects_chain(&in->encoding, &out->encoding);
  
  interm_signal = in->signal; /* NB: deep copy */

  e = sox_create_effect(sox_find_effect("input"));
  args[0] = (char *)in, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &in->signal, &in->signal) == SOX_SUCCESS);
  free(e);

  if (in->signal.rate != out->signal.rate) {
    e = sox_create_effect(sox_find_effect("rate"));
    assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
    free(e);
  }
    
  if (in->signal.channels != out->signal.channels) {
    e = sox_create_effect(sox_find_effect("channels"));
    assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
    free(e);
  }

  /* Create the `flanger' effect, and initialise it with default parameters: */
  e = sox_create_effect(sox_find_effect("norm"));
  assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
  /* Add the effect to the end of the effects processing chain: */
  assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
  free(e);

  e = sox_create_effect(sox_find_effect("output"));
  args[0] = (char *)out, assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
  assert(sox_add_effect(chain, e, &interm_signal, &out->signal) == SOX_SUCCESS);
  free(e);

  sox_flow_effects(chain, NULL, NULL);
  //Cleaning up
  sox_delete_effects_chain(chain);
  sox_close(out);
  sox_close(in);
  sox_quit();
}


int main(int argc, char **argv) {

  const char* const argvc[]={"../common/src/online2bin/online2-wav-nnet2-latgen-faster",
    "--do-endpointing=false",
    "--online=true",
    "--config=../common/nnet_a_gpu_online/conf/online_nnet2_decoding.conf",
    "--max-active=7000",
    "--beam=15.0",
    "--lattice-beam=6.0",
    "--acoustic-scale=0.1",
    "--word-symbol-table=../common/graph/words.txt",
    "../common/nnet_a_gpu_online/smbr_epoch2.mdl", 
    "../common/graph/HCLG.fst",
    "\"ark:echo utterance-id1 utterance-id1|\"",
    "\"scp:echo utterance-id1 null|\"",
    "ark:/dev/null",(char*)NULL};

  int port = atoi(argv[1]);
  int cmdcenterport = atoi(argv[2]);

  //Register with the command center 
  boost::shared_ptr<TTransport> cmdsocket(new TSocket("localhost", cmdcenterport));
  boost::shared_ptr<TTransport> cmdtransport(new TBufferedTransport(cmdsocket));
  boost::shared_ptr<TProtocol> cmdprotocol(new TBinaryProtocol(cmdtransport));
  CommandCenterClient cmdclient(cmdprotocol);
  cmdtransport->open();	
  std::cout << "Registering automatic speech recognition server with command center..." << std::endl;
  MachineData mDataObj;
  mDataObj.name="localhost";
  mDataObj.port=port;
  cmdclient.registerService("ASR", mDataObj);
  cmdtransport->close();

  shared_ptr<KaldiServiceHandler> handler(new KaldiServiceHandler(argvc));
  shared_ptr<TProcessor> processor(new KaldiServiceProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    
  std::cout << "Starting the automatic speech recognition server on port " << port << "..." << std::endl;
  server.serve();
  return 0;
}
