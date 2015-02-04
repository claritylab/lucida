package info.ephyra.questionanalysis;

import info.ephyra.nlp.OpenNLP;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javatools.PlingStemmer;

/**
 * <p>A simple pattern-based approach for determining the expected answer types
 * of a question.</p>
 * 
 * <p>The answer types and the patterns that identify questions asking for the
 * respective types are defined in a resource file.</p>
 * 
 * <p>The patterns for different answer types can be overlapping, thus a
 * question can be assigned more than one type.</p>
 * 
 * @author Nico Schlaefer
 * @version 2006-06-10
 */
public class AnswerTypeTester {
	/** Answer types that are supported by the tester. */
	private static ArrayList<String> answerTypes = new ArrayList<String>();
	/** Patterns identifying questions asking for these types. */
	private static ArrayList<Pattern> patterns = new ArrayList<Pattern>();
	
	/**
	 * Loads the supported answer types and the corresponding patterns from a
	 * resource file.
	 * 
	 * @param filename resource file
	 * @return true, iff the answer types and patterns were loaded successfully
	 */
	public static boolean loadAnswerTypes(String filename) {
		File file = new File(filename);
		try {
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			while (in.ready()) {
				String line = in.readLine().trim();
				if (line.length() == 0 || line.startsWith("//"))
					continue;  // skip blank lines and comments
				
				String[] tokens = line.split("\\s+", 2);
				answerTypes.add(tokens[0]);
				patterns.add(Pattern.compile("(^|\\W)" + tokens[1] + "($|\\W)",
											 Pattern.CASE_INSENSITIVE));
			}
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Determines the expected answer types for a question.
	 * 
	 * @param qn normalized question string
	 * @param stemmed stemmed question string
	 * @return expected answer types
	 */
	public static String[] getAnswerTypes(String qn, String stemmed) {
		// get nouns in the question string
		ArrayList<String> nouns = new ArrayList<String>();
		String[] tokens = OpenNLP.tokenize(qn);
		String[] pos = OpenNLP.tagPos(tokens);
		for (int i = 0; i < tokens.length; i++)
			if (pos[i].startsWith("NN"))
				nouns.add(PlingStemmer.stem(tokens[i]));
		
		// expected answer types
		ArrayList<String> expected = new ArrayList<String>();
		// for each type, index of last noun matched by answer type pattern
		ArrayList<Integer> nounIndices = new ArrayList<Integer>();
		// for each type, start of substring matched by answer type pattern
		ArrayList<Integer> matchStarts = new ArrayList<Integer>();
		// for each type, length of answer type pattern
		ArrayList<Integer> patternLengths = new ArrayList<Integer>();
		
		// get expected answer types
		for (int i = 0; i < answerTypes.size(); i++) {
			Matcher m = patterns.get(i).matcher(stemmed);
			if (m.find()) {
				expected.add(answerTypes.get(i));
				
				int lastNoun = Integer.MAX_VALUE;
				for (int n = 0; n < nouns.size(); n++)
					if (m.group(0).contains(nouns.get(n))) lastNoun = n;
				nounIndices.add(lastNoun);
				matchStarts.add(m.start());
				patternLengths.add(patterns.get(i).pattern().length());
			}
		}
		
		// get best matches, considering:
		boolean[] best = new boolean[expected.size()];
		Arrays.fill(best, true);
		// - index of last noun
		int minNounIndex = Integer.MAX_VALUE;
		for (int i = 0; i < expected.size(); i++)
			minNounIndex = Math.min(minNounIndex, nounIndices.get(i));
		for (int i = 0; i < expected.size(); i++)
			if (nounIndices.get(i) > minNounIndex) best[i] = false;
		// - start of matched substring
		int minMatchStart = Integer.MAX_VALUE;
		for (int i = 0; i < expected.size(); i++)
			if (best[i] == true)
				minMatchStart = Math.min(minMatchStart, matchStarts.get(i));
		for (int i = 0; i < expected.size(); i++)
			if (best[i] == true)
				if (matchStarts.get(i) > minMatchStart) best[i] = false;
		// - length of pattern
		int maxLength = Integer.MIN_VALUE;
		for (int i = 0; i < expected.size(); i++)
			if (best[i] == true)
				maxLength = Math.max(maxLength, patternLengths.get(i));
		for (int i = 0; i < expected.size(); i++)
			if (best[i] == true)
				if (patternLengths.get(i) < maxLength) best[i] = false;
		
		ArrayList<String> results = new ArrayList<String>();
		for (int i = 0; i < expected.size(); i++)
			if (best[i]) results.add(expected.get(i));
		return results.toArray(new String[results.size()]);
	}
}
