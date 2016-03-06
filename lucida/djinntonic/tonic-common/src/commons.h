#ifndef COMMONS_H
#define COMMONS_H

#include "../../gen-cpp/LucidaService.h"
#include <string>
#include <boost/thread.hpp>
#include <iostream>

// use for TNonblockingServer
// #include <thrift/protocol/TBinaryProtocol.h>
// #include <thrift/server/TNonblockingServer.h>
// #include <thrift/transport/TServerSocket.h>
// #include <thrift/transport/TTransportUtils.h>
// #include <thrift/TToString.h>

#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TSocket.h>
#include <thrift/TToString.h>


using namespace std;
using namespace boost;

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

class TClient {
	private:
		int TIMEOUT;
	public:
		TClient();
		LucidaServiceClient *creatLucidaClient(string host, int port);
};

class TServers {
	private:
		// void buildNonBlockingServer(int port, TProcessor *processor);
	public:
		TServers();
    void launchSingleThreadThriftServer(int port, boost::shared_ptr<TProcessor>
        &processor, thread &thrift_server);
};

#endif
