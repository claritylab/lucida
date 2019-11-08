//Java packages
import java.io.IOException;
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
* A template Client that get the upcoming events from template Server and prints the results.
*/
public class templateClient {
	public static void main(String [] args) 
			throws IOException{
		if (args.length != 1){
			System.out.println("Wrong arguments!");
			System.exit(1);
		}
		Integer port = Integer.valueOf(args[0]);

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
			// Talk to the template server.
			transport.open();
			System.out.println(query_input_data);
			System.out.println("///// Connecting to template... /////");
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
