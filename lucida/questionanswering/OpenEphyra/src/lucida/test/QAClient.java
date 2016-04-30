package lucida.test;

//Java packages
import java.util.List;
import java.util.ArrayList;

//Thrift java libraries 
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;

//Generated code
import lucida.thrift.*;

/** 
* A testing Client that sends a single query to OpenEphyra Server and prints the results.
*/
public class QAClient {
	
	public static void main(String [] args) {
		// Collect the port number.
		int port = 8083;

		if (args.length >= 1) {
			port = Integer.parseInt(args[0]);
		}
		
		// User.
		String LUCID = "qll";
		QuerySpec spec = new QuerySpec();
		
		// Knowledge.
		final QueryInput knowledge_text = new QueryInput("text", new ArrayList<String>() {{
		    add("Today China’s population is over 1.4 billion,"
		    		+ " the largest of any country in the world.");
		    add("The capital of Italy is Rome.");
		}});
		final QueryInput knowledge_url = new QueryInput("URL", new ArrayList<String>() {{
		    add("https://en.wikipedia.org/wiki/Cookie");
		}});
		QuerySpec knowledge = new QuerySpec(new ArrayList<QueryInput>() {{
		    add(knowledge_text);
		    add(knowledge_url);
		}});
		
		// Query.
		final QueryInput query_input = new QueryInput("query", new ArrayList<String>() {{
		    add("What is the Chinese population?");
		}});
		QuerySpec query = new QuerySpec(new ArrayList<QueryInput>() {{
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
			// Call the three functions.
			client.create(LUCID, spec);
			client.learn(LUCID, knowledge);
			System.out.println("///// Query input: /////");
			System.out.println(query_input);
			String answer = client.infer(LUCID, query);
			// Print the answer.
			System.out.println("///// Answer: /////");
			System.out.println(answer);
			transport.close();
		} catch (TException x) {
			x.printStackTrace();
		}
	}
}
