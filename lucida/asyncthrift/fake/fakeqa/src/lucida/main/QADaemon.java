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
		String LUCID = "QA";
		QuerySpec spec = new QuerySpec();
		spec.name = "" + 8083;
		// Initialize thrift objects.
		TTransport transport = new TSocket("localhost", 8080);
		TProtocol protocol = new TBinaryProtocol(new TFramedTransport(transport));
		LucidaService.Client client = new LucidaService.Client(protocol);
		try {
			transport.open();
			System.out.println("Connecting to CMD at port " + 8080);
			// Register itself to CMD.
			client.create(LUCID, spec);
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
				new QAServiceHandler.AsyncQAServiceHandler());
		TNonblockingServerTransport transport = new TNonblockingServerSocket(8083);
		TNonblockingServer.Args arguments = new TNonblockingServer.Args(transport)
		.processor(proc)	
		.protocolFactory(new TBinaryProtocol.Factory())
		.transportFactory(new TFramedTransport.Factory());
		final TNonblockingServer server = new TNonblockingServer(arguments);
		System.out.println("QA at port 8083");
		server.serve();
	}
}
