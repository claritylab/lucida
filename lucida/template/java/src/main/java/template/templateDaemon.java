package template;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.ArrayList;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import org.apache.thrift.TProcessor;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TThreadedSelectorServer;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TNonblockingServerTransport;
import org.apache.thrift.transport.TTransportException;
import org.apache.thrift.async.AsyncMethodCallback;

import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;

// Mongodb java libraries
import com.mongodb.Cursor;
import com.mongodb.DB;
import com.mongodb.DBCollection;
import com.mongodb.DBCursor;
import com.mongodb.DBObject;
import com.mongodb.MongoClient;
import com.mongodb.BasicDBObject;

import thrift.*;

/**
 * Starts the template server and listens for requests.
 */
public class templateDaemon {
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
		// TODO: change your service
		BasicDBObject query = new BasicDBObject("name", "template");
		DBCursor cursor = coll.find(query);
		String port_str = cursor.next().get("port").toString();
		mongoClient.close();
		Integer port = Integer.valueOf(port_str);

		TProcessor proc = new LucidaService.AsyncProcessor(
				new TPLServiceHandler.AsyncTPLServiceHandler());
		TNonblockingServerTransport transport = new TNonblockingServerSocket(port);
		TThreadedSelectorServer.Args arguments = new TThreadedSelectorServer.Args(transport)
		.processor(proc)	
		.protocolFactory(new TBinaryProtocol.Factory())
		.transportFactory(new TFramedTransport.Factory());
		final TThreadedSelectorServer server = new TThreadedSelectorServer(arguments);
		// TODO: Change XXX into your service's acronym
		System.out.println("TPL at port " + port_str);
		server.serve();
	}
}
