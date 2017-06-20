#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "FACEHandler.h"
#include <folly/init/Init.h>
#include <cstdlib>
#include <iostream>

DEFINE_int32(num_of_threads,
             4,
             "Number of threads (default: 4)");

using namespace apache::thrift;
using namespace apache::thrift::async;

using namespace cpp2;
using namespace std;

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);

  if (argc != 2){
    cerr << "Wrong argument!" << endl;
    exit(EXIT_FAILURE);
  }
  int port = atoi(argv[1]);

  auto handler = std::make_shared<FACEHandler>();
  auto server = folly::make_unique<ThriftServer>();

  server->setPort(port);
  server->setNWorkerThreads(FLAGS_num_of_threads);
  server->setInterface(std::move(handler));
  server->setIdleTimeout(std::chrono::milliseconds(0));
  server->setTaskExpireTime(std::chrono::milliseconds(0));

  cout << "FACE at " << port << endl;

  server->serve();

  return 0;
}
