package lucida.main;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;
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

// The service handler
import lucida.handler.*;
import lucida.thrift.*;

/**
 * Starts the question-answer server and listens for requests.
 */
public class QADaemon {
	public static void main(String [] args) 
			throws TTransportException, IOException, InterruptedException {
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
