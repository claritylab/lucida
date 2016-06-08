#include <memory>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>

DEFINE_int32(port,
             9001,
             "Port for ASR service (default: 9001)");

DEFINE_int32(num_of_threads,
             4,
             "Number of threads (default: 4)");

#include "ASRHandler.h"

using namespace apache::thrift;
using namespace apache::thrift::async;

using namespace cpp2;
//using namespace facebook::windtunnel::treadmill::services::asr;

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  auto handler = std::make_shared<ASRHandler>();
  auto server = folly::make_unique<ThriftServer>();

  server->setPort(FLAGS_port);
  server->setNWorkerThreads(FLAGS_num_of_threads);
  server->setInterface(std::move(handler));
  server->setIdleTimeout(std::chrono::milliseconds(0));
  server->setTaskExpireTime(std::chrono::milliseconds(0));

  server->serve();

  return 0;
}
