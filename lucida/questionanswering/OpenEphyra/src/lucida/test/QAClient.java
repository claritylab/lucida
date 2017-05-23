package lucida.test;

//Java packages
import java.io.IOException;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Properties;
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
		// Collect the port number.
		Properties port_cfg = new Properties();
		InputStream input = new FileInputStream("../../config.properties");
		port_cfg.load(input);
		String port_str = port_cfg.getProperty("QA_PORT");
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
