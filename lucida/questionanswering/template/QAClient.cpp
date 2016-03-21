// Program for testing the stand-alone OpenEphyra QA service
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdlib>
#include <getopt.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "gen-cpp/QAService.h"
#define NUM_ARGS 3

// NOTE: I'm assuming that std is unlikely to
// create global namespace conflicts with apache thrift
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

void printHelpMsg();

// Sends a factoid question to the question-answer server.
// The question-answer server should return only one answer
// to a given factoid question.
//
// Params:
//    client -- a QAServiceClient object who sends requests
//        in serialized form to the question-answer server.
//    question -- eg.) What is the speed of light?
void clientAskFactoid(qastubs::QAServiceClient& client, string question);

// Sends a list question to the question-answer server.
// The question-answer server should return at least one answer
// to a given list question.
//
// Params: (same as above)
// Note: For a question like "What is the speed of light?"
// multiple answers may be returned.
void clientAskList(qastubs::QAServiceClient& client, string question);

// Runs two unit tests for the stand-alone question-answer service.
// Users must supply the arguments in the form ./qaclient (QUESTION) (PORT)
int main(int argc, char** argv) {
  // Option parsing for client
  bool factoid = false;
  bool list = false;
  int c = -1;
  while (1) {
    // These options set no flag. When getopt_long() finds one of these
    // options, it returns the fourth field: 'h', 'f', or 'l'.
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"factoid", no_argument, 0, 'f'},
      {"list", no_argument, 0, 'l'},
      {0, 0, 0, 0}
    };

    int option_index = 0;
    c = getopt_long(argc, argv, "hfl", long_options, &option_index);

    // getopt_long() returns -1 when no options are found
    if (c == -1) {
      break;
    }

    switch (c) {
      case 'h':
        printHelpMsg();
        return 0;
      case 'f':
        cout << "Selected factoid test" << endl;
        factoid = true;
        break;
      case 'l':
        cout << "Selected list test" << endl;
        list = true;
        break;
      default:
        abort();
    }
  }

  // If user didn't specify either factoid or list,
  // then exit.
  if (!factoid && !list) {
    cout << "Please specify the question type" << endl;
    printHelpMsg();
    exit(1);
  }

  int port = 9091;
  string question = "";
  if ((optind + 1) < argc) {
    question = argv[optind];
    port = atoi(argv[optind + 1]);
    cout << "question = " << question << endl;
    cout << "port = " << port << endl; 
  } else {
    cout << "You didn't give me a question or a port" << endl;
    printHelpMsg();
    exit(1);
  } // End option parsing

  boost::shared_ptr<TTransport> socket(new TSocket("localhost", port));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  qastubs::QAServiceClient client(protocol);

  try {
    // Run unit tests
    transport->open();
    if (factoid) {
      clientAskFactoid(client, question);
    }
    if (list) {
      clientAskList(client, question);
    }
    transport->close();
    } catch (TException &tx) {
      cout << "ERROR: " << tx.what() << endl;
  }

  return 0;
}

void printHelpMsg() {
  cout << "Usage: ./qaclient [--list | --factoid] <Question> <Port>" << endl;
}

void clientAskFactoid(qastubs::QAServiceClient& client, string question) {
  struct timeval tv1, tv2;
  string answer;

  // ask factoid question
  cout << "calling askFactoidThrift():" << endl;
  gettimeofday(&tv1, NULL);
  client.askFactoidThrift(answer, question); // pass QAService a question
  gettimeofday(&tv2, NULL);
  unsigned int query_latency = (tv2.tv_sec - tv1.tv_sec)
      * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  cout << "client sent the question successfully..." << endl;
  cout << "ANSWER = " << answer << endl;
  cout << "server replied within " << fixed << setprecision(2)
      << (double)query_latency / 1000 << " ms" << endl;
  cout << endl;
}

void clientAskList(qastubs::QAServiceClient& client, string question) {
  struct timeval tv1, tv2;
  vector<string> answers;

  cout << "calling askListThrift():" << endl;
  gettimeofday(&tv1, NULL);
  client.askListThrift(answers, question); // pass QAService a question
  gettimeofday(&tv2, NULL);
  unsigned int query_latency = (tv2.tv_sec - tv1.tv_sec)
      * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  cout << "client sent the question successfully..." << endl;
  
  cout << "printing answers:" << endl;
  vector<string>::iterator it;
  for (it = answers.begin(); it != answers.end(); ++it) {
    cout << "\t" << *it << endl;
  }

  cout << "server replied within " << fixed << setprecision(2)
      << (double)query_latency / 1000 << " ms" << endl;
  cout << endl;
}
