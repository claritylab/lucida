package info.ephyra.nlp;

import org.tartarus.snowball.ext.englishStemmer;

/**
 * This class provides an interface to the Snowball stemmer for the English
 * language.
 * 
 * @author Nico Schlaefer
 * @version 2006-04-21
 */
public class SnowballStemmer {
	/** Snowball stemmer for the English language. */
	private static englishStemmer stemmer;
	
	/**
	 * Creates the stemmer.
	 */
	public static void create() {
		stemmer = new englishStemmer();
	}
	
    /**
     * Stems a single English word.
     * 
     * @param word the word to be stemmed
     * @return stemmed word
     */
	public static String stem(String word) {
		stemmer.setCurrent(word);
		stemmer.stem();
		return stemmer.getCurrent();
    }
	
	/**
	 * Stems all tokens in a string of space-delimited English words.
	 * 
	 * @param tokens string of tokens to be stemmed
	 * @return string of stemmed tokens
	 */
	public static String stemAllTokens(String tokens) {
		String[] tokenArray = tokens.split(" ");
		String stemmed = "";
		
		if (tokenArray.length > 0) stemmed += stem(tokenArray[0]);
		for (int i = 1; i < tokenArray.length; i++)
			stemmed += " " + stem(tokenArray[i]);
		
		return stemmed;
	}
}
