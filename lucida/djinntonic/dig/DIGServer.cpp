#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "DIGHandler.h"
#include <folly/init/Init.h>
#include "Parser.h"
#include <iostream>

DEFINE_int32(num_of_threads,
             4,
             "Number of threads (default: 4)");

using namespace apache::thrift;
using namespace apache::thrift::async;

using namespace cpp2;
//using namespace facebook::windtunnel::treadmill::services::dig;

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);

  Properties props;
  props.Read("../../config.properties");
  string portVal;
  int port;
  if (!props.GetValue("DIG_PORT", portVal)) {
    cout << "DIG port not defined" << endl;
    return -1;
  } else {
    port = atoi(portVal.c_str());
  }

  auto handler = std::make_shared<DIGHandler>();
  auto server = folly::make_unique<ThriftServer>();

  server->setPort(port);
  server->setNWorkerThreads(FLAGS_num_of_threads);
  server->setInterface(std::move(handler));
  server->setIdleTimeout(std::chrono::milliseconds(0));
  server->setTaskExpireTime(std::chrono::milliseconds(0));

  server->serve();

  return 0;
}
