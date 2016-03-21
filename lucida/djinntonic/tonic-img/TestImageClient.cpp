#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>

// import common utility headers
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <fstream>
#include <sys/time.h>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <glog/logging.h>

#include <stdlib.h>
#include <time.h>
#include <map>

// import opencv headers
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "boost/program_options.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/algorithm/string/replace.hpp"
#include "boost/lexical_cast.hpp"

// import the service headers
#include "../gen-cpp/LucidaService.h"
#include "../gen-cpp/lucidaservice_constants.h"
#include "../gen-cpp/lucidaservice_types.h"
#include "../gen-cpp/lucidatypes_constants.h"
#include "../gen-cpp/lucidatypes_types.h"
#include "../tonic-common/src/commons.h"
#include "../tonic-common/src/ServiceTypes.h"
#include "../tonic-common/src/serialize.h"

// define the namespace
using namespace std;
using namespace cv;

namespace po = boost::program_options;

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

//extracts cmd args for the client and pushes others to tags for service
string set_tags(int argc, char** argv, vector<string> &tags){
  string ret;
  for(int i = 1; i < argc; ++i){
    if(!strcmp(argv[i],"--file") || !strcmp(argv[i], "-f")){
      ret = argv[i + 1];
      ++i;
    }
    else{
      tags.push_back(argv[i]);
    }
  }
  return ret;
}

//parse command line arguments from server side
po::variables_map parse_opts(int ac, char** av) {
  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message")
      ("port,p", po::value<int>()->default_value(8080),
          "Tonic-IMG service port")
      ("svip,s", po::value<string>()->default_value("localhost"),
          "Tonic-IMG service IP")
      ("task,t", po::value<string>()->default_value("imc"),
          "imc|face|dig")
      ("file,f", po::value<string>()->default_value("common-img/imc-list.txt"),
          "list of inputs")
        ;

  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    exit(1);
  }
  return vm;
}

int main(int argc, char** argv){
  ServiceTypes svt;
  vector<string> tags;

  po::variables_map vm = parse_opts(argc, argv);

  string filename = set_tags(argc, argv, tags);
  cout << filename << endl;

  try {
    TClient tClient;
    LucidaServiceClient * imgClient = NULL;
    imgClient = tClient.creatLucidaClient(vm["svip"].as<string>(),
        vm["port"].as<int>());

    //set up the queryspec
    QuerySpec query;
    query.__set_name("test-img");

    string type = svt.SERVICE_INPUT_IMAGE;

    vector<string> data;

    QueryInput impQueryInput;

    impQueryInput.__set_type(type);
    impQueryInput.__set_tags(tags);
    LOG(INFO) << "Successfully parsed and set tags";

    //loop through all files listed and set data
    ifstream file(vm["file"].as<string>().c_str());
    string img_file;
    while (getline(file, img_file)) {
      LOG(INFO) << "filename: " << img_file;
      Mat img = imread(img_file);

      string instring = serialize(img, img_file);

      data.push_back(instring);
    }
    impQueryInput.__set_data(data);
    LOG(INFO) << "Successfully opened file and set input";

    query.content.push_back(impQueryInput);

    string outstring;
    LOG(INFO) << "Calling submit query... "<< endl;

    imgClient->infer(outstring, "superfake", query);
    cout << outstring << endl;
  } catch (TException& tx) {
    cout << "Could not talk to image" << endl;    
  }

    return 0;
}
