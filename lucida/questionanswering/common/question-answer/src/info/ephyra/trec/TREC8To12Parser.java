package info.ephyra.trec;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;

/**
 * A parser for the TREC 8-12 QA tracks.
 * 
 * @author Nico Schlaefer
 * @version 2007-05-25
 */
public class TREC8To12Parser {
	/** Type of the TREC 8 to 11 questions. */
	private static final String QTYPE = "FACTOID";
	
	/**
	 * Loads the questions from a file.
	 * 
	 * @param filename file that contains the questions
	 * @return questions or <code>null</code>, if the file could not be parsed
	 */
	public static TRECQuestion[] loadQuestions(String filename) {
		File file = new File(filename);
		
		try {
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			String id = "";
			String type = "";
			String line, questionString;
			TRECQuestion question;
			ArrayList<TRECQuestion> questions = new ArrayList<TRECQuestion>();
			
			while (in.ready()) {
				line = in.readLine();
				
				if (line.matches("<num>.*")) {
					id = line.split(": ")[1].trim();
					type = QTYPE;  // TREC 8 to 11
				} else if (line.matches("<type>.*")) {
					type = line.split(": ")[1].trim().toUpperCase();  // TREC 12
				} else if (line.matches("<desc>.*")) {
					questionString = in.readLine().trim();
					
					question = new TRECQuestion(id, type, questionString);
					questions.add(question);
				}
			}
			
			in.close();
			
			return questions.toArray(new TRECQuestion[questions.size()]);
		} catch (IOException e) {
			return null;  // file could not be parsed
		}
	}
	
	/**
	 * Loads the patterns from a file.
	 * 
	 * @param filename file that contains the patterns
	 * @return patterns or <code>null</code>, if the file could not be parsed
	 */
	public static TRECPattern[] loadPatterns(String filename) {
		TRECPattern[] aligned = loadPatternsAligned(filename);
		if (aligned == null) return null;
		
		// remove null-entries
		ArrayList<TRECPattern> patterns = new ArrayList<TRECPattern>();
		for (TRECPattern pattern : aligned)
			if (pattern != null) patterns.add(pattern);
		return patterns.toArray(new TRECPattern[patterns.size()]);
	}
	
	/**
	 * Loads the patterns from a file. For each skipped question ID in the input
	 * file, a <code>null</code> entry is added to the array of patterns.
	 * 
	 * @param filename file that contains the patterns
	 * @return patterns aligned to question IDs or <code>null</code>, if the
	 *         file could not be parsed
	 */
	public static TRECPattern[] loadPatternsAligned(String filename) {
		File file = new File(filename);
		
		try {
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			String[] line;
			int id, lastId = -1;
			String regex = "";
			TRECPattern pattern;
			ArrayList<TRECPattern> patterns = new ArrayList<TRECPattern>();
			
			while (in.ready()) {
				line = in.readLine().split(" ", 2);
				id = Integer.parseInt(line[0]);
				
				if (id == lastId)
					// if still the same pattern, append the regular expression
					regex += "|" + line[1].trim();
				else {  // next pattern
					if (!(lastId == -1)) {
						// if not first pattern, add previous pattern to results
						regex += ")";
						pattern = new TRECPattern(Integer.toString(lastId),
												  new String[] {regex});
						patterns.add(pattern);
						
						// some number might have been skipped
						for (int i = lastId + 1; i < id; i++)
							patterns.add(null);
					}
					
					// start new pattern
					lastId = id;
					regex = "(?i)(" + line[1].trim();  // case is ignored
				}
			}
			
			// add last pattern to results
			regex += ")";
			pattern = new TRECPattern(Integer.toString(lastId),
									  new String[] {regex});
			patterns.add(pattern);
			
			in.close();
			
			return patterns.toArray(new TRECPattern[patterns.size()]);
		} catch (IOException e) {
			return null;  // file could not be parsed
		}
	}
	
	/**
	 * Loads the answers to the TREC9 questions from a file.
	 * 
	 * @param filename file that contains the answers
	 * @return answers or <code>null</code>, if the file could not be parsed
	 */
	public static TRECAnswer[] loadTREC9Answers(String filename) {
		File file = new File(filename);
		
		try {
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			String id;
			String line, answerString;
			TRECAnswer answer;
			ArrayList<TRECAnswer> answers = new ArrayList<TRECAnswer>();
			
			while (in.ready()) {
				line = in.readLine();
				
				if (line.matches("Question.*")) {
					id = line.split(" ")[1];
					in.readLine();
					in.readLine();
					answerString = in.readLine().trim();
					
					answer = new TRECAnswer(id, answerString);
					answers.add(answer);
				}
			}
			
			in.close();
			
			return answers.toArray(new TRECAnswer[answers.size()]);
		} catch (IOException e) {
			return null;  // file could not be parsed
		}
	}
}
