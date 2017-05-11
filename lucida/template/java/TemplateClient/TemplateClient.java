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
* A Template Client that get the upcoming events from Template Server and prints the results.
*/
public class TemplateClient {
	public static void main(String [] args) {
		// TODO: Change the port into your client send
		int port = 8888;
		if (args.length == 1) {
			port = Integer.parseInt(args[0]);
		} else {
			System.out.println("Using default port for Template Client: " + port);
		}

		// Query.
		// TODO: Adding your own sample query
		String LUCID = "Clinc";
		String query_input_data = "What is the sample query?";
		QueryInput query_input = new QueryInput();
		query_input.type = "query";
		query_input.data = new ArrayList<String>();
		query_input.data.add(query_input_data);
		QuerySpec query_spec = new QuerySpec();
		query_spec.content = new ArrayList<QueryInput>();
		query_spec.content.add(query_input);
	 
		// Initialize thrift objects.
		TTransport transport = new TSocket("localhost", port);
		TProtocol protocol = new TBinaryProtocol(new TFramedTransport(transport));
		LucidaService.Client client = new LucidaService.Client(protocol);
		try {
			// Talk to the Template server.
			transport.open();
			System.out.println(query_input_data);
			System.out.println("///// Connecting to Your Service... /////");
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
