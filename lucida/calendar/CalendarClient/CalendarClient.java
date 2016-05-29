//Java packages
import java.util.ArrayList;

//Thrift java libraries 
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;

//Generated code
import thrift.*;

/** 
* A Calendar Client that get the upcoming events from Calendar Server and prints the results.
*/
public class CalendarClient {
	public static void main(String [] args) {
		// Collect the port number.
		int port = 8084;

		if (args.length == 1) {
			port = Integer.parseInt(args[0]);
		} else {
			System.out.println("Using default port for Calendar Client: " + port);
		}

		// Query.
		String LUCID = "QLL";
		String query_input_data = "What is on my Google calendar from next Sunday morning to next Sunday noon to next Sunday night?";
		final QueryInput query_input = new QueryInput("query", new ArrayList<String>() {{
		    add(query_input_data);
		}});
		QuerySpec query_spec = new QuerySpec(new ArrayList<QueryInput>() {{
		    add(query_input);
		}});
	 
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
			System.out.println("///// Results: /////");
			System.out.println(results);
			transport.close();
		} catch (TException e) {
			e.printStackTrace();
		}
		
		return;
	}
}
