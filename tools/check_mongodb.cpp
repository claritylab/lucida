// g++ check_mongodb.cpp -std=c++11 -lmongoclient -lboost_thread -lboost_filesystem -lboost_regex -lboost_program_options -lboost_system -pthread -lssl -lcrypto -o check_mongodb

#include <iostream>
#include <cstdlib>
#include "mongo/client/dbclient.h"

using namespace mongo;
using namespace std;

int main() {
	if (const char* env_p = std::getenv("LUCIDAROOT")) {
		std::cout << "LUCIDAROOT is: " << env_p << '\n';
	}
	DBClientConnection conn;
	try {
		conn.connect("localhost:27017");
		cout << "Connection ok" << endl;
	} catch( DBException &e ) {
		cout << "Caught " << e.what() << endl;
	}
	string mongodbCollection = "lucida.images_Johann";  
	auto_ptr<mongo::DBClientCursor> cursor = conn.query( mongodbCollection , mongo::BSONObj() );  
	int count = 0;  
	while (cursor->more()) {
		cout << "@@@" << endl;
		BSONObj p = cursor->next();
		cout << p.getStringField("label") << endl;
		++count;
	}
	cout << "### Numer of documents in lucida.images_Johann: " << count << endl;
	return 0;
}

