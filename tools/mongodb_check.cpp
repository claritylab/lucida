// Outputs "Connection ok" if MongoDB and C++ driver are installed successfully.
#include <iostream>
#include "client/dbclient.h"
using namespace mongo;


int main() {
    DBClientConnection conn;
    try {
        conn.connect("localhost:27017");
        std::cout << "Connection ok" << std::endl;
    } catch( DBException &e ) {
        std::cout << "Caught " << e.what() << std::endl;
    }
    return 0;
}
