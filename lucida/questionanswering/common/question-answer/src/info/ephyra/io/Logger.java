package info.ephyra.io;

import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.QuestionInterpretation;
import info.ephyra.search.Result;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.text.DecimalFormat;
import java.util.Date;

/**
 * Logs all questions that are posed to the system and the answers that are
 * returned to a file. Logging can be enabled or disabled. By default, logging
 * is enabled.
 * 
 * @author Nico Schlaefer
 * @version 2005-09-30
 */
public class Logger {
	/** The log file. */
	private static File logfile;
	/** True, iff logging is enabled. */
	private static boolean enabled = true;
	
	/**
	 * Sets the log file.
	 * 
	 * @param filename name of the log file
	 */
	public static void setLogfile(String filename) {
		logfile = new File(filename);
	}
	
	/**
	 * Enables or disables logging.
	 * 
	 * @param enable <code>true</code> to enable logging,
	 * 				 <code>false</code> to disable it
	 */
	public static void enableLogging(boolean enable) {
		enabled = enable;
	}
	
	/**
	 * Starts an entry for a factoid question.
	 * 
	 * @param question question string
	 * @return true, iff logging was successful
	 */
	public static boolean logFactoidStart(String question) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("<factoid>");
			out.println("\t<time>");
			out.println("\t\t" + (new Date()).toString());
			out.println("\t</time>");
			out.println("\t<question>");
			out.println("\t\t" + question);
			out.println("\t</question>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Ends an entry for a factoid question.
	 * 
	 * @return true, iff logging was successful
	 */
	public static boolean logFactoidEnd() {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("</factoid>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Starts an entry for a list question.
	 * 
	 * @param question question string
	 * @return true, iff logging was successful
	 */
	public static boolean logListStart(String question) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("<list>");
			out.println("\t<time>");
			out.println("\t\t" + (new Date()).toString());
			out.println("\t</time>");
			out.println("\t<question>");
			out.println("\t\t" + question);
			out.println("\t</question>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Ends an entry for a list question.
	 * 
	 * @return true, iff logging was successful
	 */
	public static boolean logListEnd() {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("</list>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Starts an entry for a definitional question.
	 * 
	 * @param question question string
	 * @return true, iff logging was successful
	 */
	public static boolean logDefinitionalStart(String question) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("<definitional>");
			out.println("\t<time>");
			out.println("\t\t" + (new Date()).toString());
			out.println("\t</time>");
			out.println("\t<question>");
			out.println("\t\t" + question);
			out.println("\t</question>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Ends an entry for a definitional question.
	 * 
	 * @return true, iff logging was successful
	 */
	public static boolean logDefinitionalEnd() {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("</definitional>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the normalization of a question.
	 * 
	 * @param qn question normalization
	 * @return true, iff logging was successful
	 */
	public static boolean logNormalization(String qn) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("\t<normalization>");
			out.println("\t\t" + qn);
			out.println("\t</normalization>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the determined answer types.
	 * 
	 * @param ats answer types
	 * @return true, iff logging was successful
	 */
	public static boolean logAnswerTypes(String[] ats) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			for (String at : ats) {
				out.println("\t<answertype>");
				out.println("\t\t" + at);
				out.println("\t</answertype>");
			}
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the interpretations of a question.
	 * 
	 * @param qis question interpretations
	 * @return true, iff logging was successful
	 */
	public static boolean logInterpretations(QuestionInterpretation[] qis) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			for (QuestionInterpretation qi : qis) {
				out.println("\t<interpretation>");
				out.println("\t\t<property>");
				out.println("\t\t\t" + qi.getProperty());
				out.println("\t\t</property>");
				out.println("\t\t<target>");
				out.println("\t\t\t" + qi.getTarget());
				out.println("\t\t</target>");
				for (String context : qi.getContext()) {
					out.println("\t\t<context>");
					out.println("\t\t\t" + context);
					out.println("\t\t</context>");
				}
				out.println("\t</interpretation>");
			}
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the predicates in a question.
	 * 
	 * @param ps predicates
	 * @return true, iff logging was successful
	 */
	public static boolean logPredicates(Predicate[] ps) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			for (Predicate p : ps) {
				out.println("\t<predicate>");
				out.println("\t\t" + p.getAnnotated());
				out.println("\t</predicate>");
			}
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the query strings.
	 * 
	 * @param queries the queries
	 * @return true, iff logging was successful
	 */
	public static boolean logQueryStrings(Query[] queries) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			for (Query query : queries) {
				out.println("\t<querystring>");
				out.println("\t\t" + query.getQueryString());
				out.println("\t</querystring>");
			}
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the results returned by the QA engine.
	 * 
	 * @param results the results
	 * @return true, iff logging was successful
	 */
	public static boolean logResults(Result[] results) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			for (Result result : results) {
				out.println("\t<result>");
				
				out.println("\t\t<answer>");
				out.println("\t\t\t" + result.getAnswer());
				out.println("\t\t</answer>");
				
				out.println("\t\t<score>");
				out.println("\t\t\t" + result.getScore());
				out.println("\t\t</score>");
				
				if (result.getDocID() != null) {
					out.println("\t\t<docid>");
					out.println("\t\t\t" + result.getDocID());
					out.println("\t\t</docid>");
				}
				
				QuestionInterpretation qi =
					result.getQuery().getInterpretation();
				if (qi != null) {
					out.println("\t\t<interpretation>");
					out.println("\t\t\t<property>");
					out.println("\t\t\t\t" + qi.getProperty());
					out.println("\t\t\t</property>");
					out.println("\t\t\t<target>");
					out.println("\t\t\t\t" + qi.getTarget());
					out.println("\t\t\t</target>");
					for (String context : qi.getContext()) {
						out.println("\t\t\t<context>");
						out.println("\t\t\t\t" + context);
						out.println("\t\t\t</context>");
					}
					out.println("\t\t</interpretation>");
				}
				
				out.println("\t</result>");
			}
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs results with true/false judgements.
	 * 
	 * @param results the results
	 * @param correct judgements
	 * @return true, iff logging was successful
	 */
	public static boolean logResultsJudged(Result[] results, boolean[] correct)
	{
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			for (int i = 0; i < results.length; i++) {
				out.println("\t<result>");
				out.println("\t\t<answer>");
				out.println("\t\t\t" + results[i].getAnswer());
				out.println("\t\t</answer>");
				out.println("\t\t<score>");
				out.println("\t\t\t" + results[i].getScore());
				out.println("\t\t</score>");
				if (results[i].getDocID() != null) {
					out.println("\t\t<docid>");
					out.println("\t\t\t" + results[i].getDocID());
					out.println("\t\t</docid>");
				}
				out.println("\t\t<correct>");
				out.println(
					(correct[i])
					? "\t\t\ttrue"
					: "\t\t\tfalse");
				out.println("\t\t</correct>");
				out.println("\t</result>");
			}
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the resulting precision and MRR.
	 * 
	 * @param precision the precision
	 * @param mrr mean reciprocal rank
	 * @return true, iff logging was successful
	 */
	public static boolean logScores(float precision, float mrr) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		DecimalFormat df = new DecimalFormat();
		df.setMaximumFractionDigits(3);
		df.setMinimumFractionDigits(3);
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("<scores>");
			out.println("\t<precision>");
			out.println("\t\t" + df.format(precision));
			out.println("\t</precision>");
			out.println("\t<mrr>");
			out.println("\t\t" + df.format(mrr));
			out.println("\t</mrr>");
			out.println("</scores>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the score of the factoid component.
	 * 
	 * @param score the score
	 * @param absThresh absolute confidence threshold for results
	 */
	public static boolean logFactoidScore(float score, float absThresh) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		DecimalFormat df = new DecimalFormat();
		df.setMaximumFractionDigits(3);
		df.setMinimumFractionDigits(3);
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("<factoidscore abs_thresh=\"" + absThresh + "\">");
			out.println("\t" + df.format(score));
			out.println("</factoidscore>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Logs the score of the list component.
	 * 
	 * @param score the score
	 * @param relThresh relative confidence threshold for results
	 */
	public static boolean logListScore(float score, float relThresh) {
		// logging is disabled or log file is not specified
		if (!enabled || logfile == null) return false;
		
		DecimalFormat df = new DecimalFormat();
		df.setMaximumFractionDigits(3);
		df.setMinimumFractionDigits(3);
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(logfile, true));
			
			out.println("<listscore rel_thresh=\"" + relThresh + "\">");
			out.println("\t" + df.format(score));
			out.println("</listscore>");
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
}
