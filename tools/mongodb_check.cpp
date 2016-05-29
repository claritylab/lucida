// Outputs "Connection ok" if MongoDB and C++ driver are installed successfully.
#include <iostream>
#include "client/dbclient.h"
using namespace mongo;


int main() {
    DBClientConnection conn;
    try {
        conn.connect("localhost:27017");
        cout << "Connection ok" << endl;
    } catch( DBException &e ) {
        cout << "Caught " << e.what() << endl;
    }
    return 0;
}
