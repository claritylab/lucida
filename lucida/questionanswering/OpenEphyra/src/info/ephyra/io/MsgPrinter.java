package info.ephyra.io;

import info.ephyra.answerselection.filters.Filter;
import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.QuestionInterpretation;
import info.ephyra.search.Result;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.text.SimpleDateFormat;

/**
 * <p>Prints out status and error messages as well as results to the standard
 * output. The output of status and error messages can be enabled or disabled.
 * By default, all status and error messages are disabled.</p>
 * 
 * <p>All print methods are thread-save to avoid overlapping outputs from
 * different threads.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-14
 */
public class MsgPrinter {
	/** True, iff status messages are enabled. */
	private static boolean statusMsgs;
	/** True, iff error messages are enabled. */
	private static boolean errorMsgs;
	
	/**
	 * Enables or disables status messages.
	 * 
	 * @param enable <code>true</code> to enable status messages,
	 * 				 <code>false</code> to disable them
	 */
	public static void enableStatusMsgs(boolean enable) {
		statusMsgs = enable;
	}
	
	/**
	 * Enables or disables error messages.
	 * 
	 * @param enable <code>true</code> to enable error messages,
	 * 				 <code>false</code> to disable them
	 */
	public static void enableErrorMsgs(boolean enable) {
		errorMsgs = enable;
	}
	
	/**
	 * Prints out an arbitrary status message.
	 * 
	 * @param status a status message
	 */
	public static synchronized void printStatusMsg(String status) {
		if (statusMsgs) printMessage(status);
	}
	
	/**
	 * Prints out an arbitrary status message with a timestamp.
	 * 
	 * @param status a status message
	 */
	public static synchronized void printStatusMsgTimestamp(String status) {
		if (statusMsgs) printStatusMsg(status + " (" + getTimestamp() + ")");
	}
	
	/**
	 * Prints out a target.
	 * 
	 * @param target a target
	 */
	public static synchronized void printTarget(String target) {
		if (statusMsgs) {
			target = "Target: " + target;
			String line = "";
			for (int i = 0; i < target.length(); i++) line += "=";
			
			printMessage("\n" + line);
			printMessage(target);
			printMessage(line);
		}
	}
	
	/**
	 * Prints out a question.
	 * 
	 * @param question a question
	 */
	public static synchronized void printQuestion(String question) {
		if (statusMsgs) {
			question = "Question: " + question;
			String line = "";
			for (int i = 0; i < question.length(); i++) line += "-";
			
			printMessage("\n" + line);
			printMessage(question);
			printMessage(line);
		}
	}
	
