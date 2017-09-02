#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "FACEHandler.h"
#include <folly/init/Init.h>
#include "Parser.h"
#include <iostream>

DEFINE_int32(num_of_threads,
             4,
             "Number of threads (default: 4)");

using namespace apache::thrift;
using namespace apache::thrift::async;

using namespace cpp2;

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);

  Properties props;
  props.Read("../../config.properties");
  string portVal;
  int port;
  if (!props.GetValue("FACE_PORT", portVal)) {
    cout << "FACE port not defined" << endl;
    return -1;
  } else {
    port = atoi(portVal.c_str());
  }

  auto handler = std::make_shared<FACEHandler>();
  auto server = folly::make_unique<ThriftServer>();

  server->setPort(port);
  server->setNWorkerThreads(FLAGS_num_of_threads);
  server->setInterface(std::move(handler));
  server->setIdleTimeout(std::chrono::milliseconds(0));
  server->setTaskExpireTime(std::chrono::milliseconds(0));

  server->serve();

  return 0;
}
