package lucida.main;

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

// The service handler
import lucida.handler.*;
import lucida.thrift.*;

/**
 * Starts the question-answer server and listens for requests.
 */
public class QADaemon {
	private static void connectToCMD() {
		// User.
		String LUCID = "Falk";
		QuerySpec spec = new QuerySpec();
		// Knowledge.
		final QueryInput knowledge_text = new QueryInput("text", new ArrayList<String>() {{
		    add("YodaQA is being developed by Fauk.");
		}});
		QuerySpec knowledge = new QuerySpec(new ArrayList<QueryInput>() {{
		    add(knowledge_text);
		}});
		// Query.
		final QueryInput query_input = new QueryInput("query", new ArrayList<String>() {{
		    add("What is Falk developing?");
		}});
		QuerySpec query = new QuerySpec(new ArrayList<QueryInput>() {{
		    add(query_input);
		}});
		// Initialize thrift objects.
		TTransport transport = new TSocket("localhost", 8080);
		TProtocol protocol = new TBinaryProtocol(new TFramedTransport(transport));
		LucidaService.Client client = new LucidaService.Client(protocol);
		try {
			// Talk to the server.
			transport.open();
			System.out.println("///// Connecting to CMD at port " + 8080 + " ... /////");
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
	
	public static void main(String [] args) 
			throws TTransportException, IOException, InterruptedException {	
		connectToCMD();
		
		TProcessor proc = new LucidaService.AsyncProcessor(
				new QAServiceHandler.AsyncQAServiceHandler());
		TNonblockingServerTransport transport = new TNonblockingServerSocket(8083);
		TNonblockingServer.Args arguments = new TNonblockingServer.Args(transport)
		.processor(proc)	
		.protocolFactory(new TBinaryProtocol.Factory())
		.transportFactory(new TFramedTransport.Factory());
		final TNonblockingServer server = new TNonblockingServer(arguments);
		System.out.println("Start listening to requests at port 8083 ...");
		server.serve();
	}
}
