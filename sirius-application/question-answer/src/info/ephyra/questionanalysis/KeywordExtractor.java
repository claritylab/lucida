package info.ephyra.questionanalysis;

import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.nlp.indices.WordFrequencies;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>Extracts keywords from a question.</p>
 * 
 * <p>The method <code>getKeywords()</code> tokenizes the question string and
 * drops single characters and bad keywords that frequently appear in questions.
 * Furthermore, all words that appear in the <code>FunctionWords</code>
 * dictionary and duplicates are dropped.</p>
 * 
 * <p>The method <code>getInfrequentKeywords()</code> additionally drops the
 * most frequent keywords if the number of words exceeds the threshold specified
 * in <code>MAX_WORDS</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-02-09
 */
public class KeywordExtractor {
	/** Tokens that are always separated with blanks. */
	private static final Pattern DELIMS1 =
		Pattern.compile("(\\!|\\?|;|\"|'|/|\\\\|\\(|\\)|\\[|\\]|\\{|\\})");
	/** Tokens that are only separated with blanks if not in between numbers.*/
	private static final Pattern DELIMS2 =
		Pattern.compile("(^|\\D)(,|\\:)($|\\D)");
	/** Tokens that are only separated with blanks if final token. */
	private static final Pattern DELIMS3 = Pattern.compile("(\\.)$");
	/** Words that should not be part of a query string. */
	private static final String IGNORE = "(names?|give|tell|list)";
	/** Maximum number of keywords that are extracted. */
	private static final int MAX_WORDS = Integer.MAX_VALUE;
	
	/**
	 * Drops single characters.
	 * 
	 * @param words array of words
	 * @return array without single characters
	 */
	private static String[] dropSingleChars(String[] words) {
		ArrayList<String> noChar = new ArrayList<String>();
		
		for (String word : words)
			if (word.length() > 1) noChar.add(word);
		
		return noChar.toArray(new String[noChar.size()]);
	}
	
	/**
	 * Drops keywords that should not be part of a query string.
	 * 
	 * @param words array of words
	 * @return array without bad keywords
	 */
	private static String[] dropBadKeywords(String[] words) {
		ArrayList<String> goodKeywords = new ArrayList<String>();
		
		for (String word : words)
			if (!word.matches("(?i)" + IGNORE)) goodKeywords.add(word);
		
		return goodKeywords.toArray(new String[goodKeywords.size()]);
	}
	
	/**
	 * Drops function words.
	 * 
	 * @param words array of words
	 * @return array without function words
	 */
	private static String[] dropFunctionWords(String[] words) {
		ArrayList<String> content = new ArrayList<String>();
		
		for (String word : words)
			if (!FunctionWords.lookup(word))
				content.add(word);
		
		return content.toArray(new String[content.size()]);
	}
	
	/**
	 * Drops duplicates.
	 * 
	 * @param words array of words
	 * @return array without duplicates
	 */
	private static String[] dropDuplicates(String[] words) {
		HashSet<String> normSet = new HashSet<String>();
		ArrayList<String> wordList = new ArrayList<String>();
		
		for (String word : words) {
			String norm = StringUtils.normalize(word);
			if (normSet.add(norm))  // compare normalizations
				wordList.add(word);
		}
		
		return wordList.toArray(new String[wordList.size()]);
	}
	
	/**
	 * Removes the most frequent words if the number of words exceeds the
	 * threshold specified in the <code>MAX_WORDS</code> field.
	 * 
	 * @param words array of words
	 * @return array of at most <code>MAX_WORDS</code> words
	 */
	private static String[] dropFrequentWords(String[] words) {
		if (words.length > MAX_WORDS) {  // number of words exceeds threshold
			// get word frequencies
			int[] frequencies = new int[words.length];
			for (int i = 0; i < words.length; i++)
				frequencies[i] = WordFrequencies.lookup(words[i]);
			
			// mark most frequent words
			int index = -1, max = -1;
			for (int i = 0; i < words.length - MAX_WORDS; i++) {
				for (int j = 0; j < words.length; j++)
					if (frequencies[j] > max) {
						index = j;
						max = frequencies[j];
					}
				
				frequencies[index] = -1;
				max = -1;
			}
			
			// create new array with rare words
			String[] rare = new String[MAX_WORDS];
			int pos = 0;
			for (int i = 0; i < words.length; i++)
				if (frequencies[i] >= 0) rare[pos++] = words[i];
			
			return rare;
		} else {  // number of words less than or equal threshold
			return words;
		}
	}
	
	/**
	 * A rule-based tokenizer used to extract keywords for a query. This
	 * tokenizer is conservative, e.g. it does not split "F16" or "1,000.00".
	 * 
	 * @param text text to tokenize
	 * @return string of space-delimited tokens
	 */
	public static String tokenizeWithSpaces(String text) {
		String rep;
		
		Matcher m1 = DELIMS1.matcher(text);
		while (m1.find()) {
			rep = " " + m1.group(0) + " ";
			text = text.replace(m1.group(0), rep);
		}
		
		Matcher m2 = DELIMS2.matcher(text);
		while (m2.find()) {
			rep = m2.group(1) + " "  + m2.group(2) + " " + m2.group(3);
			text = text.replace(m2.group(0), rep);
		}
		
		Matcher m3 = DELIMS3.matcher(text);
		if (m3.find()) {
			rep = " " + m3.group(0);
			text = text.substring(0, text.length() - 1) + rep;
		}
		
		text = text.replaceAll("\\s++", " ").trim();
		
		return text;
	}
	
	/**
	 * Applies the rule-based tokenizer and splits the resulting string along
	 * whitespaces.
	 * 
	 * @param text text to tokenize
	 * @return array of tokens
	 */
	public static String[] tokenize(String text) {
		text = tokenizeWithSpaces(text);
		return text.split(" ");
	}
	
	/**
	 * Extracts keywords from a question.
	 * 
	 * @param verbMod question string with modified verbs
	 * @return keywords
	 */
	public static String[] getKeywords(String verbMod) {
		// split question into words
		String[] words = tokenize(verbMod);
		
		// drop single characters
		words = dropSingleChars(words);
		
		// drop bad keywords
		words = dropBadKeywords(words);
		
		// drop function words
		words = dropFunctionWords(words);
		
		// drop duplicates
		words = dropDuplicates(words);
		
		return words;
	}
	
	/**
	 * Extracts keywords from a question and a context string.
	 * 
	 * @param verbMod question string with modified verbs
	 * @param context context string
	 * @return keywords
	 */
	public static String[] getKeywords(String verbMod, String context) {
		return getKeywords(verbMod + " " + context);
	}
	
	/**
	 * Extracts the up to <code>MAX_WORDS</code> least frequent keywords from a
	 * question.
	 * 
	 * @param verbMod question string with modified verbs
	 * @return keywords
	 */
	public static String[] getInfrequentKeywords(String verbMod) {
		// get all keywords
		String[] words = getKeywords(verbMod);
		
		// drop frequent keywords if the number of keywords exceeds 'MAX_WORDS'
		words = dropFrequentWords(words);
		
		return words;
	}
	
	/**
	 * Checks if the text contains one of the keywords.
	 * 
	 * @param text a text
	 * @param kws keywords
	 * @return <code>true</code> iff the text contains one of the keywords
	 */
	public static boolean containsKeyword(String text, String[] kws) {
		HashSet<String> kwsSet = new HashSet<String>();
		for (String kw : kws) kwsSet.add(kw);
		
		String[] words = tokenize(text);
		for (String word : words)
			if (kwsSet.contains(word)) return true;
		
		return false;
	}
}
