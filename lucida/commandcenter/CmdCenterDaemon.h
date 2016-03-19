#ifndef CmdCenterDaemon_H
#define CmdCenterDaemon_H
// C++ thrift headers 
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>
#include <thrift/processor/TMultiplexedProcessor.h>

// Additional C++ headers for querying registered services
#include <thrift/transport/TSocket.h>

// Useful C++ headers
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstdlib>

// Thrift-generated stubs for RPC handling
#include "gen-cpp/CommandCenter.h"
#include "gen-cpp/commandcenter_types.h"
#include "gen-cpp/QAService.h"
#include "gen-cpp/KaldiService.h"
#include "gen-cpp/ImageMatchingService.h"

// Boost libraries
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
// #include <boost/date_time.hpp>

// Extras
#include "base64.h"

// Threading
#include <pthread.h>

// define the number of threads in pool
#define THREAD_WORKS 16


using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace cmdcenterstubs;
using namespace qastubs;

class BadImgFileException {};

class AssignmentFailedException{
public:
	AssignmentFailedException(std::string msg) {
		err = msg;
	}
	string err;
};

class ServiceData
{
public:
	ServiceData(std::string hostname, int port) {
	  	socket = boost::shared_ptr<TTransport>(new TSocket(hostname, port));
	  	transport = boost::shared_ptr<TTransport>(new TBufferedTransport(socket));
	  	protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
	}
	ServiceData(ServiceData *sd) {
	  	socket = sd->socket;
	  	transport = sd->transport;
	  	protocol = sd->protocol;
	}
	boost::shared_ptr<TTransport> socket;
	boost::shared_ptr<TTransport> transport;
	boost::shared_ptr<TProtocol> protocol;
};

// NOTE: why am I not using get() and set()? ~Ben
// set() allows you to encapsulate assignment validity checks
// within the class. However, it's difficult to determine whether
// audio/img data is well-formed from within the command center.
class AsrServiceData : public ServiceData
{
public:
	AsrServiceData(ServiceData *sd)
	: ServiceData(sd), client(protocol), audio("") {}
	KaldiServiceClient client;
	std::string audio;
};

class QaServiceData : public ServiceData
{
public:
	QaServiceData(ServiceData *sd)
	: ServiceData(sd), client(protocol) {}
	QAServiceClient client;
};

class ImmServiceData : public ServiceData
{
public:
	ImmServiceData(ServiceData *sd)
	: ServiceData(sd), client(protocol), img("") {}
	ImageMatchingServiceClient client;
	std::string img;
};

#endif
