//Java packages
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
import parserstubs.ParserService;

public class ParserClient {
	private static final int MIN_ARGS_COUNT = 4;
	
  public static void main(String [] args) {
  	// collect the port number
  	int port = -1;
  	if (args.length >= 1) {
  		try {
  			port = Integer.parseInt(args[0]);
  		} catch (NumberFormatException e) {
  			// args[0] does not contain a parsable integer
  			printHelpMsg();
  			e.printStackTrace();
  			return;
  		}
  		System.out.println("Port = " + port);
  	} else {
  		// no port number specified
  		System.out.println("Error: Please specify a port number.");
  		printHelpMsg();
  		return;
  	}
  	
  	// collect the path to the Indri repository
  	String Indri_path = "";
  	if (args.length >= 2) {
  		Indri_path = args[1];
  		System.out.println("Your Indri repository will be stored at: " + Indri_path);
  	} else {
  		// no Indri path specified
  		System.out.println("Error: Please specify the path to the Indri repository.");
  		printHelpMsg();
  		return;
  	}
  	
  	// collect the number of urls and files
  	int num_urls, num_files = -1;
  	if (args.length >= 4) {
  		try {
  			num_urls = Integer.parseInt(args[2]);
  			num_files = Integer.parseInt(args[3]);
  		} catch (NumberFormatException e) {
  		// args[2] or args[3] does not contain a parsable integer
  			printHelpMsg();
  			e.printStackTrace();
  			return;
  		}
  		if (num_urls < 0 || num_files < 0) {
  			// check if the input is negative
  			System.out.println("Error: Please specify a valid number of urls and files.");
  			return;
  		}
  		System.out.println("Number of urls = " + num_urls);
  		System.out.println("Number of files = " + num_files);
  	} else {
  		// no number of urls or number of files specified
  		System.out.println("Error: Please specify both the number of urls and the number of files.");
  		printHelpMsg();
  		return;
  	}
  	
  	// collect the urls and file paths
  	List<String> urls = new ArrayList<String>();
  	List<String> files = new ArrayList<String>();
  	if (args.length == MIN_ARGS_COUNT + num_urls + num_files) {
  		// add urls
  		for (int i = MIN_ARGS_COUNT; i < MIN_ARGS_COUNT + num_urls; ++i) {
  			urls.add(args[i]);
  		}
  		// add files
  		for (int i = MIN_ARGS_COUNT + num_urls; i < MIN_ARGS_COUNT + num_urls + num_files; ++i) {
  			files.add(args[i]);
  		}
  	} else {
  		// number of urls plus files does not match the specified number
  		System.out.println("Error: The number of urls and files does not match the input.");
  		return;
  	}
   
  	// parse the urls and files specified by the user 
    // initialize thrift objects
    TTransport transport = new TSocket("localhost", port);
    TProtocol protocol = new TBinaryProtocol(new TFramedTransport(transport));
    ParserService.Client client = new ParserService.Client(protocol);
    try {
    	// talk to the parser
      transport.open();
      System.out.println("Connecting to the parser...");
      boolean is_successful = client.parseThrift(Indri_path, urls, files);
      // check if there is any error during the parsing stage
      if (is_successful) {
        System.out.println("Your Indri repository has been stored at: " + Indri_path);
      } else {
      	System.out.println("Failed. You may check the error message at the server terminal.");
      }
      transport.close();
    } catch (TException x) {
      x.printStackTrace();
    }
    
    return;
  }
  
	public static void printHelpMsg() {
		System.out.println("Usage: java ParserClient (PORT) (pathToIndriRepository) (numberOfUrls) (numberOfFiles) (listOfUrls) (listOfFilePaths)");
		System.out.println("The lists of urls and files must be space-separated, and must be valid urls and paths to files.");
	}
}
