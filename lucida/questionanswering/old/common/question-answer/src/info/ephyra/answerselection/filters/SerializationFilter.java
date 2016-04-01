package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.search.Result;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

/**
 * <p>The <code>SerializationFilter</code> serializes the results and writes
 * them to a file.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-25
 */
public class SerializationFilter extends Filter {
	/** Output file for serialized results. */
	private static File serialFile;
	
	// Getters/Setters
	public static File getSerialFile() {
		return serialFile;
	}
	public static void setSerialFile(File serialFile) {
		SerializationFilter.serialFile = serialFile;
	}
	
	/**
	 * Filters an array of <code>Result</code> objects.
	 * 
	 * @param results results to filter
	 * @return filtered results
	 */
	public Result[] apply(Result[] results) {
		// output file set?
		if (serialFile == null) return results;
		
		// modify file name if file already exists
		// (comment this out to replace existing files)
		String path = serialFile.getPath();
		File serialFile = new File(path);
		if (serialFile.exists()) {
			path = serialFile.getPath() + "_2";
			serialFile = new File(path);
			int i = 2;
			while (serialFile.exists()) {
				path = serialFile.getPath();
				path = path.replaceFirst("_" + i + "$", "_" + ++i);
				serialFile = new File(path);
			}
		}
		
		// serialize results
		try {
			FileOutputStream fos = new FileOutputStream(serialFile);
			ObjectOutputStream oos = new ObjectOutputStream(fos);
			
			for (Result result : results) oos.writeObject(result);
			
			oos.close();
		} catch (IOException e) {
			MsgPrinter.printErrorMsg("Could not write serialized results:");
			MsgPrinter.printErrorMsg(e.toString());
			System.exit(1);
		}
		
		return results;
	}
}
