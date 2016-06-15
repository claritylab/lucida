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
	public static void main(String [] args) 
			throws TTransportException, IOException, InterruptedException {	
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
