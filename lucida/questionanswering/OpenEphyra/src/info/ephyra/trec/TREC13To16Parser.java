package info.ephyra.trec;

import info.ephyra.io.MsgPrinter;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.QuestionInterpretation;
import info.ephyra.search.Result;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringReader;
import java.util.ArrayList;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

/**
 * A parser for the TREC 13-16 QA tracks.
 * 
 * @author Nico Schlaefer
 * @version 2008-02-07
 */
public class TREC13To16Parser {
	/** Characters that have to be replaced before parsing an XML document. */
	private static final String[] SPECIALCHARS = {"&"};
	/** Replacements for the special characters. */
	private static final String[] REPLACEMENTS = {"&amp;"};
	
	/** Cached log file entries of type "factoid". */
	private static ArrayList<String> factoidEntries;
	/** Cached log file entries of type "list". */
	private static ArrayList<String> listEntries;
	/** Cached log file entries of type "other". */
	private static ArrayList<String> otherEntries;
	
	/**
	 * Drops the cached entries of type "factoid".
	 */
	public static void dropCachedFactoidEntries() {
		factoidEntries = null;
	}
	
	/**
	 * Drops the cached entries of type "list".
	 */
	public static void dropCachedListEntries() {
		listEntries = null;
	}
	
	/**
	 * Drops the cached entries of type "other".
	 */
	public static void dropCachedOtherEntries() {
		otherEntries = null;
	}
	
	/**
	 * Drops the cached entries of all types.
	 */
	public static void dropAllCachedEntries() {
		dropCachedFactoidEntries();
		dropCachedListEntries();
		dropCachedOtherEntries();
	}
	
	/**
	 * Loads the target objects from a file.
	 * 
	 * @param filename file that contains the targets
	 * @return targets or <code>null</code>, if the file could not be parsed
	 */
	public static TRECTarget[] loadTargets(String filename) {
		try {
			// create factory object
			DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
			// create DOM parser
			DocumentBuilder parser = factory.newDocumentBuilder();
			// parse file and build tree
			Document trecD = parser.parse(new File(filename));
			
			NodeList targetL = trecD.getElementsByTagName("target");
			TRECTarget[] targets = new TRECTarget[targetL.getLength()];
			
			for (int i = 0; i < targets.length; i++) {
				Element targetE = (Element) targetL.item(i);
				
				String targetId = targetE.getAttribute("id").trim();
				String targetDesc = targetE.getAttribute("text").trim();
				
				NodeList questionL = targetE.getElementsByTagName("q");
				TRECQuestion[] questions = new TRECQuestion[questionL.getLength()];
				
				for (int j = 0; j < questions.length; j++) {
					Element questionE = (Element) questionL.item(j);
					
					String questionId = questionE.getAttribute("id").trim();
					String type = questionE.getAttribute("type").trim();
					String questionString =	questionE.getFirstChild().getNodeValue().trim();
					
					questions[j] = new TRECQuestion(questionId, type, questionString);
				}
				
				targets[i] = new TRECTarget(targetId, targetDesc, questions);
			}
			
			return targets;
		} catch (Exception e) {
			MsgPrinter.printErrorMsg("Failed to load or parse question file:");
			MsgPrinter.printErrorMsg(e.toString());
			
			return null;
		}
	}
	
	/**
	 * Loads patterns for the factoid or list questions from a file.
	 * 
	 * @param filename file that contains the patterns
	 * @return patterns or <code>null</code>, if the file could not be parsed
	 */
	public static TRECPattern[] loadPatterns(String filename) {
		File file = new File(filename);
		
		try {
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			String[] line;
			String id, lastId = "";
			ArrayList<String> regexs = new ArrayList<String>();
			ArrayList<TRECPattern> patterns = new ArrayList<TRECPattern>();
			
			while (in.ready()) {
				line = in.readLine().split(" ", 2);
				
				id = line[0];
				if (!id.equals(lastId) && !lastId.equals("")) {
					// add pattern for previous question ID
					patterns.add(new TRECPattern(lastId,
							regexs.toArray(new String[regexs.size()])));
					regexs = new ArrayList<String>();
				}
				
				regexs.add("(?i)" + line[1].trim());
				lastId = id;
			}
			
			// add last pattern
			patterns.add(new TRECPattern(lastId,
					regexs.toArray(new String[regexs.size()])));
			
			in.close();
			
			return patterns.toArray(new TRECPattern[patterns.size()]);
		} catch (IOException e) {
			MsgPrinter.printErrorMsg("Failed to load or parse pattern file:");
			MsgPrinter.printErrorMsg(e.toString());
			
			return null;
		}
	}
	
