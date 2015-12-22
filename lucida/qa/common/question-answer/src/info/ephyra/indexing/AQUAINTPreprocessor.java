package info.ephyra.indexing;

import info.ephyra.io.MsgPrinter;
import info.ephyra.util.FileUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;

/**
 * A preprocessor for the AQUAINT corpus:
 * <ul>
 * <li>adds paragraph tags if missing</li>
 * <li>splits paragraphs, e.g. to separate publisher details</li>
 * </ul>
 * 
 * @author Nico Schlaefer
 * @version 2006-04-30
 */
public class AQUAINTPreprocessor {
	/** Directory of the AQUAINT corpus */
	private static String dir;
	
	/**
	 * Adds paragraph tags if missing.
	 * 
	 * @return true, iff the preprocessing was successful
	 */
	private static boolean addParagraphTags() {
		File[] files = FileUtils.getFilesRec(dir);
		
		for (File file : files) {
			// read file content and modify
			MsgPrinter.printStatusMsg("Parsing " + file.getName() + "...");
			ArrayList<String> doc = new ArrayList<String>();
			boolean mod = false;
			try {
				BufferedReader in = new BufferedReader(new FileReader(file));
				String line, prevLine = "";
				boolean text = false, paragraph = false;
				
				while (in.ready()) {
					line = in.readLine();
					
					if (text) {
						if (paragraph) {
							if (line.contains("</P>")) {
								paragraph = false;
							} else if (line.contains("<P>")) {
								doc.add("</P>");
								mod = true;
							} else if (line.matches("\\s*+<[^>]++>\\s*+")) {
								doc.add("</P>");
								mod = true;
								paragraph = false;
							} else if (!prevLine.contains("<P>") &&
									   (line.startsWith("\t") ||
										line.startsWith("  "))) {
								doc.add("</P>");
								doc.add("<P>");
								mod = true;
							}
						} else {
							if (line.contains("<P>")) {
								paragraph = true;
							} else if (line.contains("</P>")) {
								doc.add("<P>");
								mod = true;
							} else if (!line.matches("\\s*+<[^>]++>\\s*+")) {
								doc.add("<P>");
								mod = true;
								paragraph = true;
							}
						}
					}
					if (line.contains("<TEXT>")) text = true;
					if (line.contains("</TEXT>")) text = false;
					doc.add(line);
					
					prevLine = line;
				}
				
				in.close();
			} catch (IOException e) {
				return false;
			}
			
			// write modified content
			if (mod) {
				try {
					PrintWriter out = new PrintWriter(file, "UTF-8");
					for (String line : doc)	out.println(line);
					out.close();
				} catch (IOException e) {
					return false;
				}
				MsgPrinter.printStatusMsg("...modified");
			}
		}
		
		return true;
	}
	
	/**
	 * Splits paragraphs, e.g. to separate publisher details.
	 * 
	 * @return true, iff the preprocessing was successful
	 */
	private static boolean splitParagraphs() {
		File[] files = FileUtils.getFilesRec(dir);
		
		for (File file : files) {
			// read file content and modify
			MsgPrinter.printStatusMsg("Parsing " + file.getName() + "...");
			ArrayList<String> doc = new ArrayList<String>();
			boolean mod = false;
			try {
				BufferedReader in = new BufferedReader(new FileReader(file));
				String line;
				boolean begin = false;
				
				while (in.ready()) {
					line = in.readLine();
					
					if (begin) {
						String[] split = line.split("( _ | -- )", 2);
						if (split.length == 2) {
							doc.add(split[0]);
							doc.add("</P>");
							doc.add("<P>");
							doc.add(split[1]);
							mod = true;
						} else
							doc.add(line);
						if (!line.contains("<P>")) begin = false;
					} else {
						doc.add(line);
						if (line.contains("<TEXT>")) begin = true;
					}
				}
				
				in.close();
			} catch (IOException e) {
				return false;
			}
			
			// write modified content
			if (mod) {
				try {
					PrintWriter out = new PrintWriter(file, "UTF-8");
					for (String line : doc)	out.println(line);
					out.close();
				} catch (IOException e) {
					return false;
				}
				MsgPrinter.printStatusMsg("...modified");
			}
		}
		
		return true;
	}
	
	/**
	 * <p>Entry point of the program.</p>
	 * 
	 * <p>Preprocesses the AQUAINT corpus.</p>
	 * 
	 * @param args argument 1: directory of the AQUAINT corpus
	 */
	public static void main(String[] args) {
		if (args.length < 1) {
			MsgPrinter.printUsage("java AQUAINTPreprocessor AQUAINT_directory");
			System.exit(1);
		}
		dir = args[0];
		
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// add paragraph tags if missing
		MsgPrinter.printStatusMsg("Adding paragraph tags...");
		if (addParagraphTags())
			MsgPrinter.printStatusMsg("Paragraph tags added successfully.");
		else {
			MsgPrinter.printErrorMsg("Could not add paragraph tags.");
			System.exit(1);
		}
		
		// split paragraphs
		MsgPrinter.printStatusMsg("Splitting paragraphs...");
		if (splitParagraphs())
			MsgPrinter.printStatusMsg("Paragraphs splitted successfully.");
		else {
			MsgPrinter.printErrorMsg("Could not split paragraphs.");
			System.exit(1);
		}
	}
}
