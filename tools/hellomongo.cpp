// // //  g++ hellomongo.cpp -I /usr/local/include/mongo /usr/local/lib/libmongoclient.a  -lboost_thread -lboost_filesystem -lboost_program_options -lboost_system -pthread -o hellomongo


// #include <iostream>
// #include "client/dbclient.h"

// using namespace mongo;

// int main() {


//     DBClientConnection conn;
//     //BSONObj p = BSONObjBuilder().append("name", "Joe").append("age", 33).obj();

//     try {
//         conn.connect("localhost:27017");

//         cout << "connected ok" << endl;
//     } catch( DBException &e ) {
//         cout << "caught " << e.what() << endl;
//     }

//     //conn.insert("lucida.images_Johann", p);

//     string mongodbCollection = "lucida.images_Johann";  
//     auto_ptr<mongo::DBClientCursor> cursor = conn.query( mongodbCollection , mongo::BSONObj() );  
//     int count = 0;  
//     while (cursor->more()) {
//         cout << "@@@" << endl;
//         BSONObj p = cursor->next();
//         cout << p.getStringField("label") << endl;
//         cout << p.getStringField("data").size() << endl;

//         ++count;
//     }
//     cout << "###" << count << endl;


//     return 0;
// }


// g++ --std=c++11 hellomongo.cpp -o hellomongo $(pkg-config --cflags --libs libmongocxx)

#include <iostream>

#include <string>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

using namespace std;

int main(int, char**) {
    mongocxx::instance inst{};
    mongocxx::client conn{mongocxx::uri{}};

    bsoncxx::builder::stream::document document{};

    auto collection = conn["lucida"]["images_Johann"];

    document << "hello" << "world";


    collection.insert_one(document.view());
    auto cursor = collection.find({});

    for (auto&& doc : cursor) {
        std::cout << bsoncxx::to_json(doc) << std::endl;
        // auto x = doc.find("hello")->get_value();
        // std::cout << x << std::endl;
    }


    //conn["lucida"].drop();
}