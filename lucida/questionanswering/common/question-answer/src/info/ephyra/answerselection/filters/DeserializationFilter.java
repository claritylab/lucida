package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.search.Result;

import java.io.EOFException;
import java.io.File;
import java.io.FileInputStream;
import java.io.ObjectInputStream;
import java.util.ArrayList;

/**
 * <p>The <code>DeserializationFilter</code> reads serialized results from one
 * or more files.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-24
 */
public class DeserializationFilter extends Filter {
	/** Input file with serialized results. */
	private static File[] serialFiles;
	
	// Getters/Setters
	public static File[] getSerialFiles() {
		return serialFiles;
	}
	public static void setSerialFiles(File[] serialFiles) {
		DeserializationFilter.serialFiles = serialFiles;
	}
	
	/**
	 * Adds an input file with serialized results.
	 * 
	 * @param serialFile input file with serialized results
	 */
	public static void addSerialFile(File serialFile) {
		File[] newFiles;
		if (serialFiles != null) {
			newFiles = new File[serialFiles.length + 1];
			for (int i = 0; i < serialFiles.length; i++)
				newFiles[i] = serialFiles[i];
		} else {
			newFiles = new File[1];
		}
		newFiles[newFiles.length - 1] = serialFile;
		
		serialFiles = newFiles;
	}
	
	/**
	 * Clears the input files with serialized results.
	 */
	public static void clearSerialFiles() {
		serialFiles = null;
	}
	
	/**
	 * Filters an array of <code>Result</code> objects.
	 * 
	 * @param results results to filter
	 * @return filtered results
	 */
	public Result[] apply(Result[] results) {
		// any input file set?
		if (serialFiles == null || serialFiles.length == 0) return results;
		
		// keep old results
		ArrayList<Result> resultsL = new ArrayList<Result>();
		for (Result result : results) resultsL.add(result);
		
		// deserialize and add results
		for (File serialFile : serialFiles) {
			// input file exists?
			if (!serialFile.exists()) continue;
			
			try {
				FileInputStream fis = new FileInputStream(serialFile);
				ObjectInputStream ois = new ObjectInputStream(fis);
				
				try {
					while (true) {
						Object o = ois.readObject();
						if (o instanceof Result) {
							Result result = (Result) o;
							resultsL.add(result);
						}
					}
				} catch (EOFException e) {/* end of file reached */}
				
				ois.close();
			} catch (Exception e) {
				MsgPrinter.printErrorMsg("Could not read serialized results:");
				MsgPrinter.printErrorMsg(e.toString());
				System.exit(1);
			}
		}
		
		return resultsL.toArray(new Result[resultsL.size()]);
	}
}
