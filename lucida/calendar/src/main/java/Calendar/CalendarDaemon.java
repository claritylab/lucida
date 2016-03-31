package Calendar;

// Thrift java libraries 
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TNonblockingServerTransport;

// Thrift client-side code
import org.apache.thrift.protocol.TBinaryProtocol;

// Generated code
import thrift.*;

/**
* Starts the Calendar server and listens for requests.
*/
public class CalendarDaemon {
	/** 
	* An object whose methods are implementations of the Calendar thrift
	* interface.
	*/
	public static CalendarServiceHandler handler;

	/**
	* An object responsible for communication between the handler
	* and the server. It decodes serialized data using the input protocol,
	* delegates processing to the handler, and writes the response
	* using the output protocol.
	*/
	public static LucidaService.Processor<CalendarServiceHandler> processor;

	/** 
	* Entry point for Calendar.
	* @param args the argument list. Provide port numbers
	* for Calendar.
	*/
	public static void main(String [] args) {
		try {
			int tmp_port = 8081;
			if (args.length == 1) {
				tmp_port = Integer.parseInt(args[0].trim());
			} else {
				System.out.println("Using default port for Calendar: " + tmp_port);
			}

			// Inner classes receive copies of local variables to work with.
			// Local vars must not change to ensure that inner classes are synced.
			final int port = tmp_port;
			// The handler implements the generated java interface
			// that was originally specified in the thrift file.
			// When it's called, an Open Ephyra object is created.
			handler = new CalendarServiceHandler();
			processor = new LucidaService.Processor<CalendarServiceHandler>(handler);
			Runnable simple = new Runnable() {
				// This is the code that the thread will run.
				public void run() {
					simple(processor, port);
				}
			};
			// Let system schedule the thread.
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
	* @param port the port at which the Calendar service will listen.
	*/
	public static void simple(LucidaService.Processor processor, final int port) {
		try {
			// Create a multi-threaded server: TNonblockingServer.
			TNonblockingServerTransport transport = new TNonblockingServerSocket(port);
			TNonblockingServer.Args args = new TNonblockingServer.Args(transport)
			.processor(processor)	
			.protocolFactory(new TBinaryProtocol.Factory())
			.transportFactory(new TFramedTransport.Factory());
			final TNonblockingServer server = new TNonblockingServer(args);

			Thread t1 = new Thread(new Runnable() {
				public void run() {
					System.out.println("Starting Calendar server at port " + port + "...");
					server.serve();
				}
			});

			t1.start();
			t1.join();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
