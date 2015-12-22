package info.ephyra.questionanalysis;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.nlp.semantics.ontologies.WordNet;
import info.ephyra.util.Dictionary;
import info.ephyra.util.FileUtils;
import info.ephyra.util.HashDictionary;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.regex.PatternSyntaxException;

import javatools.PlingStemmer;

/**
 * The <code>QuestionInterpreter</code> parses a question and determines the
 * TARGET, the CONTEXT and the PROPERTY it asks for.
 * 
 * @author Nico Schlaefer
 * @version 2005-11-09
 */
public class QuestionInterpreter {
	/** The patterns that are applied to a question. */
	private static ArrayList<QuestionPattern> questionPatterns =
		new ArrayList<QuestionPattern>();
	/** For each PROPERTY a dictionary of keywords. */
	private static Hashtable<String, HashDictionary> keywords =
		new Hashtable<String, HashDictionary>();
	/** For each PROPERTY a template for a question asking for it. */
	private static Hashtable<String, String> questionTemplates =
		new Hashtable<String, String>();
	/** For each PROPERTY a template for an answer string. */
	private static Hashtable<String, String> answerTemplates =
		new Hashtable<String, String>();
	
	/**
	 * Adds the keywords in a descriptor of a question pattern to the dictionary
	 * for the respective PROPERTY.
	 * 
	 * @param expr pattern descriptor 
	 * @param prop PROPERTY the question pattern belongs to
	 */
	private static void addKeywords(String expr, String prop) {
		// tokenize expr, delimiters are meta-characters, '<', '>' and blank
		StringTokenizer st = new StringTokenizer(expr, "\\|*+?.^$(){}[]<> ");
		
		String token;
		HashDictionary dict;
		while (st.hasMoreTokens()) {
			token = st.nextToken();
			if (token.length() > 2 && !FunctionWords.lookup(token)) {
				// token has a length of at least 3 and is not a function word
				dict = keywords.get(prop);
				if (dict == null) {  // new dictionary
					dict = new HashDictionary();
					keywords.put(prop, dict);
				}
				
				dict.add(token);  // add token to the dictionary
			}
		}
	}
	
	/**
	 * Loads the question patterns from a directory of PROPERTY files. Each file
	 * contains a list of pattern descriptors. Their format is described in the
	 * documentation of the class <code>QuestionPattern</code>.
	 * 
	 * @param dir directory of the question patterns
	 * @return true, iff the question patterns were loaded successfully
	 */
	public static boolean loadPatterns(String dir) {
		File[] files = FileUtils.getFiles(dir);
		
		try {
			BufferedReader in;
			String prop, line;
			
			for (File file : files) {
				prop = file.getName();
				in = new BufferedReader(new FileReader(file));
				
				while (in.ready()) {
					line = in.readLine().trim();
					if (line.length() == 0 || line.startsWith("//"))
						continue;  // skip blank lines and comments
					
					if (line.startsWith("QUESTION_TEMPLATE")) {
						// add question template
						String[] tokens = line.split("\\s+", 2);
						if (tokens.length > 1)
							questionTemplates.put(prop, tokens[1]);
					} else if (line.startsWith("ANSWER_TEMPLATE")) {
						// add answer template
						String[] tokens = line.split("\\s+", 2);
						if (tokens.length > 1)
							answerTemplates.put(prop, tokens[1]);
					} else {
						try {
							// add question pattern
							questionPatterns.add(new QuestionPattern(line, prop));
							// add keywords to the dictionary for prop
							addKeywords(line, prop);
						} catch (PatternSyntaxException pse) {
							MsgPrinter.printErrorMsg("Problem loading pattern:\n" +
													 prop + " " + line);
							MsgPrinter.printErrorMsg(pse.getMessage());
						}
					}
				}
				
				in.close();
			}
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Interprets a question by applying the question patterns and returns the
	 * interpretations of minimal length.
	 * 
	 * @param qn normalized question string
	 * @param stemmed stemmed question string
	 * @return array of interpretations or an empty array, if there was no
	 * 		   matching question pattern
	 */
	public static QuestionInterpretation[] interpret(String qn,
													 String stemmed) {
		ArrayList<QuestionInterpretation> qis =
			new ArrayList<QuestionInterpretation>();
		
		// apply the question patterns
		for (QuestionPattern questionPattern : questionPatterns) {
			QuestionInterpretation qi =
				questionPattern.apply(qn, stemmed);
			
			if (qi != null) qis.add(qi);
		}
		
		// sort the interpretations by their length
		QuestionInterpretation[] sorted =
			qis.toArray(new QuestionInterpretation[qis.size()]);
		Arrays.sort(sorted);
		
		// only return interpretations of minimal length
		ArrayList<QuestionInterpretation> minLength =
			new ArrayList<QuestionInterpretation>();
		for (QuestionInterpretation qi : sorted)
			if (qi.getLength() == sorted[0].getLength()) minLength.add(qi);
		
		return minLength.toArray(new QuestionInterpretation[minLength.size()]);
	}
	
	/**
	 * Looks up a word in the dictionary for the given PROPERTY.
	 * 
	 * @param word the word to be looked up
	 * @param prop the PROPERTY
	 * @return true, iff <code>word</code> is in the dictionary for
	 * 		   <code>prop</code>
	 */
	public static boolean lookupKeyword(String word, String prop) {
		Dictionary dict = keywords.get(prop);
		if (dict == null) return false;
		
		if (dict.contains(word)) return true;
		String stem = PlingStemmer.stem(word);
		if (dict.contains(stem)) return true;
		String lemma = WordNet.getLemma(word, WordNet.VERB);
		if (lemma != null && dict.contains(lemma)) return true;
		
		return false;
	}
	
	/**
	 * Returns a question string that asks for the specified property of the
	 * target object or <code>null</code> if no question template is available
	 * for the property.
	 * 
	 * @param to target object
	 * @param prop property
	 * @return question string or <code>null</code>
	 */
	public static String getQuestion(String to, String prop) {
		String question = questionTemplates.get(prop);
		if (question == null) return null;
		
		return question.replace("<TO>", to);
	}
	
	/**
	 * Returns an answer string that expresses that the property object is an
	 * instance of the specified property or <code>null</code> if no answer
	 * template is available for the property.
	 * 
	 * @param po property object
	 * @param prop property
	 * @return answer string or <code>null</code>
	 */
	public static String getAnswer(String po, String prop) {
		String answer = answerTemplates.get(prop);
		if (answer == null) return null;
		
		return answer.replace("<PO>", po);
	}
}
