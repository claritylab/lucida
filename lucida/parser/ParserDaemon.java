// Thrift java libraries 
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TNonblockingServerTransport;

// Thrift client-side code
import org.apache.thrift.protocol.TBinaryProtocol;

// Generated code
import parserstubs.ParserService;

/**
 * Starts the parser server and listens for requests.
 */
public class ParserDaemon {
	/** 
	 * An object whose methods are implementations of the parser thrift
	 * interface.
	 */
	public static ParserServiceHandler handler;

	/**
	 * An object responsible for communication between the handler
	 * and the server. It decodes serialized data using the input protocol,
	 * delegates processing to the handler, and writes the response
	 * using the output protocol.
	 */
	public static ParserService.Processor<ParserServiceHandler> processor;

	/** 
	 * Entry point for parser.
	 * @param args the argument list. Provide port number.
	 */
	public static void main(String [] args) {
		try {
			// collect the port number
			int tmp_port = 8080;
			if (args.length == 1) {
				tmp_port = Integer.parseInt(args[0].trim());
			} else {
				System.out.println("Using default port for parser: "
						+ tmp_port);
			}

			// create a new thread to handle the requests
			final int port = tmp_port;
			handler = new ParserServiceHandler();
			processor = new ParserService.Processor<ParserServiceHandler>(handler);
			Runnable simple = new Runnable() {
				public void run() {
					simple(processor, port);
				}
			};      
			new Thread(simple).start();
		} catch (Exception e) {
			e.printStackTrace();
		}   
	}

	/**
	 * Listens for requests and forwards request information
	 * to handler.
	 * @param processor the thrift processor that will handle serialization
	 * and communication with the handler once a request is received.
	 * @param port the port at which the parser service will listen.
	 */
	public static void simple(ParserService.Processor<ParserServiceHandler> processor, final int port) {
		try {
			// create a multi-threaded server: TNonblockingServer
			TNonblockingServerTransport transport = new TNonblockingServerSocket(port);
			TNonblockingServer.Args args = new TNonblockingServer.Args(transport)
			.processor(processor)	
			.protocolFactory(new TBinaryProtocol.Factory())
			.transportFactory(new TFramedTransport.Factory());
			TNonblockingServer server = new TNonblockingServer(args);
			System.out.println("Starting parser at port " + port + "...");
			server.serve();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
