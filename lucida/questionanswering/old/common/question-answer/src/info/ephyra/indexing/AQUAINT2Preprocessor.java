package info.ephyra.indexing;

import info.ephyra.io.MsgPrinter;
import info.ephyra.util.FileUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A preprocessor for the AQUAINT-2 corpus:
 * <ul>
 * <li>adds paragraph tags for document types 'multi', 'advis' and 'other'</li>
 * <li>converts documents to 'trectext' format required by Indri</li>
 * </ul>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-14
 */
public class AQUAINT2Preprocessor {
	/** Directory of the AQUAINT corpus */
	private static String dir;
	
	/**
	 * Adds paragraph tags to documents of type 'multi', 'advis' and 'other'.
	 * Documents of type 'story' are usually already tagged.
	 * 
	 * @return true, iff the preprocessing was successful
	 */
	private static boolean addParagraphTags() {
		File[] files = FileUtils.getFilesRec(dir);
		
		for (File file : files) {
			// only parse data files
			if (file.getName().contains(".")) {
				MsgPrinter.printStatusMsg("Ignoring " + file.getPath() + ".\n");
				continue;
			}
			
			MsgPrinter.printStatusMsg("Parsing " + file.getName() + "...");
			
			// read file content and modify
			ArrayList<String> doc = new ArrayList<String>();
			try {
				FileInputStream fis = new FileInputStream(file);
				BufferedReader in =
					new BufferedReader(new InputStreamReader(fis, "UTF-8"));
				String line, prevLine;
				boolean text = false, paragraph = false;
				Pattern p = Pattern.compile("\\s*+<DOC\\s*+id=\"([^\"]*+)\"" +
						"\\s*+type=\"([^\"]*+)\"\\s*+>\\s*+");
				String docNo = null, docType = null;
				boolean mod = false;
				
				while (in.ready()) {
					line = in.readLine();
					prevLine = (doc.size() > 0) ? doc.get(doc.size() - 1) : "";
					
					Matcher m = p.matcher(line);
					if (m.find()) {
						docNo = m.group(1);
						docType = m.group(2);
					}
					
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
									   line.matches("\\s*+")) {
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
					if (line.contains("</TEXT>")) {
						if (mod) {
							// print message if 'story' document was modified
							if (!(docType.equals("multi") ||
									docType.equals("advis") ||
									docType.equals("other")))
								MsgPrinter.printStatusMsg("Document " + docNo +
										" of type '" + docType + "' modified.");
						}
						mod = false;
						text = false;
					}
					
					if (!line.matches("\\s*+"))	doc.add(line);
				}
				
				in.close();
			} catch (IOException e) {
				return false;
			}
			
			// write modified content
			try {
				FileOutputStream fos = new FileOutputStream(file);
				PrintWriter out =
					new PrintWriter(new OutputStreamWriter(fos, "UTF-8"));
				for (String line : doc)	out.println(line);
				out.close();
			} catch (IOException e) {
				return false;
			}
			
			MsgPrinter.printStatusMsg("...parsed.\n");
		}
		
		return true;
	}
	
	/**
	 * Converts the documents to the 'trectext' format required by Indri.
	 * 
	 * @return true, iff the preprocessing was successful
	 */
	private static boolean convertToTrectext() {
		File[] files = FileUtils.getFilesRec(dir);
		
		for (File file : files) {
			// only parse data files
			if (file.getName().contains(".")) {
				MsgPrinter.printStatusMsg("Ignoring " + file.getPath() + ".\n");
				continue;
			}
			
			MsgPrinter.printStatusMsg("Parsing " + file.getName() + "...");
			
			// read file content and modify
			ArrayList<String> doc = new ArrayList<String>();
			try {
				FileInputStream fis = new FileInputStream(file);
				BufferedReader in =
					new BufferedReader(new InputStreamReader(fis, "UTF-8"));
				String line;
				Pattern p = Pattern.compile("\\s*+<DOC\\s*+id=\"([^\"]*+)\"" +
						"\\s*+type=\"([^\"]*+)\"\\s*+>\\s*+");
				String docNo = null, docType = null;
				
				while (in.ready()) {
					line = in.readLine();
					
					if (line.matches("\\s*+<\\?xml .*+") ||
							line.matches("\\s*+<!DOCTYPE .*+") ||
							line.matches("\\s*+<DOCSTREAM>\\s*+") ||
							line.matches("\\s*+</DOCSTREAM>\\s*+")) {
						System.out.println("Dropping line: " + line);
					} else if (line.matches("\\s*+<HEADLINE>\\s*+")) {
						doc.add("<TITLE>");
					} else if (line.matches("\\s*+</HEADLINE>\\s*+")) {
						doc.add("</TITLE>");
					} else {
						Matcher m = p.matcher(line);
						if (m.find()) {
							docNo = m.group(1);
							docType = m.group(2);
							doc.add("<DOC>");
							doc.add("<DOCNO>" + docNo + "</DOCNO>");
							doc.add("<DOCTYPE>" + docType + "</DOCTYPE>");
						} else {
							doc.add(line);
						}
					}
				}
				
				in.close();
			} catch (IOException e) {
				return false;
			}
			
			// write modified content
			try {
				FileOutputStream fos = new FileOutputStream(file);
				PrintWriter out =
					new PrintWriter(new OutputStreamWriter(fos, "UTF-8"));
				for (String line : doc)	out.println(line);
				out.close();
			} catch (IOException e) {
				return false;
			}
			
			MsgPrinter.printStatusMsg("...parsed.\n");
		}
		
		return true;
	}
	
	/**
	 * <p>Entry point of the program.</p>
	 * 
	 * <p>Preprocesses the AQUAINT-2 corpus.</p>
	 * 
	 * @param args argument 1: directory of the AQUAINT-2 corpus
	 */
	public static void main(String[] args) {
		if (args.length < 1) {
			MsgPrinter.printUsage("java AQUAINT2Preprocessor " +
					"AQUAINT2_directory");
			System.exit(1);
		}
		dir = args[0];
		
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// add paragraph tags if missing
		MsgPrinter.printStatusMsg("Adding paragraph tags:\n");
		if (addParagraphTags())
			MsgPrinter.printStatusMsg("Paragraph tags added successfully.\n");
		else {
			MsgPrinter.printErrorMsg("Could not add paragraph tags.");
			System.exit(1);
		}
		
		// convert to 'trectext'
		MsgPrinter.printStatusMsg("Converting to 'trectext' format:\n");
		if (convertToTrectext())
			MsgPrinter.printStatusMsg("Documents converted successfully.");
		else {
			MsgPrinter.printErrorMsg("Could not convert documents.");
			System.exit(1);
		}
	}
}