	/**
	 * Loads the results for a question from a log file.
	 * 
	 * @param question the question
	 * @param type the type of question ("factoid", "list" or "other")
	 * @param logfile the log file
	 * @return array of results or <code>null</code> if the question could not
	 * 		   be found in the log file
	 */
	public static Result[] loadResults(String question, String type, String logfile) {
		try {
			// get cached entries for given question type
			ArrayList<String> entries;
			if (type.equals("FACTOID")) entries = factoidEntries;
			else if (type.equals("LIST")) entries = listEntries;
			else entries = otherEntries;
			
			// get entries from log file if not cached
			if (entries == null) {
				entries = new ArrayList<String>();
				String entry = "";
				BufferedReader in = new BufferedReader(new FileReader(logfile));
				while (in.ready()) {
					String line = in.readLine();
					
					// handle characters that are not allowed in XML
					for (int i = 0; i < SPECIALCHARS.length; i++)
						line = line.replace(SPECIALCHARS[i], REPLACEMENTS[i]);
//					if (!line.matches("\\s*+</?\\w++>\\s*+"))
//						line = "<![CDATA[" + line.trim() + "]]>";
					
					if (line.matches("<" + type.toLowerCase() + ">")) entry = "";
					entry += line + "\n";
					if (line.matches("</" + type.toLowerCase() + ">")) entries.add(entry);
				}
				
				// cache entries
				if (type.equals("FACTOID")) factoidEntries = entries;
				else if (type.equals("LIST")) listEntries = entries;
				else otherEntries = entries;
			}
			
			// traverse entries in reverse order
			for (int i = entries.size() - 1; i >= 0; i--) {
				// create factory object
				DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
				// create DOM parser
				DocumentBuilder parser = factory.newDocumentBuilder();
				// parse entry and build tree
				Document entryD = parser.parse(new InputSource(new StringReader(entries.get(i))));
				
				// Is this the question we are looking for?
				Element questionE = (Element) entryD.getElementsByTagName("question").item(0);
				String questionS = questionE.getFirstChild().getNodeValue().trim();
				if (!questionS.equals(question)) continue;
				
				// get results
				ArrayList<Result> results = new ArrayList<Result>();
				NodeList resultsL = entryD.getElementsByTagName("result");
				for (int j = 0; j < resultsL.getLength(); j++) {
					Element resultE = (Element) resultsL.item(j);
					
					Element answerE = (Element) resultE.getElementsByTagName("answer").item(0);
					String answerS = answerE.getFirstChild().getNodeValue().trim();
					
					Element scoreE = (Element) resultE.getElementsByTagName("score").item(0);
					float scoreF = Float.parseFloat(scoreE.getFirstChild().getNodeValue().trim());
					
					Element docidE = (Element) resultE.getElementsByTagName("docid").item(0);
					String docidS = docidE.getFirstChild().getNodeValue().trim();
					
					Element qiE = (Element) resultE.getElementsByTagName("interpretation").item(0);
					QuestionInterpretation qi = null;
					if (qiE != null) {
						Element propertyE = (Element) qiE.getElementsByTagName("property").item(0);
						String propertyS = propertyE.getFirstChild().getNodeValue().trim();
						
						Element targetE = (Element) qiE.getElementsByTagName("target").item(0);
						String targetS = targetE.getFirstChild().getNodeValue().trim();
						
						NodeList contextL = qiE.getElementsByTagName("context");
						String[] contextS = new String[contextL.getLength()];
						for (int k = 0; k < contextS.length; k++) {
							Element contextE = (Element) contextL.item(k);
							contextS[k] = contextE.getFirstChild().getNodeValue().trim();
						}
						
						qi = new QuestionInterpretation(targetS, contextS, propertyS);
					}
					
					Query query = new Query(null);
					query.setInterpretation(qi);
					Result result = new Result(answerS, query, docidS);
					result.setScore(scoreF);
					
					results.add(result);
				}
				
				return results.toArray(new Result[results.size()]);
			}
			
			return null;  // question not found
		} catch (Exception e) {
			MsgPrinter.printErrorMsg("Failed to load or parse log file:");
			MsgPrinter.printErrorMsg(e.toString());
			
			return null;
		}
	}
	
	/**
	 * Appends answers to an output file.
	 * 
	 * @param filename the output file
	 * @param answers the answers
	 * @param correct for each answer a flag that is true iff the answer is
	 * 		  correct or <code>null</code> if the answers were not evaluated
	 * @param runTag tag that uniquely identifies the run
	 * @return <code>true</code>, iff the answers could be saved
	 */
	public static boolean saveAnswers(String filename, TRECAnswer[] answers,
									  boolean[] correct, String runTag) {
		try {
			PrintWriter out =
				new PrintWriter(new FileOutputStream(filename, true));
			
			for (int i = 0; i < answers.length; i++) {
				String line = answers[i].getId();
				line += "   " + runTag;
				if (correct == null || correct.length < i + 1)
					line += "   ";
				else
					line += ((correct[i]) ? " + " : " - ");
				line += answers[i].getSupportDoc();
				if (answers[i].getAnswerString() != null)
					line += "   " + answers[i].getAnswerString();
				
				out.println(line);
			}
			
			out.close();
		} catch (IOException e) {
			MsgPrinter.printErrorMsg("Failed to save answers:");
			MsgPrinter.printErrorMsg(e.toString());
			return false;
		}
		
		return true;
	}
}
