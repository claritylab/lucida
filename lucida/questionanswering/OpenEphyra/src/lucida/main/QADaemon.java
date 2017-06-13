package lucida.main;

// Thrift java libraries 
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TNonblockingServerTransport;
import org.apache.thrift.server.TThreadedSelectorServer;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TTransportException;

import java.io.IOException;
import java.util.ArrayList;

// Mongodb java libraries
import com.mongodb.Cursor;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBCursor;
import com.mongodb.DBObject;
import com.mongodb.MongoClient;
import com.mongodb.BasicDBObject;

import org.apache.thrift.TException;
import org.apache.thrift.TProcessor;
// Thrift client-side code for registering w/ sirius
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;

// Generated code
import lucida.thrift.*;

// The service handler
import lucida.handler.*;
import lucida.handler.QAServiceHandler.AsyncQAServiceHandler;

/**
 * Starts the question-answer server and listens for requests.
 */
public class QADaemon {
	/** 
	 * Entry point for question-answer.
	 * @param args the argument list. Provide port numbers
	 * for both sirius and qa.
	 */
	public static void main(String [] args) 
			throws TTransportException, IOException, InterruptedException {	
		// Get the port ID from Mongodb
		String mongo_addr = "localhost";
		if (System.getenv("MONGO_PORT_27017_TCP_ADDR") != null) {
			mongo_addr = System.getenv("MONGO_PORT_27017_TCP_ADDR");
		}
		MongoClient mongoClient = new MongoClient(mongo_addr, 27017);
		DB db = mongoClient.getDB("lucida");
		DBCollection coll = db.getCollection("service_info");
		BasicDBObject query = new BasicDBObject("name", "questionanswering");
		DBCursor cursor = coll.find(query);
		String port_str = cursor.next().get("port").toString();
		mongoClient.close();
		Integer port = Integer.valueOf(port_str);
		
		TProcessor proc = new LucidaService.AsyncProcessor(
				new QAServiceHandler.AsyncQAServiceHandler());
		TNonblockingServerTransport transport = new TNonblockingServerSocket(port);
		TThreadedSelectorServer.Args arguments = new TThreadedSelectorServer.Args(transport)
		.processor(proc)	
		.protocolFactory(new TBinaryProtocol.Factory())
		.transportFactory(new TFramedTransport.Factory());
		final TThreadedSelectorServer server = new TThreadedSelectorServer(arguments);
		System.out.println("QA at port " + port);
		server.serve();
	}
}
