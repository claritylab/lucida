package info.ephyra.indexing;

import info.ephyra.io.MsgPrinter;
import info.ephyra.util.FileUtils;
import info.ephyra.util.HTMLConverter;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.ArrayList;

/**
 * A preprocessor for the Blog06 corpus:
 * <ul>
 * <li>converts HTML to plain text</li>
 * <li>splits documents into paragraphs along structuring HTML tags</li>
 * </ul>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-16
 */
public class Blog06Preprocessor {
	/**
	 * Converts a single file.
	 * 
	 * @param input corpus file
	 */
	private static void convertFile(File input) {
		MsgPrinter.printStatusMsg("Parsing " + input.getName() + "...");
		
		// read document
		ArrayList<String> headers = new ArrayList<String>();
		ArrayList<String> contents = new ArrayList<String>();
		try {
			FileInputStream fis = new FileInputStream(input);
			BufferedReader reader =
				new BufferedReader(new InputStreamReader(fis, "UTF-8"));
			boolean content = false;
			
			while (reader.ready()) {
				if (content == false) {
					// read header
					StringBuilder sb = new StringBuilder();
					while (reader.ready()) {
						String line = reader.readLine();
						sb.append(line + "\n");
						if (line.matches("\\s*?</DOCHDR>\\s*+")) break;
					}
					headers.add(sb.toString());
					content = true;
				} else {
					// read content
					StringBuilder sb = new StringBuilder();
					while (reader.ready()) {
						String line = reader.readLine();
						sb.append(line + "\n");
						if (line.matches("\\s*?</DOC>\\s*+")) break;
					}
					contents.add(sb.toString());
					content = false;
				}
			}
			
			reader.close();
			
			if (headers.size() == 0 || contents.size() == 0 ||
					headers.size() != contents.size()) {
				MsgPrinter.printErrorMsg(input.getName() + " is malformatted.");
				System.exit(1);
			}
		} catch (IOException e) {
			MsgPrinter.printErrorMsg("Could not read from " + input.getName() +
					".");
			System.exit(1);
		}
		
		// convert contents to plain text
		for (int i = 0; i < contents.size(); i++) {
			String text = HTMLConverter.html2text(contents.get(i));
			if (text == null) {
				MsgPrinter.printErrorMsg(input.getName() +
						" could not be parsed.");
				System.exit(1);
			}
			contents.set(i, text);
		}
		
		// add paragraph tags along new lines
		for (int i = 0; i < contents.size(); i++) {
			String[] lines = contents.get(i).split("\\n");
			StringBuilder sb = new StringBuilder();
			for (int j = 0; j < lines.length; j++)
				sb.append("<P>" + lines[j] + "</P>\n");
			contents.set(i, sb.toString());
		}
		
		// write header and modified content
		File output = new File(input.getPath() + ".parsed");
		try {
			FileOutputStream fos = new FileOutputStream(output);
			PrintWriter writer =
				new PrintWriter(new OutputStreamWriter(fos, "UTF-8"));
			
			for (int i = 0; i < headers.size(); i++) {
				writer.print(headers.get(i));
				writer.print(contents.get(i));
				writer.print("</DOC>\n");
			}
			
			writer.close();
		} catch (IOException e) {
			MsgPrinter.printErrorMsg("Could not write to " + output.getName() +
					".");
			System.exit(1);
		}
	}
	
	/**
	 * <p>Entry point of the program.</p>
	 * 
	 * <p>Preprocesses the Blog06 corpus.</p>
	 * 
	 * @param args argument 1: directory of the Blog06 corpus
	 */
	public static void main(String[] args) {
		if (args.length < 1) {
			MsgPrinter.printUsage("java Blog06Preprocessor Blog06_directory");
			System.exit(1);
		}
		String dir = args[0];
		
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// convert all content files in the corpus directory
		File[] files = FileUtils.getFilesRec(dir);
		for (File file : files) {
			String filename = file.getName();
			// file must not be hidden and must end in "-" followed by a number
			if (!filename.startsWith(".") && filename.matches(".*?-\\d++"))
				convertFile(file);
		}
		
		MsgPrinter.printStatusMsg("...done.");
	}
}
