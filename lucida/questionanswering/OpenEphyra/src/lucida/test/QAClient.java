package lucida.test;

//Java packages
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;

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
import lucida.thrift.*;

/** 
 * A testing Client that sends a single query to OpenEphyra Server and prints the results.
 */
public class QAClient {
	/**
	 * Creates a QueryInput.
	 */
	private static final QueryInput createQueryInput(
			final String type,
			final String data,
			final String tag) {
		final QueryInput rtn = new QueryInput();
		rtn.type = type;
		rtn.data =  new ArrayList<String>() {{
			add(data);
		}};
		rtn.tags =  new ArrayList<String>() {{
			add(tag);
		}};
		return rtn;
	}

	/**
	 * Creates a QuerySpec.
	 */
	private static final QuerySpec createQuerySpec(
			String name,
			List<QueryInput> query_input_list) {
		final QuerySpec rtn = new QuerySpec();
		rtn.name = name;
		rtn.content = query_input_list;
		return rtn;
	}

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
		BasicDBObject query_ = new BasicDBObject("name", "questionanswering");
		DBCursor cursor = coll.find(query_);
		String port_str = cursor.next().get("port").toString();
		mongoClient.close();
		final Integer port = Integer.valueOf(port_str);

		// User.
		String LUCID = "Clinc";
		QuerySpec create_spec = new QuerySpec();

		// Knowledge.
		final QueryInput knowledge_text = createQueryInput(
				"text",
				"Clinc is created by Jason and Lingjia.",
				"1234567");
		final QueryInput knowledge_url = createQueryInput(
				"url",
				"https://en.wikipedia.org/wiki/Apple_Inc.",
				"abcdefg");	
		final QuerySpec knowledge = createQuerySpec(
				"knowledge",
				new ArrayList<QueryInput>() {{
					add(knowledge_text);
					add(knowledge_url);
				}});

		// Unlearn.
		final QueryInput knowledge_unlearn_input = createQueryInput(
				"unlearn",
				"",
				"abcdefg");
		final QuerySpec knowledge_unlearn_spec = createQuerySpec(
				"unlearn knowledge",
				new ArrayList<QueryInput>() {{
					add(knowledge_unlearn_input);
				}});

		// Query.
		final QueryInput query_input = new QueryInput();
		query_input.type = "text";
		query_input.data = new ArrayList<String>() {{
			add("Who created Clinc?");
		}};
		query_input.tags = new ArrayList<String>() {{
			add("localhost");
			add(Integer.toString(port));
			add("0");
		}};
		final QuerySpec query = createQuerySpec(
				"query",
				new ArrayList<QueryInput>() {{
					add(query_input);
				}});
		
		// Initialize thrift objects.
		// TTransport transport = new TSocket("clarity08.eecs.umich.edu", port);
		TTransport transport = new TSocket("localhost", port);
		TProtocol protocol = new TBinaryProtocol(new TFramedTransport(transport));
		LucidaService.Client client = new LucidaService.Client(protocol);
		try {
			// Talk to the server.
			transport.open();
			System.out.println("///// Connecting to OpenEphyra at port " + port + " ... /////");
			// Learn and ask.
			client.create(LUCID, create_spec);
			client.learn(LUCID, knowledge);
			System.out.println("///// Query input: /////");
			System.out.println(query_input.data.get(0));
			String answer = client.infer(LUCID, query);
			// Print the answer.
			System.out.println("///// Answer: /////");
			System.out.println(answer);
			// Unlearn and ask again.
			client.learn(LUCID, knowledge_unlearn_spec);
			System.out.println("///// Query input: /////");
			System.out.println(query_input.data.get(0));
			answer = client.infer(LUCID, query);
			// Print the answer.
			System.out.println("///// Answer: /////");
			System.out.println(answer);	
			transport.close();
		} catch (TException x) {
			x.printStackTrace();
		}
	}
}
