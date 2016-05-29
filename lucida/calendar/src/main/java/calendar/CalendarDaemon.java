package calendar;

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

import thrift.*;

/**
 * Starts the calendar server and listens for requests.
 */
public class CalendarDaemon {
	private static void connectToCMD() {
		QueryInput query_input = new QueryInput();
		query_input.type = "CA";
		query_input.data = new ArrayList<String>();
		query_input.data.add("localhost");
		query_input.tags = new ArrayList<String>();
		query_input.tags.add("8084");
		QuerySpec spec = new QuerySpec();
		spec.content = new ArrayList<QueryInput>();
		spec.content.add(query_input);
		// Initialize thrift objects.
		TTransport transport = new TSocket("localhost", 8080);
		TProtocol protocol = new TBinaryProtocol(new TFramedTransport(transport));
		LucidaService.Client client = new LucidaService.Client(protocol);
		try {
			transport.open();
			System.out.println("Connecting to CMD at port " + 8080);
			// Register itself to CMD.
			client.create("", spec);
			transport.close();
			System.out.println("Successfully connected to CMD");
		} catch (TException x) {
			x.printStackTrace();
		}
	}
	
	public static void main(String [] args) 
			throws TTransportException, IOException, InterruptedException {	
		connectToCMD();
		
		TProcessor proc = new LucidaService.AsyncProcessor(
				new CAServiceHandler.AsyncCAServiceHandler());
		TNonblockingServerTransport transport = new TNonblockingServerSocket(8084);
		TThreadedSelectorServer.Args arguments = new TThreadedSelectorServer.Args(transport)
		.processor(proc)	
		.protocolFactory(new TBinaryProtocol.Factory())
		.transportFactory(new TFramedTransport.Factory());
		final TThreadedSelectorServer server = new TThreadedSelectorServer(arguments);
		System.out.println("CA at port 8084");
		server.serve();
	}
}
