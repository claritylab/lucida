package info.ephyra.nlp.semantics;

import info.ephyra.io.MsgPrinter;
import info.ephyra.util.FileCache;
import info.ephyra.util.RegexConverter;

import java.io.BufferedReader;
import java.io.BufferedWriter;
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
 * A Wrapper for the ASSERT semantic role labeler.
 * 
 * @author Andy Schlaikjer, Nico Schlaefer
 * @version 2007-04-25
 */
public class ASSERT {
	/** Directory of ASSERT. */
	private static final String ASSERT_DIR = System.getenv("ASSERT");
	/** Run ASSERT in client mode. */
	private static final boolean ASSERT_CLIENT_MODE = false;
	
	/** Enable caching of parses. */
	private static final boolean CACHING = true;
	/** Cache directory where parses are stored. */
	private static final String CACHE_DIR = "cache/assert";
	
	/** Pattern for extracting parses from ASSERT's output file. */
	private static final Pattern PARSE_P = Pattern.compile("(\\d++): (.*+)");
	
	/**
	 * Creates a temporary file containing the sentences to be processed by ASSERT.
	 * 
	 * @param ss sentences to be parsed
	 * @return input file
	 */
	private static File createInputFile(String[] ss) throws Exception {
		try {
			File input = File.createTempFile("assert", ".input", new File(ASSERT_DIR + "/scripts"));
//			input.deleteOnExit();
			PrintWriter pw = new PrintWriter(new BufferedWriter(
				new OutputStreamWriter(new FileOutputStream(input), "ISO-8859-1")));
			
			for (String sentence : ss) {
				pw.println(sentence);
				if (pw.checkError()) throw new IOException();
			}
			
			pw.close();
			if (pw.checkError()) throw new IOException();
			
			return input;
		} catch (IOException e) {
			throw new IOException("Failed to create input file.");
		}
	}
	
	/**
	 * Instantiates an ASSERT process using the supplied input file.
	 * 
	 * @param input an input file initialized previously with a call to <code>createInputFile()</code>
	 * @return log file
	 */
	private static File execAssertProcess(File input) throws Exception {
		String basename = input.getCanonicalPath();
		basename = basename.substring(0, basename.lastIndexOf('.'));
		File logf = new File(basename + ".log");
//		logf.deleteOnExit();
		
		// instantiate an annotator process
		Process process;
		try {
			// in order to pipe stdout and stderr from our subprocess, we start
			// an instance of bash and then exec assert from within this shell.
			process = Runtime.getRuntime().exec("bash");
			PrintWriter pw = new PrintWriter(process.getOutputStream());
			
			// assert generates its intermediate and final output files within
			// the current working directory, so we first must "cd" to the
			// directory we want output to be stored, making sure to test error
			// status of the "cd" operation.
			String cmd = "cd " + input.getParent() + " 2> "
				+ logf.getCanonicalPath()
				+ "; rv=\"$?\"; if [ \"$rv\" -ne \"0\" ]; then exit $rv; fi; "
				+ ASSERT_DIR + "/scripts/assert "
				+ (ASSERT_CLIENT_MODE ? "--mode=client " : "")
				+ input.getCanonicalPath() + " >> " + logf.getCanonicalPath()
				+ " 2>&1; exit";
			
			pw.println(cmd);
			
			// close stdin, stdout, and stderr of the subprocess for good
			// measure. This is usually a good thing to do, both from a java
			// threads point of view (many helper threads to handle each
			// subprocess io stream), and daemonization of subprocess point of
			// view (we don't want the subprocess thinking we've got something
			// more to say to it).
			pw.close();
			// process.getInputStream().close();
			// process.getErrorStream().close();
		} catch (Exception e) {
			throw new Exception("Failed to execute annotator process.", e);
		}
		
		// timeout on the Assert process
		// Interruptor interruptor = new Interruptor(timeout * 1000);
		int rv;
		try {
			// start the interruptor thread just before we block on the
			// previosly instantiated annotator process.
			// interruptor.run();
			
			// block on the annotator process. if the process does not finish
			// before the timeout provided to the interruptor thread, then the
			// interruptor will interrupt this blocking call and we'll recieve
			// an InterruptedException.
			rv = process.waitFor();
			
			// as soon as the annotator process finishes, interrupt the
			// interruptor so it doesn't needlessly interrupt us!
			// interruptor.interrupt();
		} catch (InterruptedException e) {
			// the subprocess has timed out, so we kill it before throwing a new
			// exception.
			process.destroy();
			throw new Exception("Process timed out.");
		}
		
		// test return value
		if (rv != 0) throw new Exception("Process returned error code: " + rv);
		
		return logf;
	}
	
