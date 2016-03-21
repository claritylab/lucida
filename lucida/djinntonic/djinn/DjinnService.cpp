#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <map>
#include <vector>
#include <glog/logging.h>

#include "boost/thread.hpp"
#include "boost/bind.hpp"
#include "boost/program_options.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/algorithm/string/replace.hpp"

#include "../gen-cpp/LucidaService.h"
#include "../gen-cpp/lucidaservice_constants.h"
#include "../gen-cpp/lucidaservice_types.h"
#include "../gen-cpp/lucidatypes_constants.h"
#include "../gen-cpp/lucidatypes_types.h"
#include "../tonic-common/src/commons.h"
#include "../tonic-common/src/tonic.h"
#include "../tonic-common/src/timer.h"
#include "caffe/caffe.hpp"

#include "LucidaService.h"
#include "utils.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using caffe::Blob;
using caffe::Caffe;
using caffe::Net;
using caffe::Layer;
using caffe::shared_ptr;
using caffe::Timer;
using caffe::vector;

using namespace std;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

class LucidaServiceHandler : virtual public LucidaServiceIf
{
 public:
  LucidaServiceHandler() {
    this->SERVICE_NAME = "djinn";
    this->SERVICE_IP   = "localhost";
    this->SERVICE_PORT = 7071;
    this->solver       = "training/superfake_solver.prototxt";
    this->LUCID        = "superfake";
  }

  void create(const std::string& LUCID, const  ::QuerySpec& spec) {
    // make vector< pair location, class>
    // convert_imageset.cpp: converts a set inputs to leveldb/lmdb
    //    need as input: list of images and their labels
    //    create train/test split
    // set reasonable training batch size
    //
    // TODO: currently only handles images for training
    // TODO: change source of network to point to newly created databases
    // TODO: move this into functions
    // TODO: use incoming LUCID for each new database
    // TODO: needs heavy editing on solver and model definition to loop the
    // LUCID in and keep the number of iterations manageable
    // TODO: remove hardcoded paths
    // TODO: add ability to use finetuning instead of new network, probably the
    // way to go in this situation

    float split = 0.8; // split data coming in 80/20
    int num_train = split * spec.content[0].data.size();
    LOG(INFO) << "Total images: " << spec.content[0].data.size();
    LOG(INFO) << "Train images: " << num_train;

    fs::create_directory("training/data");

    // set up train lmdb
    vector<pair<string, int> > train;
    string db_path = "training/data/train_lmdb";
    fs::remove_all(db_path); // remove old lmdb if it already exists
    for (int i = 0; i < num_train; i++) {
      string data = spec.content[0].data[i];
      int label = atoi(spec.content[0].tags[i].c_str());
      train.push_back(make_pair(data, label));
    }

    convert_images(train, db_path.c_str());

    // set up val lmdb
    vector<pair<string, int> > val;
    db_path = "training/data/val_lmdb";
    fs::remove_all(db_path); // remove old lmdb if it already exists
    for (int i = num_train; i < spec.content[0].data.size(); i++) {
      string data = spec.content[0].data[i];
      int label = atoi(spec.content[0].tags[i].c_str());
      val.push_back(make_pair(data, label));
    }

    convert_images(val, db_path.c_str());
  }

  void learn(const std::string& LUCID, const  ::QuerySpec& knowledge) {
    Caffe::set_phase(Caffe::TRAIN);
    caffe::SolverParameter solver_param;
    caffe::ReadProtoFromTextFileOrDie(solver, &solver_param);

    shared_ptr<caffe::Solver<float> > solver(caffe::GetSolver<float>(solver_param));
    solver->Solve();
  }

  void infer(std::string& _return, const std::string& LUCID, const  ::QuerySpec& query) {
    Caffe::set_phase(Caffe::TEST);
    time_t rawtime;
    time(&rawtime);
    LOG(INFO) << "receiving djinn data at " << ctime(&rawtime);

    int64_t start_time, end_time;

    for (int i = 0; i < query.content.size(); i++){
      struct timeval now;
      gettimeofday(&now, NULL);
      start_time = (now.tv_sec*1E6+now.tv_usec) / 1000;
      string str = request_handler(query.content[i]);
      _return += str;
      gettimeofday(&now, 0);
      end_time = (now.tv_sec*1E6+now.tv_usec) / 1000;
    }
  }

  void init(po::variables_map vm){
    this->SERVICE_PORT      = vm["port"].as<int>();
    this->SERVICE_IP        = vm["svip"].as<string>();
    this->vm = vm;

    if (vm["gpu"].as<bool>())
      Caffe::set_mode(Caffe::GPU);
    else
      Caffe::set_mode(Caffe::CPU);

    // load all models at init
    ifstream file(vm["nets"].as<string>().c_str());
    string net_name;
    while (file >> net_name) {
      string net = vm["common"].as<string>() + "configs/" + net_name;
      Net<float>* temp = new Net<float>(net);
      const std::string name = temp->name();
      nets[name] = temp;
      std::string weights = vm["common"].as<string>() +
                            vm["weights"].as<string>() + name + ".caffemodel";
      nets[name]->CopyTrainedLayersFrom(weights);
    }
  }

private:
  string SERVICE_NAME;
  string SERVICE_IP;
  string LUCID;
  string solver;
  int SERVICE_PORT;    
  po::variables_map vm;
  map<string, Net<float>*> nets;

