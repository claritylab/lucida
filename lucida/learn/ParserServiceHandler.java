// This handler implements the services provided to the client.

// Java packages
import java.net.URL;
import java.util.List;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;

// Jsoup and Indri
import org.jsoup.Jsoup;
import lemurproject.indri.IndexEnvironment;

// Interface definition
import parserstubs.ParserService;

/** Implementation of the parser interface defined
 * in the parser thrift file. A client request to any
 * method defined in the thrift file is handled by the
 * corresponding method here.
 */
public class ParserServiceHandler implements ParserService.Iface {
  /** Parses the client's urls and files to an Indri repository
   * and returns true if the repository is created.
   * @param Indri_path eg. "path/to/Indri/repository"
   * @param urls eg. "[https://en.wikipedia.org/wiki/Apple, http://clarity-lab.org/]"
   * @param files eg. "[path/to/file1.txt, path/to/file2.pdf]"
   */
	public boolean parseThrift(String Indri_path, List<String> urls, List<String> files) {
		// parse urls
		// make the temp directory
		File file = new File("temp/");
		file.mkdirs();
		System.out.println("Parsing urls...");
		for (int i = 0; i < urls.size(); ++i) {
			String parsed_text = "";
			String temp_file_path = "temp/" + i + ".txt";
			try {
				// set the time limit (connection and read timeout, in milliseconds) to
				// the maximum value an int can have, 2^31-1.
				parsed_text = Jsoup.parse(new URL(urls.get(i)), Integer.MAX_VALUE).text();
				
				// write the parsed text to a temporary file
				FileWriter fileWriter = new FileWriter(temp_file_path);
				// wrap FileWriter in BufferedWriter
				BufferedWriter bufferedWriter = new BufferedWriter(fileWriter);
				bufferedWriter.write(parsed_text);
				// close files
				bufferedWriter.close();   
			} catch (Exception e) {
				e.printStackTrace();
				return false;
			}
		}
		
		// let Indri parse the web page files in temp and the user input files
		// make the Indri directory
		File indri_dir = new File(Indri_path);
		indri_dir.mkdirs();
		System.out.println("Parsing files...");
		IndexEnvironment env = new IndexEnvironment();
		try {
			env.setStemmer("krovetz");
			env.create(Indri_path);
			// parse the web page files in temp 
			for (int i = 0; i < urls.size(); ++i) {
				String temp_file_path = "temp/" + i + ".txt";
				env.addFile(temp_file_path);
			}
			// parse the user input files
			for (int i = 0; i < files.size(); ++i) {
				env.addFile(files.get(i));
			}
			env.close();
			// print the number of documents that has been indexed
			System.out.println("documentsIndexed: " + env.documentsIndexed());
		} catch (Exception e) {
			e.printStackTrace();
			return false;
		}
		
    return true;
  }
  
  public void ping() {
    System.out.println("pinged");
  }
}

