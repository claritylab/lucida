package info.ephyra.nlp.indices;

import info.ephyra.util.Dictionary;
import info.ephyra.util.HashDictionary;

import java.io.IOException;

/**
 * <p>A dictionary of function words in English.<p>
 * 
 * <p>Internally, it uses an instance of <code>HashDictionary</code> to store
 * the words.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-09-13
 */
public class FunctionWords {
	/** The dictionary containing the function words. */
	private static Dictionary dictionary;
	
	/**
	 * Creates the dictionary from a list of function words in a file.
	 * 
	 * @param filename file containing the function words
	 * @return true, iff the function words were loaded successfully
	 */
	public static boolean loadIndex(String filename) {
		try {
			dictionary = new HashDictionary(filename);
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Looks up a word in the dictionary.
	 * 
	 * @param word the word to look up
	 * @return true, iff it is a function word
	 */
	public static boolean lookup(String word) {
		return (dictionary != null && dictionary.contains(word));
	}
}