  string request_handler(const QueryInput &q_in) {
    // 1. Client sends the application type
    // 2. Client sends the size of incoming data
    // 3. Client sends data

    //q_in tags:
    // 0. request name 
    // 1. length of data
    string reqname = q_in.tags[0];
    LOG(INFO) << "The reqname is "  << reqname;
    int sock_elts = atoi(q_in.tags[1].c_str());
    LOG(INFO) << "The data size is " << sock_elts;

    string data = q_in.data[0];

    map<string, Net<float>*>::iterator it = nets.find(reqname);
    if (it == nets.end()) {
      LOG(ERROR) << "Task " << reqname << " not found.";
      return (void*)1;
    } else
      LOG(INFO) << "Task " << reqname << " forward pass.";

    // reshape input dims if incoming data != current net config
    LOG(INFO) << "Elements received on socket " << sock_elts << "\n";

    reshape(nets[reqname], sock_elts); 
    int in_elts = nets[reqname]->input_blobs()[0]->count();
    int out_elts = nets[reqname]->output_blobs()[0]->count();
    float* in = (float*)malloc(in_elts * sizeof(float));
    float* out = (float*)malloc(out_elts * sizeof(float));
   
    LOG(INFO) << "Starting lexical_cast...";
    istringstream ins(data);
    string buff;
    for (int i = 0; i < in_elts; i++){
      ins >> buff;
      float temp = boost::lexical_cast<float>(buff);
      * (in + i) = temp;
    }

    // Main loop of the thread, following this order
    // 1. Receive input feature (has to be in the size of sock_elts)
    // 2. Do forward pass
    // 3. Send back the result
    // 4. Repeat 1-3

    LOG(INFO) << "Executing forward pass.";

    SERVICE_fwd(in, in_elts, out, out_elts, nets[reqname]);

    LOG(INFO) << "Writing to return string.";
    string str;
    for (int i = 0; i < out_elts; i++){
      float b = *(out + i);
      string temp_str = boost::lexical_cast<string>(b);
      str = str + temp_str + " ";
    }

    // Exit the thread

    free(in);
    free(out);
    return str;
  }

  void SERVICE_fwd(float* in, int in_size, float* out, int out_size,
      Net<float>* net) {
    string net_name = net->name();
    STATS_INIT("service", "DjiNN service inference");
    PRINT_STAT_STRING("network", net_name.c_str());

    if (Caffe::mode() == Caffe::CPU)
      PRINT_STAT_STRING("platform", "cpu");
    else
      PRINT_STAT_STRING("platform", "gpu");

    float loss;
    vector<Blob<float>*> in_blobs = net->input_blobs();

    tic();
    in_blobs[0]->set_cpu_data(in);
    vector<Blob<float>*> out_blobs = net->ForwardPrefilled(&loss);

    PRINT_STAT_DOUBLE("inference latency", toc());

    STATS_END();

    if (out_size != out_blobs[0]->count())
      LOG(FATAL) << "out_size =! out_blobs[0]->count())";
    else
      memcpy(out, out_blobs[0]->cpu_data(), out_size * sizeof(float));
  }

};

po::variables_map parse_opts(int ac, char** av)
{
  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message")
      ("port,p", po::value<int>()->default_value(5000),
          "Service port number")
      ("svip,s", po::value<string>()->default_value("localhost"),
          "Service IP address")
      ("common,m", po::value<string>()->default_value("../tonic-common/"),
          "Directory with configs and weights")
      ("nets,n", po::value<string>()->default_value("nets.txt"),
          "File with list of network configs (.prototxt/line)")
      ("weights,w", po::value<string>()->default_value("weights/"),
          "Directory containing weights (in common)")
      ("gpu,g", po::value<bool>()->default_value(false), "Use GPU?")
      ("debug,v", po::value<bool>()->default_value(false),
          "Turn on all debug");

  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    exit(1);
  }
  return vm;
}

int main(int argc, char **argv)
{
  po::variables_map vm = parse_opts(argc, argv);
  
  LucidaServiceHandler *DjinnService = new LucidaServiceHandler();
  shared_ptr<LucidaServiceHandler> handler(DjinnService);
  shared_ptr<TProcessor> processor(new LucidaServiceProcessor(handler));

  TServers tServer;
  thread thrift_server;
  tServer.launchSingleThreadThriftServer(vm["port"].as<int>(), processor, thrift_server);
  DjinnService->init(vm);

  LOG(INFO) << "Init done." << endl;
  LOG(INFO) << "Starting the Djinn service at " << vm["svip"].as<string>() << ":"
            << vm["port"].as<int>() 
            << endl;
  thrift_server.join();

  return 0;
}
