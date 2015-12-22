package info.ephyra.trec;

import info.ephyra.io.Logger;
import info.ephyra.io.MsgPrinter;
import info.ephyra.search.Result;

import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>Runs and evaluates Ephyra on the data from the TREC 8-11 QA tracks.</p>
 * 
 * <p>This class extends <code>OpenEphyraCorpus</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-11
 */
public class EphyraTREC8To11 extends OpenEphyraCorpus {
	/** Maximum number of factoid answers. */
	protected static final int FACTOID_MAX_ANSWERS = 5;
	/** Absolute threshold for factoid answer scores. */
	protected static final float FACTOID_ABS_THRESH = 0;
	/** Log file for the results returned by Ephyra. */
	private static String logFile;
	/** Load answers from log file? */
	private static boolean loadLog = false;
	/** Question strings. */
	protected static String[] qss;
	/** Corresponding regular expressions that describe correct answers. */
	protected static String[] regexs;
	
	/**
	 * Loads questions and patterns from files.
	 * 
	 * @param qFile name of the question file
	 * @param pFile name of the pattern file
	 */
	private static void loadTRECData(String qFile, String pFile) {
		// load questions from file
		TRECQuestion[] questions = TREC8To12Parser.loadQuestions(qFile);
		qss = new String[questions.length];
		for (int i = 0; i < questions.length; i++)
			qss[i] = questions[i].getQuestionString();
		
		// load patterns from file
		TRECPattern[] patterns = TREC8To12Parser.loadPatternsAligned(pFile);
		regexs = new String[questions.length];
		for (int i = 0; i < questions.length; i++)
			if ((i < patterns.length) && (patterns[i] != null))
				regexs[i] = patterns[i].getRegexs()[0];
	}
	
	/**
	 * Initializes Ephyra, asks the questions and evaluates and logs the
	 * answers.
	 */
	private static void runAndEval() {
		// initialize Ephyra
		EphyraTREC8To11 ephyra = new EphyraTREC8To11();
		
		float precision = 0;
		float mrr = 0;
		
		for (int i = 0; i < qss.length; i++) {
			MsgPrinter.printQuestion(qss[i]);
			
			Logger.enableLogging(false);
			
			// ask Ephyra or load answer from log file
			Result[] results = null;
			if (loadLog)
				results = TREC13To16Parser.loadResults(qss[i], "FACTOID",
													   logFile);
			if (results == null) {  // answer not loaded from log file
				Logger.enableLogging(true);
				Logger.logFactoidStart(qss[i]);
				results = ephyra.askFactoid(qss[i], FACTOID_MAX_ANSWERS,
											FACTOID_ABS_THRESH);
			}
			
			// evaluate answers
			boolean[] correct = new boolean[results.length];
			int firstCorrect = 0;
			if (regexs[i] != null) {
				Pattern p = Pattern.compile(regexs[i]);
				for (int j = 0; j < results.length; j++) {
					Matcher m = p.matcher(results[j].getAnswer());
					correct[j] = m.find();
					if (correct[j] && firstCorrect == 0) firstCorrect = j + 1;
				}
			}
			if (firstCorrect > 0) {
				precision++;
				mrr += ((float) 1) / firstCorrect;
			}
			
			Logger.logResultsJudged(results, correct);
			Logger.logFactoidEnd();
		}
		
		precision /= qss.length;
		mrr /= qss.length;
		
		Logger.logScores(precision, mrr);
	}
	
	/**
	 * Runs and evaluates Epyhra on TREC data.
	 * 
	 * @param args argument 1: name of the question file<br>
	 * 			   argument 2: name of the pattern file<br>
	 * 			   [argument 3: log=logfile (if not set an unambiguous file name
	 * 							is generated automatically)]<br>
	 * 			   [argument 5: load_log (answers are loaded from the log file
	 * 							instead of querying Ephyra)]
	 */
	public static void main(String[] args) {
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		if (args.length < 2) {
			MsgPrinter.printUsage("java EphyraTREC8To11 question_file " +
								  "pattern_file [log=logfile] [load_log]");
			System.exit(1);
		}
		
		// load questions and patterns
		loadTRECData(args[0], args[1]);
		
		for (int i = 2; i < args.length; i++)
			if (args[i].matches("log=.*")) {
				// set log file
				logFile = args[i].substring(4);
			} else if (args[i].equals("load_log")) {
				// answers are loaded from log file
				loadLog = true;
			}
		
		// if log file not set, generate unambiguous name
		if (logFile == null) {
			String n = "";
			Matcher m = Pattern.compile("\\d++").matcher(args[0]);
			if (m.find()) n = m.group(0);
			String date = "";
			Calendar c = new GregorianCalendar();
			date += c.get(Calendar.DAY_OF_MONTH);
			if (date.length() == 1) date = "0" + date;
			date = (c.get(Calendar.MONTH) + 1) + date;
			if (date.length() == 3) date = "0" + date;
			date = c.get(Calendar.YEAR) + date;
			logFile = "log/TREC" + n + "_" + date;
		}
		Logger.setLogfile(logFile);
		
		// ask Ephyra the questions and evaluate the answers
		runAndEval();
	}
}
