#include <gflags/gflags.h>
#include <glog/logging.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "IMCHandler.h"
#include <folly/init/Init.h>
#include "mongo/client/dbclient.h"
#include <iostream>

DEFINE_int32(num_of_threads,
             4,
             "Number of threads (default: 4)");

using namespace apache::thrift;
using namespace apache::thrift::async;

using namespace cpp2;
using namespace mongo;
using namespace std;
//using namespace facebook::windtunnel::treadmill::services::imc;

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);

  // Initialize MongoDB C++ driver.
  client::initialize();
  DBClientConnection conn;
  string mongo_addr;
  if (const char* env_p = getenv("MONGO_PORT_27017_TCP_ADDR")) {
    cout << "MongoDB: " << env_p << endl;
    mongo_addr = env_p;
  } else {
    cout << "MongoDB: localhost" << endl;
    mongo_addr = "localhost";
  }
  conn.connect(mongo_addr);
  cout << "Connection is ok" << endl;
  auto_ptr<DBClientCursor> cursor = conn.query(
      "lucida.service_info", MONGO_QUERY("name" << "imageclassification"));
  BSONObj p;
  int port = 0;
  while (cursor->more()) {
    p = cursor->next();
    string port_str = p.getField("port").String();
    port = atoi(port_str.c_str());
  }

  auto handler = std::make_shared<IMCHandler>();
  auto server = folly::make_unique<ThriftServer>();

  server->setPort(port);
  server->setNWorkerThreads(FLAGS_num_of_threads);
  server->setInterface(std::move(handler));
  server->setIdleTimeout(std::chrono::milliseconds(0));
  server->setTaskExpireTime(std::chrono::milliseconds(0));

  cout << "IMC at " << port << endl;

  server->serve();

  return 0;
}