	/**
	 * Reads the annotated sentences from the output file created by ASSERT.
	 * 
	 * @param input an input file initialized previously with a call to <code>createInputFile()</code>
	 * @param sentCount number of sentences that have been passed to ASSERT
	 * @return annotated sentences
	 */
	private static String[][] readOutputFile(File input, int sentCount) throws Exception {
		try {
			String basename = input.getCanonicalPath();
			basename = basename.substring(0, basename.lastIndexOf('.'));
			File output = new File(basename + ".parses");
//			output.deleteOnExit();
			
			BufferedReader br = new BufferedReader(new InputStreamReader(new FileInputStream(output), "ISO-8859-1"));
			
			// this leaves out sentences that could not be parsed
//			ArrayList<String> as = new ArrayList<String>();
//			ArrayList<String[]> ass = new ArrayList<String[]>();
//			String line;
//			int previous = -1;
//			while ((line = br.readLine()) != null) {
//				Matcher parseM = PARSE_P.matcher(line);
//				if (parseM.find()) {
//					int sid = Integer.parseInt(parseM.group(1));
//					String annotation = parseM.group(2).trim();
//					
//					if (sid != previous) {
//						if (as.size() > 0) ass.add(as.toArray(new String[as.size()]));
//						as = new ArrayList<String>();
//						previous = sid;
//					}
//					as.add(annotation);
//				} else throw new IOException();
//			}
//			br.close();
//			if (as.size() > 0) ass.add(as.toArray(new String[as.size()]));
//			return ass.toArray(new String[ass.size()][]);
			
			// this creates an empty array for sentences that could not be parsed
			ArrayList<String> as = new ArrayList<String>();
			String[][] ass = new String[sentCount][];
			String line;
			int previous = -1;
			while ((line = br.readLine()) != null) {
				Matcher parseM = PARSE_P.matcher(line);
				if (parseM.find()) {
					int sid = Integer.parseInt(parseM.group(1));
					String annotation = parseM.group(2).trim();
					
					if (sid != previous) {
						if (as.size() > 0) ass[previous] = as.toArray(new String[as.size()]);
						as = new ArrayList<String>();
						previous = sid;
					}
					as.add(annotation);
				} else {
					if (!line.equals("ERROR: Found an empty input file... exiting."))
						throw new IOException("Malformatted line: " + line);
				}
			}
			br.close();
			if (as.size() > 0) ass[previous] = as.toArray(new String[as.size()]);
			for (int i = 0; i < ass.length; i++)
				if (ass[i] == null) ass[i] = new String[0];
			return ass;
		} catch (IOException e) {
			throw new IOException("Failed to parse output file.");
		}
	}
	
	/**
	 * Checks the log file for ASSERT failures. Returns <code>Integer.MAX_VALUE</code> if ASSERT successfully parsed the
	 * sentences or the index of the last sentence that was parsed if ASSERT failed. -1 indicates that no sentence could
	 * be parsed.
	 * 
	 * @param logf log file
	 * @return <code>Integer.MAX_VALUE</code> or index of last sentence that was parsed
	 */
	private static int checkLogFile(File logf) {
		try {
			BufferedReader br = new BufferedReader(new InputStreamReader(new FileInputStream(logf), "ISO-8859-1"));
			
			int lastIndex = -1;
			Pattern p = Pattern.compile("^(\\d++): ");
			while (br.ready()) {
				String line = br.readLine();
				
				Matcher m = p.matcher(line);
				if (m.find()) lastIndex = Integer.parseInt(m.group(1));
				
				if (line.contains(" DOMAIN/FRAME/")) {
					br.close();
					return lastIndex;  // ASSERT crashed
				}
			}
			
			br.close();
			
			return Integer.MAX_VALUE;  // log file looks ok
		} catch (IOException e) {
			return -1;  // log file cannot be read
		}
	}
	
