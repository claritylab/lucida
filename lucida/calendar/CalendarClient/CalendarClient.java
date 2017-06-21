//Java packages
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Properties;

//Thrift java libraries 
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

//Generated code
import thrift.*;

/** 
* A Calendar Client that get the upcoming events from Calendar Server and prints the results.
*/
public class CalendarClient {
	public static void main(String [] args) 
			throws IOException {
		// Get the port ID from Mongodb
		String mongo_addr = "localhost";
		if (System.getenv("MONGO_PORT_27017_TCP_ADDR") != null) {
			mongo_addr = System.getenv("MONGO_PORT_27017_TCP_ADDR");
		}
		MongoClient mongoClient = new MongoClient(mongo_addr, 27017);
		DB db = mongoClient.getDB("lucida");
		DBCollection coll = db.getCollection("service_info");
		BasicDBObject query = new BasicDBObject("name", "calendar");
		DBCursor cursor = coll.find(query);
		String port_str = cursor.next().get("port").toString();
		mongoClient.close();
		Integer port = Integer.valueOf(port_str);

		// Query.
		String LUCID = "Clinc";
		String query_input_data = "What is on my Google calendar for last week?";
		QueryInput query_input = new QueryInput();
		query_input.type = "query";
		query_input.data = new ArrayList<String>();
		query_input.data.add(query_input_data);
		QuerySpec query_spec = new QuerySpec();
		query_spec.content = new ArrayList<QueryInput>();
		query_spec.content.add(query_input);
	 
		// Initialize thrift objects.
		// TTransport transport = new TSocket("clarity08.eecs.umich.edu", port);
		TTransport transport = new TSocket("localhost", port);
		TProtocol protocol = new TBinaryProtocol(new TFramedTransport(transport));
		LucidaService.Client client = new LucidaService.Client(protocol);
		try {
			// Talk to the Calendar server.
			transport.open();
			System.out.println(query_input_data);
			System.out.println("///// Connecting to Calendar... /////");
			String results = client.infer(LUCID, query_spec);
			System.out.println("///// Result: /////");
			System.out.println(results);
			transport.close();
		} catch (TException e) {
			e.printStackTrace();
		}
		
		return;
	}
}