	/**
	 * Prints out the status message that the engine is in the initialization
	 * phase.
	 */
	public static synchronized void printInitializing() {
		if (statusMsgs) printMessage("+++++ Initializing engine (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that the engine is in the coreference
	 * resolution phase.
	 */
	public static synchronized void printResolvingCoreferences() {
		if (statusMsgs) printMessage("\n+++++ Resolving Coreferences (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that the engine is in the question analysis
	 * phase.
	 */
	public static synchronized void printAnalyzingQuestion() {
		if (statusMsgs) printMessage("\n+++++ Analyzing question (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that the engine is in the query generation
	 * phase.
	 */
	public static synchronized void printGeneratingQueries() {
		if (statusMsgs) printMessage("\n+++++ Generating queries (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that the engine is in the search phase.
	 */
	public static synchronized void printSearching() {
		if (statusMsgs) printMessage("\n+++++ Searching (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that the engine is in the answer selection
	 * phase.
	 */
	public static synchronized void printSelectingAnswers() {
		if (statusMsgs) printMessage("\n+++++ Selecting Answers (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that the TREC data is being loaded.
	 */
	public static synchronized void printLoadingTRECData() {
		if (statusMsgs)
			printMessage("\n+++++ Loading TREC data (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that the questions are being interpreted.
	 */
	public static synchronized void printInterpretingQuestions() {
		if (statusMsgs)
			printMessage("\n+++++ Interpreting questions (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that queries are being formed.
	 */
	public static synchronized void printFormingQueries() {
		if (statusMsgs) printMessage("\n+++++ Forming queries (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that text passages are being fetched.
	 */
	public static synchronized void printFetchingPassages() {
		if (statusMsgs) printMessage("\n+++++ Fetching passages (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that patterns are being extracted.
	 */
	public static synchronized void printExtractingPatterns() {
		if (statusMsgs) printMessage("\n+++++ Extracting patterns (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that patterns are being saved.
	 */
	public static synchronized void printSavingPatterns() {
		if (statusMsgs) printMessage("\n+++++ Saving patterns (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that patterns are being loaded.
	 */
	public static synchronized void printLoadingPatterns() {
		if (statusMsgs) printMessage("\n+++++ Loading patterns (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that patterns are being assessed.
	 */
	public static synchronized void printAssessingPatterns() {
		if (statusMsgs) printMessage("\n+++++ Assessing patterns (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the status message that patterns are being filtered.
	 */
	public static synchronized void printFilteringPatterns() {
		if (statusMsgs) printMessage("\n+++++ Filtering patterns (" + getTimestamp() + ") +++++");
	}
	
	/**
	 * Prints out the question string.
	 * 
	 * @param qs question string
	 */
	public static synchronized void printQuestionString(String qs) {
		if (statusMsgs)	printMessage("\nQuestion: " + qs);
	}
	
	/**
	 * Prints out the question string with resolved coreferences.
	 * 
	 * @param res resolved question string
	 */
	public static synchronized void printResolvedQuestion(String res) {
		if (statusMsgs)	printMessage("\nResolved question: " + res);
	}
	
	/**
	 * Prints out the normalization of a question.
	 * 
	 * @param qn question normalization
	 */
	public static synchronized void printNormalization(String qn) {
		if (statusMsgs)	printMessage("Normalization: " + qn);
	}
	
	/**
	 * Prints out the answer types.
	 * 
	 * @param ats answer types
	 */
	public static synchronized void printAnswerTypes(String[] ats) {
		if (statusMsgs) {
			printMessage("\nAnswer types:");
			if (ats.length == 0) printMessage("-");
			for (String at : ats) printMessage(at);
		}
	}
	
	/**
	 * Prints out the interpretations of a question.
	 * 
	 * @param qis <code>QuestionInterpretation</code> array
	 */
	public static synchronized void	printInterpretations(
			QuestionInterpretation[] qis) {
		if (statusMsgs) {
			printMessage("\nInterpretations:");
			if (qis.length == 0) printMessage("-");
			for (QuestionInterpretation qi : qis) {
				printMessage(qi.toString());
			}
		}
	}
	
	/**
	 * Prints out the predicates in a question.
	 * 
	 * @param ps <code>Predicate</code> array
	 */
	public static synchronized void	printPredicates(Predicate[] ps) {
		if (statusMsgs) {
			printMessage("\nPredicates:");
			if (ps.length == 0) printMessage("-");
			for (Predicate p : ps) {
				printMessage(p.toStringMultiLine());
			}
		}
	}
	
	/**
	 * Prints out query strings.
	 * 
	 * @param queries <code>Query</code> objects
	 */
	public static synchronized void printQueryStrings(Query[] queries) {
		if (statusMsgs) {
			printMessage("Query strings:");
			for (Query query : queries)
				printMessage(query.getQueryString());
		}
	}
	
	/**
	 * Prints out the status message that a filter has started its work in the
	 * answer selection phase, plus the number of results passed to the filter.
	 * 
	 * @param	filter		the filter that has just started its work
	 * @param	resCount	the number of results passed to the filter
	 */
	public static synchronized void printFilterStarted(Filter filter,
			int resCount) {
		if (statusMsgs && (filter != null)) {
			String filterName = filter.getClass().getName();
			filterName = filterName.substring(filterName.lastIndexOf(".") + 1);
			printMessage("Filter \"" + filterName + "\" started, " + resCount +
					" Results (" + getTimestamp() + ")");
		}
	}
	
	/**
	 * Prints out the status message that a filter has finished its work in the
	 * answer selection phase, plus the number of remaining results.
	 * 
	 * @param	filter		the filter that has just finished its work
	 * @param	resCount	the number of remaining results
	 */
	public static synchronized void printFilterFinished(Filter filter,
			int resCount) {
		if (statusMsgs && (filter != null)) {
			String filterName = filter.getClass().getName();
			filterName = filterName.substring(filterName.lastIndexOf(".") + 1);
			printMessage("Filter \"" + filterName + "\" finished, " + resCount +
					" Results (" + getTimestamp() + ")");
		}
	}
	
	/**
	 * Prints out an arbitrary error message.
	 * 
	 * @param error an error message
	 */
	public static synchronized void printErrorMsg(String error) {
		if (errorMsgs) System.err.println(error);
	}
	
	/**
	 * Prints out an arbitrary error message with a timestamp.
	 * 
	 * @param error an error message
	 */
	public static synchronized void printErrorMsgTimestamp(String error) {
		if (errorMsgs) printErrorMsg(error + " (" + getTimestamp() + ")");
	}
	
	/**
	 * Prints out a search error message.
	 * 
	 * @param e an <code>Exception</code> that has been thrown
	 */
	public static synchronized void printSearchError(Exception e) {
		if (errorMsgs) {
			System.err.println("\nSearch error:");
			System.err.println(e);
		}
	}
	
	/**
	 * Prints out an HTTP error message.
	 * 
	 * @param error an error msg
	 */
	public static synchronized void printHttpError(String error) {
		if (errorMsgs) {
			System.err.println("\nHTTP error:");
			System.err.println(error);
		}
	}
	
	/**
	 * Prints out instructions on how to use the program.
	 * 
	 * @param instr instructions
	 */
	public static synchronized void printUsage(String instr) {
		printMessage("Usage: " + instr);
	}
	
	/**
	 * Prints a prompt for a question.
	 */
	public static synchronized void printQuestionPrompt() {
		System.out.print("\nQuestion: ");
	}
	
	/**
	 * Prints out the answers.
	 * 
	 * @param results <code>Result</code> objects
	 */
	public static synchronized void printAnswers(Result[] results) {
		printMessage("\nAnswer:");
		for (int i = 0; i < results.length; i++) {
			printMessage("[" + (i + 1) + "]\t" +
							   results[i].getAnswer());
			printMessage("\tScore: " + results[i].getScore());
			if (results[i].getDocID() != null)
				printMessage("\tDocument: " + results[i].getDocID());
		}
	}
	
	/**
	 * Prints out an "answer unknown" message.
	 */
	public static synchronized void printAnswerUnknown() {
		printMessage("\nSorry, I don't know the answer.");
	}
	
	/**	the DateFormat object used in getTimespamt
	 */
	private static SimpleDateFormat timestampFormatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss"); 
	
	/**
	 * @return	a timestamp String for logging
	 */
	public static synchronized String getTimestamp() {
		return timestampFormatter.format(System.currentTimeMillis());
	}
	private static BufferedWriter logWriter = null;
	
	/**	print a message
	 * @param	message		the message to print
	 */
	private static synchronized void printMessage(String message) {
		System.out.println(message);
		if (logWriter != null) try {
			logWriter.write(message);
			logWriter.newLine();
			logWriter.flush();
		} catch (Exception e) {}
	}
	
	/**	set the log file
	 * @param	logFile		the path and name of the filte to write log entries to (in addition to System.out)
	 */
	public static synchronized void setLogFile(String logFile) {
		try {
			if (logFile == null) {
				if (logWriter != null) {
					logWriter.close();
					logWriter = null;
				}
			} else {
				if (logWriter != null) logWriter.close();
				logWriter = new BufferedWriter(new FileWriter(logFile));
			}
		} catch (Exception e) {
			logWriter = null;
		}
	}
}