	/**
	 * Annotates the predicates in an array of sentences.
	 * 
	 * @param ss sentences to be parsed
	 * @return annotated sentences
	 */
	public static String[][] annotatePredicates(String ss[]) {
		// drop special characters that ASSERT cannot handle
		Pattern p = Pattern.compile(".++");
		for (int i = 0; i < ss.length; i++) {
			String noSpecChar = "";
			
			Matcher m = p.matcher(ss[i]);
			while (m.find()) noSpecChar += " " + m.group(0);
			
			ss[i] = noSpecChar.trim();
		}
		
		// if caching is enabled, try to read parses from cache
		String[][] allParses = new String[ss.length][];  // parses from both cache and ASSERT
		ArrayList<Integer> originalIndices = new ArrayList<Integer>();  // used to merge parses from cache and ASSERT
		if (CACHING) {
			FileCache cache = new FileCache(CACHE_DIR);
			
			ArrayList<String> notInCache = new ArrayList<String>();  // sentences that are not in the cache
			for (int i = 0; i < ss.length; i++) {
				String[] parses = cache.read(ss[i]);
				if (parses != null)
					allParses[i] = parses;
				else {
					notInCache.add(ss[i]);
					originalIndices.add(i);
				}
			}
			
			ss = notInCache.toArray(new String[notInCache.size()]);
		}
		
		// get missing parses from ASSERT
		String[][] parses = new String[ss.length][];
		if (ss.length > 0 && ASSERT_DIR != null && ASSERT_DIR.length() > 0) {
			try {
				MsgPrinter.printStatusMsgTimestamp("Parsing " + ss.length + " sentences with ASSERT...");
				
				int beginIndex = 0;
				while (beginIndex < ss.length) {  // restart ASSERT if it crashed
					// copy sentences that have not been parsed yet
					String[] sentences = new String[ss.length - beginIndex];
					for (int i = 0; i < sentences.length; i++)
						sentences[i] = ss[i + beginIndex];
					
					// parse these sentences
					File input = createInputFile(sentences);
					File logf = execAssertProcess(input);
					String[][] output = readOutputFile(input, ss.length);
					
					// merge parses in one array
					int lastIndex = checkLogFile(logf);
					if (lastIndex > -1 && lastIndex < Integer.MAX_VALUE) {
						MsgPrinter.printErrorMsg("ASSERT could not parse sentence:\n" + sentences[lastIndex]);
						output[lastIndex] = null;
					} else if (lastIndex == Integer.MAX_VALUE) {
						lastIndex = sentences.length - 1;
					}
					lastIndex = beginIndex + lastIndex;
					for (int i = beginIndex; i <= lastIndex; i++)
						parses[i] = output[i - beginIndex];
					
					beginIndex = lastIndex + 1;
				}
				
				MsgPrinter.printStatusMsgTimestamp("...done");
			} catch (Exception e) {
				MsgPrinter.printErrorMsg("\nCould not call ASSERT:\n" + e.getMessage());
				System.exit(1);
			}
		}
		
		// if caching is enabled, write new parses to cache and merge parses from cache and ASSERT
		if (CACHING) {
			FileCache cache = new FileCache(CACHE_DIR);
			
			for (int i = 0; i < parses.length; i++) {
				if (parses[i] != null) cache.write(ss[i], parses[i]);  // write to cache
				allParses[originalIndices.get(i)] = parses[i];  // merge with results from cache
			}
		} else {
			allParses = parses;
		}
		
		// return an empty array for sentences that could not be parsed
		for (int i = 0; i < allParses.length; i++)
			if (allParses[i] == null) allParses[i] = new String[0];
		
		return allParses;
	}
	
	/**
	 * This untokenizer is tailored for ASSERT. It does not only remove abundant blanks but it also tries to restore
	 * special characters that have been dropped by ASSERT.
	 * 
	 * @param text text to untokenize
	 * @param original string that contains the original text as a subsequence
	 * @return subsequence of the original string or <code>null</code>, iff there is no such subsequence
	 */
	public static String untokenize(String text, String original) {
		// try with boundary matchers
		String regex = RegexConverter.strToRegexWithBounds(text);
		regex = regex.replace(" ", "\\W*");
		Matcher m = Pattern.compile(regex).matcher(original);
		if (m.find()) return m.group(0);
		
		// try without boundary matchers
		regex = RegexConverter.strToRegex(text);
		regex = regex.replace(" ", "\\W*");
		m = Pattern.compile(regex).matcher(original);
		if (m.find()) return m.group(0);
		
		// untokenization failed
		return null;
	}
}
